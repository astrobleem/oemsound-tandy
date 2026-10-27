"""Build only Effects Lab through the current guarded MSC6 wrapper."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,shutil,struct
from importlib.machinery import SourceFileLoader
HERE=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
for arg in ('dosbox','toolchain','lease','out'):p.add_argument('--'+arg,type=Path,required=True)
a=p.parse_args();out=a.out.resolve();tool=a.toolchain.resolve()
for n in ('BIN/CL.EXE','BIN/MASM.EXE','BIN/LINK4.EXE','LIB/SLIBCE.LIB','LIB/SLIBCEW.LIB','LIB/LIBW.LIB','DDK/286/INC/WINDOWS.H'):
 if not (tool/n).is_file():p.error('Missing external toolchain input: '+n)
if any(c in str(out)+str(tool) for c in ('"','\n','\r')):p.error('Invalid mount path')
out.mkdir(parents=True,exist_ok=False)
files={n:HERE/n for n in ('EFFECTS.C','FXCORE.C','FXCORE.H','EFFECTS.DEF','EFFECTS.RSP','BUILD.BAT','FXSTART.C','FXSTART.RSP')}
for n in ('TPLAY.C','TPLAYPOL.H','SOUNDID.H','DOSSTAT.ASM'):files[n]=HERE.parent/'LAUNCH'/n
for n in ('TIMECLK.H','TCBURST.H','TCXBASS.H','PITLINK.H'):files[n]=HERE.parents[2]/'experimental/WINLIB30/SOURCE'/n
originals={n:path.read_bytes() for n,path in files.items()}
for n,data in originals.items():(out/n).write_bytes(data.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n'))
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
guard=HERE.parent/'LAUNCH/RUNDOS.PY';loader=SourceFileLoader('effects_guard',str(guard));spec=importlib.util.spec_from_loader(loader.name,loader)
m=importlib.util.module_from_spec(spec);loader.exec_module(m);m.run(a.dosbox,out/'BUILD.CNF',a.lease,60)
log=(out/'BUILD.LOG').read_text(errors='replace')
if 'PASS' not in log or 'FAIL' in log:raise RuntimeError('Build failed: '+str(out/'BUILD.LOG'))
binary=out/'EFFECTS.EXE';exe=bytearray(binary.read_bytes());ne=struct.unpack_from('<I',exe,0x3c)[0]
assert exe[ne:ne+2]==b'NE';struct.pack_into('<H',exe,ne+0x3e,0x0300);binary.write_bytes(exe)
def pin(b):return {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
segments=[];table=ne+struct.unpack_from('<H',exe,ne+0x22)[0]
for i in range(struct.unpack_from('<H',exe,ne+0x1c)[0]):
 off,size,flags,minimum=struct.unpack_from('<HHHH',exe,table+8*i);segments.append(dict(bytes=size or 65536,minimum=minimum or 65536,flags=flags))
record={'target':'8086 Windows 3.0 real mode; MSC6/LINK4','sources':{n:pin((out/n).read_bytes()) for n in files},'original_sources':{n:pin(b) for n,b in originals.items()},'runtime':pin(exe),'launcher':pin((out/'FXSTART.EXE').read_bytes()),'segments':segments,'status':'native compile/link only; execution separate'}
(out/'BUILD.JSON').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
