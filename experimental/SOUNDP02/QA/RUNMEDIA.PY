#!/usr/bin/env python3
"""Run one disposable Win3 instrument test. Supply lawful OS and display files.
The --fixture directory must contain WINDOWS/, RESERVE.COM, CPUCHECK.COM,
and LATERCPU.COM. Its source files are never changed. Do not distribute it.
"""
from pathlib import Path
import argparse,fcntl,hashlib,json,os,shutil,subprocess,time
p=argparse.ArgumentParser(description=__doc__)
for n in ['fixture','display','runtime','dosbox','out','lock']:p.add_argument('--'+n,type=Path,required=True)
p.add_argument('--app',default='INSTEST');p.add_argument('--arg',default='/test');p.add_argument('--cycles',type=int,default=25000);p.add_argument('--capture',action='store_true');p.add_argument('--ega-fonts',action='store_true');p.add_argument('--timeout',type=int,default=180)
a=p.parse_args();out=a.out.resolve();out.mkdir();g=out/'guest';g.mkdir();caps=out/'captures';caps.mkdir()
def ini(path,key,value):
 lines=path.read_text().splitlines();active=False;found=False;res=[]
 for s in lines:
  if s.startswith('['):
   if active and not found:res.append(key+'='+value);found=True
   active=s.strip().lower()=='[boot]'
  if active and '=' in s and s.split('=',1)[0].strip().lower()==key.lower():res.append(key+'='+value);found=True
  else:res.append(s)
 if not found:raise RuntimeError('Missing boot section')
 path.write_bytes(('\r\n'.join(res)+'\r\n').encode('latin1'))
shutil.copytree(a.fixture/'WINDOWS',g/'WINDOWS')
for n in ['RESERVE.COM','CPUCHECK.COM','LATERCPU.COM']:shutil.copyfile(a.fixture/n,g/n)
for f in a.runtime.iterdir():
 if f.suffix.upper() in ['.EXE','.DRV','.MID','.WZV','.WZM','.CUE']:shutil.copyfile(f,g/f.name)
(g/'JMIDI').mkdir()
for n in ['JOYMIDI.EXE','MIDIMAP.DRV']:
 if (g/n).exists():shutil.copyfile(g/n,g/'JMIDI'/n)
shutil.copyfile(a.display,g/'WINDOWS/SYSTEM/TRIAL.DRV');sys=g/'WINDOWS/SYSTEM.INI'
ini(sys,'display.drv','TRIAL.DRV');ini(sys,'shell','progman.exe')
for key,stem in [('fonts.fon','sys'),('fixedfon.fon','fix'),('oemfonts.fon','oem')]:ini(sys,key,('ega' if a.ega_fonts else 'cga')+stem+'.fon')
# Clear automatic programs only in the disposable clone.
f=g/'WINDOWS/WIN.INI';lines=f.read_text().splitlines();active=False;new=[]
for s in lines:
 if s.startswith('['):active=s.strip().lower()=='[windows]'
 if active and '=' in s and s.split('=',1)[0].strip().lower() in ['load','run']:s=s.split('=',1)[0]+'='
 new.append(s)
f.write_bytes(('\r\n'.join(new)+'\r\n').encode('latin1'))
cmd=('DX-CAPTURE /A /M /V ' if a.capture else '')+'WIN /R C:\\'+a.app+'.EXE '+a.arg
(g/'START.BAT').write_bytes(('@ECHO OFF\r\nCD \\\r\nCPUCHECK > CPUCHECK.TXT\r\nLATERCPU > LATERCPU.TXT\r\nRESERVE > RESERVE.TXT\r\nIF ERRORLEVEL 1 GOTO END\r\nSET PATH=C:\\WINDOWS;Z:\\\r\nSET TEMP=C:\\WINDOWS\\TEMP\r\n'+cmd+'\r\nECHO RETURNED > C:\\RETURNED.TXT\r\n:END\r\nEXIT\r\n').encode())
cfg=f'''[sdl]
autolock=false
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=0
memsizekb=640
allow more than 640kb base memory=false
captures={caps}
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed {a.cycles}
fpu=false
[render]
scaler=none
aspect=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound={'false' if a.capture else 'true'}
[midi]
mididevice=none
[speaker]
tandy=on
tandyrate=48000
pcspeaker=true
[autoexec]
mount C "{g}"
C:
START.BAT
'''
(out/'RUN.CNF').write_text(cfg);lock=open(a.lock,'a');fcntl.flock(lock,fcntl.LOCK_EX);t=time.monotonic()
with (out/'HOST.LOG').open('w') as log:
 proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(out/'RUN.CNF')],env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'},stdout=log,stderr=subprocess.STDOUT)
 try:proc.wait(timeout=a.timeout)
 except subprocess.TimeoutExpired:proc.kill();proc.wait()
fcntl.flock(lock,fcntl.LOCK_UN)
logs={f.name:f.read_text(errors='replace') for f in g.glob('*.LOG')}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
r={'app':a.app,'arg':a.arg,'cycles':a.cycles,'host_return':proc.returncode,'seconds':time.monotonic()-t,'returned':(g/'RETURNED.TXT').exists(),'logs':logs,'mapper_sha256':sha(g/'MIDIMAP.DRV'),'stock_sound_unchanged':sha(g/'WINDOWS/SYSTEM/SOUND.DRV')==sha(a.fixture/'WINDOWS/SYSTEM/SOUND.DRV')}
(out/'RESULT.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2));raise SystemExit(not r['returned'] or proc.returncode!=0)
