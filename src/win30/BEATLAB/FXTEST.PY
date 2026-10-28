"""Strict host checks of the actual Effects/Beats merger; no emulator."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess, sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
here=Path(__file__).resolve().parent;fx=here.parent/'EFFECTS'
shared=here.parents[2]/'experimental/WINLIB30/SOURCE'
sources=[here/'TESTS/fx.c',here/'BEATPLAY.C',here/'BEATPAT.C',here/'BEATEXP.C',here/'INSTR.C',fx/'FXCORE.C',shared/'FXMERGE.C']
gcc=Path('C:/Users/chad/Documents/Codex/toolchains/gcc-ucrt64.py')
cc=[sys.executable,str(gcc),'--'] if os.name=='nt' else ['cc']
cmd=cc+['-x','c','-std=c89','-Wall','-Wextra','-Werror','-pedantic-errors','-Wno-misleading-indentation','-I',str(here/'TESTS'),'-I',str(here),'-I',str(fx),'-I',str(shared)]+list(map(str,sources))+['-o',str(out/'fx.exe')]
env=dict(os.environ)
if os.name=='nt':env['PATH']='C:/msys64/ucrt64/bin;'+env.get('PATH','')
build=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=45)
(out/'BUILD.LOG').write_text(build.stdout+build.stderr)
if build.returncode:raise RuntimeError(build.stdout+build.stderr)
run=subprocess.run([str(out/'fx.exe')],cwd=out,capture_output=True,text=True,env=env,timeout=45)
(out/'TEST.LOG').write_text(run.stdout+run.stderr)
if run.returncode:raise RuntimeError(run.stdout+run.stderr)
pins={str(s):hashlib.sha256(s.read_bytes()).hexdigest() for s in sources}
for d in (here,fx,shared):
 for s in d.glob('*.H'):pins[str(s)]=hashlib.sha256(s.read_bytes()).hexdigest()
(out/'RESULT.JSON').write_text(json.dumps(dict(status='PASS',scope='Host C89 source tests; no native timing/ABI/audibility claim',sources=pins,result=run.stdout.strip()),indent=2))
print(run.stdout.strip())
