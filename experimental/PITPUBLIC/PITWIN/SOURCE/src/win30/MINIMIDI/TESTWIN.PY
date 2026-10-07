#!/usr/bin/env python3
"""Disposable native Win3.0 tests; never mutate source guests or user setup."""
import argparse,hashlib,json,os,shutil,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--guest',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--build',type=Path,required=True);p.add_argument('--driver',type=Path,required=True);p.add_argument('--midi',type=Path,required=True);p.add_argument('--arg',default='/test');p.add_argument('--cycles',default='5000');p.add_argument('--capture',action='store_true');p.add_argument('--intruder',type=Path);p.add_argument('--missing-driver',action='store_true');p.add_argument('--dosbox',type=Path,required=True);a=p.parse_args()
out=a.out.resolve();out.mkdir();guest=out/'guest';shutil.copytree(a.guest,guest);(out/'captures').mkdir()
for n in ('MINIMIDI.LOG','RETURN.LOG'):
 f=guest/n
 if f.exists():f.unlink()
shutil.copyfile(a.build/'MINIMIDI.EXE',guest/'MINIMIDI.EXE');shutil.copyfile(a.driver,guest/'MIDIMAP.DRV');shutil.copyfile(a.midi,guest/'TINY.MID');(guest/'BAD.MID').write_bytes(b'MThd\0\0\0\6\0\0\0\1\0\140MTrk\0\0\0\4\0\377\057')
if a.intruder:shutil.copyfile(a.intruder,guest/'MIDIINTR.EXE')
if a.missing_driver:(guest/'MIDIMAP.DRV').unlink()
config=f'''[sdl]
autolock=false
output=surface
[dosbox]
machine=tandy
memsize=0
memsizekb=640
allow more than 640kb base memory=false
captures={out}/captures
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed {a.cycles}
fpu=false
[render]
scaler=none
doublescan=false
aspect=false
[mixer]
nosound={'false' if a.capture else 'true'}
[midi]
mididevice=none
[dos]
xms=false
ems=false
umb=false
[autoexec]
@echo off
mount c "{guest}"
c:
set path=c:\\windows;z:\\
set temp=c:\\windows\\temp
c:\\tandy88\\reserve
cd c:\\windows
{'DX-CAPTURE /A /V ' if a.capture else ''}win /r c:\\MINIMIDI.EXE {a.arg}
echo RETURNED > C:\\RETURN.LOG
exit
'''
(out/'RUN.CNF').write_text(config)
env=os.environ.copy();env['SDL_VIDEODRIVER']='dummy';env['SDL_AUDIODRIVER']='dummy';t=time.monotonic()
with (out/'HOST.LOG').open('wb') as f:
 proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(out/'RUN.CNF')],stdout=f,stderr=subprocess.STDOUT,env=env)
 try:proc.wait(timeout=120)
 except subprocess.TimeoutExpired:proc.kill();proc.wait()
logs={n:(guest/n).read_text(errors='replace') for n in ('MINIMIDI.LOG','RETURN.LOG','MIDIINTR.LOG') if (guest/n).exists()}
r={'exit':proc.returncode,'seconds':round(time.monotonic()-t,2),'arg':a.arg,'cycles':a.cycles,'logs':logs,'app_sha256':hashlib.sha256((guest/'MINIMIDI.EXE').read_bytes()).hexdigest(),'driver_sha256':hashlib.sha256(a.driver.read_bytes()).hexdigest() if not a.missing_driver else None,'system_ini':(guest/'WINDOWS/SYSTEM.INI').read_text(errors='replace'),'passed':'TOTAL_FAILURES=0' in logs.get('MINIMIDI.LOG','') and 'RETURN.LOG' in logs}
(out/'RESULT.JSON').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r));raise SystemExit(not r['passed'])
