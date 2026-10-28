"""Build Tandy Beats Lab, its mapper and matching Fiddle with the shared native lease."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,runpy,shutil,struct
from importlib.machinery import SourceFileLoader
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for arg in ('dosbox','toolchain','lease','out'):p.add_argument('--'+arg,type=Path,required=True)
a=p.parse_args();out=a.out.resolve();tool=a.toolchain.resolve()
for n in ('BIN/CL.EXE','BIN/LINK4.EXE','BIN/MASM.EXE','LIB/SLIBCE.LIB','LIB/SLIBCEW.LIB','LIB/LIBW.LIB','DDK/286/INC/WINDOWS.H'):
 if not (tool/n).is_file():p.error('Missing external toolchain input: '+n)
if any(c in str(out)+str(tool) for c in ('"','\n','\r')):p.error('Invalid mount path')
if not a.lease.is_file():p.error('Existing shared lease required')
runpy.run_path(str(HERE.parent/'EFFECTS/LAUNCPIN.PY'))['validate'](HERE.parent/'EFFECTS')
out.mkdir(parents=True,exist_ok=False)
files={n:HERE/n for n in ('BEATS.C','BEATPAT.C','BEATPAT.H','BEATEXP.C','BEATEXP.H','MIDIAPI.H','BEATS.DEF','BEATS.RSP','BUILD.BAT','MIDIMAP.C','MIDIMAP.DEF','MIDIMAP.RSP','ENTRY.ASM','INSTR.C','INSTR.H','PERIODS.H','BEATPLAY.C','BEATPLAY.H','BTSTART.C','BTSTART.RSP')}
for n in ('FXCORE.C','FXCORE.H'):files[n]=HERE.parent/'EFFECTS'/n
for n in ('FXMERGE.C','FXMERGE.H','TIMECLK.H','TCBURST.H','TCXBASS.H','PITLINK.H'):files[n]=HERE.parents[2]/'experimental/WINLIB30/SOURCE'/n
for n in ('TPLAYPOL.H','SOUNDID.H','DOSSTAT.ASM'):files[n]=HERE.parent/'LAUNCH'/n
files['TPLAY.C']=HERE.parent/'EFFECTS/TPLAY.C'
for n in ('MMLFIDL.C','MMLFIDL.DEF','MMLFIDL.RSP','MMLCORE.C','MMLCORE.H'):files[n]=HERE.parent/'FIDDLE'/n
if (HERE.parent/'FIDDLE/MIDIAPI.H').read_bytes()!=(HERE/'MIDIAPI.H').read_bytes():p.error('Fiddle and Beats MIDI headers differ')
originals={n:path.read_bytes() for n,path in files.items()}
for n,path in files.items():
 data=originals[n].replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
 (out/n).write_bytes(data)
config='''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=1
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 200000
fpu=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound=true
[midi]
mididevice=none
[autoexec]
mount C "{tool}" -ro
mount D "{out}"
D:
CALL BUILD.BAT
exit
'''
(out/'BUILD.CNF').write_text(config.format(tool=tool,out=out))
guard=HERE.parent/'LAUNCH/RUNDOS.PY'
loader=SourceFileLoader('beats_guard',str(guard));spec=importlib.util.spec_from_loader(loader.name,loader)
m=importlib.util.module_from_spec(spec);loader.exec_module(m)
m.run(a.dosbox,out/'BUILD.CNF',a.lease,60)
log=(out/'BUILD.LOG').read_text(errors='replace')
if 'PASS' not in log or 'FAIL' in log:raise RuntimeError('Build failed: '+str(out/'BUILD.LOG'))
runtimes={}
for name in ('BEATS.EXE','MIDIMAP.DRV','MMLFIDL.EXE'):
 exe=bytearray((out/name).read_bytes());ne=struct.unpack_from('<I',exe,0x3c)[0]
 assert exe[ne:ne+2]==b'NE';struct.pack_into('<H',exe,ne+0x3e,0x0300);(out/name).write_bytes(exe)
 runtimes[name]={'bytes':len(exe),'sha256':hashlib.sha256(exe).hexdigest()}
runtimes['BTSTART.EXE']={'bytes':(out/'BTSTART.EXE').stat().st_size,'sha256':hashlib.sha256((out/'BTSTART.EXE').read_bytes()).hexdigest()}
def pin(data):return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
record={'target':'8086 Windows 3.0 real mode; MSC6/LINK4','sources':{n:pin((out/n).read_bytes()) for n in files},'source_originals':{n:pin(data) for n,data in originals.items()},'source_paths':{n:path.relative_to(HERE.parents[2]).as_posix() for n,path in files.items()},'runtime':runtimes['BEATS.EXE'],'runtimes':runtimes,'status':'native compile/link only; execution separate'}
(out/'BUILD.JSON').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
