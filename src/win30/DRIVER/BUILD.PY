#!/usr/bin/env python3
"""Build the bounded PSG test driver using the repository's existing DOS tools."""
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
for n in ('TSOUND.C','TSOUND.DEF','TSOUND.RSP','SNDTEST.DEF','SNDTEST.RSP','SHIM.ASM','DRVTEST.C','DRVTEST.DEF','DRVTEST.RSP','INTRUD.C','INTRUD.DEF','INTRUD.RSP','BUILD.BAT'):
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
if not passed or not (out/'TSOUND.DRV').is_file() or not (out/'PROD.DRV').is_file():raise SystemExit('Build failed; inspect BUILD.LOG and HOST.LOG')
exe=(out/'TSOUND.DRV').read_bytes(); prod=(out/'PROD.DRV').read_bytes()
prodmap=(out/'PROD.MAP').read_text(errors='replace').upper()
testmap=(out/'TSOUND.MAP').read_text(errors='replace').upper()
hooks=('DEBUGPSGCOUNT','DEBUGPSGTICK','DEBUGFAILNEXTTIMER','DEBUGVOICEELAPSED','DEBUGBEEPRETURNWAITING','DEBUGHOLDBEEPACTIVE','DEBUGTESTBEGINBEEP','DEBUGTESTTICK','DEBUGTESTENDBEEP')
if any(h in prodmap or h not in testmap for h in hooks):raise SystemExit('test hooks leaked from or missing in build variants')
result={'bytes':len(exe),'sha256':hashlib.sha256(exe).hexdigest(),'test_build_macro':'SOUND_TEST','production_bytes':len(prod),'production_sha256':hashlib.sha256(prod).hexdigest(),'production_test_hooks':'absent from map','test_build_test_hooks':'present in map','cpu':'/G0 8086','target':'Windows 3.0 real mode','compiler_flags':'/nologo /c /G0 /Gw /Gs /AS /W3 /Od','test_compiler_define':'/DSOUND_TEST'}
(out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
