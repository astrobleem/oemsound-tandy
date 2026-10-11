#!/usr/bin/env python3
"""Build only the native PSG demo, using the repository's existing DOS tools."""
import argparse, hashlib, json, os, shutil, subprocess, time
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--dosbox',type=Path,required=True)
p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[2])
p.add_argument('--out',type=Path,required=True)
a=p.parse_args(); repo=a.repo.resolve(); out=a.out.resolve()
if out.exists():p.error('--out must be new')
for n in ('BIN/CL.EXE','BIN/LINK4.EXE','LIB/SLIBCEW.LIB','LIB/LIBW.LIB'):
 if not (repo/n).is_file():p.error('Missing existing toolchain input: '+n)
out.mkdir(parents=True)
for n in ('BEATS.C','BEATS.DEF','BEATS.RSP','MIDIAPI.H','MIDIMAP.C','INSTR.C','INSTR.H','MIDIMAP.DEF','MIDIMAP.RSP','PERIODS.H','ENTRY.ASM','BUILD.BAT','INSTEST.C','INSTEST.DEF','INSTEST.RSP','MIDITEST.C','MIDITEST.DEF','MIDITEST.RSP','MIDIINTR.C','MIDIINTR.DEF','MIDIINTR.RSP'):
 shutil.copyfile(Path(__file__).parent/n,out/n)
config="""[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=16
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 200000
[mixer]
nosound=true
[midi]
mididevice=none
[autoexec]
@echo off
mount C "{repo}"
mount D "{out}"
D:
BUILD.BAT
"""
(out/'BUILD.CNF').write_text(config.format(repo=repo,out=out))
env=os.environ.copy();env.setdefault('SDL_VIDEODRIVER','dummy')
with (out/'HOST.LOG').open('wb') as log:
 proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(out/'BUILD.CNF')],stdout=log,stderr=subprocess.STDOUT,env=env)
 deadline=time.monotonic()+120;passed=False
 try:
  while time.monotonic()<deadline:
   text=(out/'BUILD.LOG').read_text(errors='replace') if (out/'BUILD.LOG').exists() else ''
   if 'PASS' in text:passed=True;break
   if 'FAIL' in text or proc.poll() is not None:break
   time.sleep(.2)
 finally:
  if proc.poll() is None:proc.kill()
  proc.wait()
if not passed or not (out/'BEATS.EXE').is_file():raise SystemExit('Build failed; inspect BUILD.LOG and HOST.LOG')
exe=(out/'BEATS.EXE').read_bytes()
result={'bytes':len(exe),'sha256':hashlib.sha256(exe).hexdigest(),'cpu':'/G0 8086','target':'Windows 3.0 real mode','compiler_flags':'/nologo /c /G0 /Gw /AS /W3 /Od'}
(out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

