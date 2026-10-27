"""Actual Win16 acceptance using the established guarded DOSBox-X DX-CAPTURE path."""
from pathlib import Path
import argparse,hashlib,importlib.util,json,re,shutil,sys,zipfile
from importlib.machinery import SourceFileLoader
sys.path.insert(0,r'C:\Users\chad\Documents\Codex\2026-10-14\task-16\deps-local')
from pyfatfs.PyFatFS import PyFatFS
p=argparse.ArgumentParser()
for n in ('build','seed','out','dosbox','lease'):p.add_argument('--'+n,type=Path,required=True)
a=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[2];out=a.out.resolve();out.mkdir();c=out/'CROOT';c.mkdir();(c/'MMLAB').mkdir();(out/'captures').mkdir()
with PyFatFS(str(a.seed/'DISK.IMG'),offset=8704,read_only=True) as fs:
 def extract(source,dest):
  dest.mkdir(exist_ok=True)
  for n in fs.listdir(source):
   s=source+'/'+n;d=dest/n
   if fs.isdir(s):extract(s,d)
   else:d.write_bytes(fs.readbytes(s))
 extract('WINDOWS',c/'WINDOWS')
with zipfile.ZipFile(root/'artifacts/win30/TWXT2.ZIP') as z:
 for name in ('RAWCLK.DLL','TBRIDGE.EXE','RESERVE.COM'):(c/'MMLAB'/name).write_bytes(z.read('TWXT2/'+name))
 (c/'WINDOWS/SYSTEM/SOUND.DRV').write_bytes(z.read('TWXT2/SOUND.DRV'))
for name in ('EFFECTS.EXE','FXSTART.EXE'):shutil.copyfile(a.build/name,c/'MMLAB'/name)
shutil.copyfile(a.build/'EFFECTS.EXE',c/'MMLAB/RAWGUI.EXE')
for name in ('SOUND.DRV','SYSTEM.DRV'):
 if (c/'WINDOWS'/name).exists():(c/'WINDOWS'/name).unlink()
win=(c/'WINDOWS/WIN.INI').read_text(encoding='latin1').replace('\r','');win=re.sub(r'(?im)^(load|run)=.*$',lambda m:m[1]+'=',win)
(c/'WINDOWS/WIN.INI').write_bytes(win.replace('\n','\r\n').encode('latin1'))
ini=(c/'WINDOWS/SYSTEM.INI').read_text(encoding='latin1').replace('\r','');ini=re.sub(r'(?im)^shell=.*$','shell=PROGMAN.EXE',ini)
(c/'WINDOWS/SYSTEM.INI').write_bytes(ini.replace('\n','\r\n').encode('latin1'))
(c/'RUNAPP.BAT').write_bytes(b'@ECHO OFF\r\nC:\r\nCD \\MMLAB\r\nFXSTART C:\\WINDOWS /TEST > C:\\START.LOG\r\nIF ERRORLEVEL 1 GOTO FAIL\r\nECHO RETURNED>C:\\DONE.TXT\r\nGOTO END\r\n:FAIL\r\nECHO FAILED>C:\\DONE.TXT\r\n:END\r\n')
config='''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
captures={out}/captures
allow more than 640kb base memory=false
memsize=1
working directory option=force
working directory default={out}
quit warning=false
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 10000
fpu=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound=false
[midi]
mididevice=none
[autoexec]
@ECHO OFF
mount C "{c}"
SET PATH=C:\\WINDOWS;Z:\\
C:
DX-CAPTURE /V /A Z:\\COMMAND.COM /C C:\\RUNAPP.BAT > C:\\CAPTURE.LOG
exit
'''
(out/'RUN.CNF').write_text(config.format(out=out,c=c))
guard=here.parent/'LAUNCH/RUNDOS.PY';loader=SourceFileLoader('effects_guard',str(guard));spec=importlib.util.spec_from_loader(loader.name,loader);m=importlib.util.module_from_spec(spec);loader.exec_module(m)
m.run(a.dosbox,out/'RUN.CNF',a.lease,120)
text=(c/'EFFECTS.LOG').read_text();(out/'EFFECTS.LOG').write_text(text)
assert 'TOTAL_FAILURES=0' in text and 'FAIL ' not in text,text
assert (c/'DONE.TXT').read_text().strip()=='RETURNED',(c/'START.LOG').read_text()
record=dict(status='PASS',assertions=text.count('PASS '),exe_sha256=hashlib.sha256((a.build/'EFFECTS.EXE').read_bytes()).hexdigest(),scope='Actual Windows 3.0 real-mode 8086 execution in guarded DOSBox-X; 640KB, no XMS/EMS; video/audio capture; not physical hardware')
(out/'RESULT.JSON').write_text(json.dumps(record,indent=2));print(json.dumps(record))
