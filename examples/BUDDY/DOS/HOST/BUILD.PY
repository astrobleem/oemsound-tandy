#!/usr/bin/env python3
from pathlib import Path
import subprocess,os,time,shutil,argparse,hashlib,json
p=argparse.ArgumentParser();p.add_argument('out',type=Path);p.add_argument('--repo',type=Path,required=True);p.add_argument('--dosbox',type=Path,required=True);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True)
source=Path(__file__).resolve().parents[1]/'SOURCE'
for q in source.iterdir():shutil.copyfile(q,out/q.name)
conf=f'''[sdl]\nfullscreen=false\noutput=surface\n[dosbox]\nmachine=tandy\nmemsize=16\n[cpu]\ncore=normal\ncputype=8086_prefetch\ncycles=fixed 200000\n[mixer]\nnosound=true\n[autoexec]\n@echo off\nmount C "{a.repo.resolve()}"\nmount D "{out}"\nD:\nBUILD.BAT\n'''
(out/'BUILD.CNF').write_text(conf)
env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'}
s='';t=time.monotonic();proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(out/'BUILD.CNF')],stdout=(out/'HOST.LOG').open('wb'),stderr=subprocess.STDOUT,env=env)
try:
 while time.monotonic()-t<120:
  s=(out/'BUILD.LOG').read_text(errors='replace') if (out/'BUILD.LOG').exists() else ''
  if 'PASS' in s or 'FAIL' in s or proc.poll() is not None:break
  time.sleep(.2)
finally:
 if proc.poll() is None:proc.kill()
 proc.wait()
print(s)
if 'PASS' not in s:raise SystemExit(1)
b=(out/'DOSPLAY.EXE').read_bytes();r={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'compiler':'Microsoft C 6 /G0 /AS /Os','assembler':'.8086 MASM 5.1','stack_bytes':4096}
(out/'RESULT.JSON').write_text(json.dumps(r,indent=2)+'\n');print(r)
