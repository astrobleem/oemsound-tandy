"""Reproduce native queue starvation and verify foreground refill with RAWCLK.
This uses actual production C with existing library Win16/hardware mocks.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,sys
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--libwintandy',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args();out=a.out.resolve();out.mkdir();here=Path(__file__).resolve().parent
fx=here.parent/'EFFECTS';shared=here.parents[2]/'experimental/WINLIB30/SOURCE'
win=a.libwintandy.resolve()/'libtandywin/AUDIO';tests=win/'HOST'
sources=[here/'TESTS/fxclock.c',here/'BEATPLAY.C',here/'BEATPAT.C',here/'BEATEXP.C',here/'INSTR.C',fx/'FXCORE.C',shared/'FXMERGE.C']
cc=[sys.executable,'C:/Users/chad/Documents/Codex/toolchains/gcc-ucrt64.py','--'] if os.name=='nt' else ['cc']
cmd=cc+['-x','c','-std=c89','-Wall','-Wextra','-Wno-misleading-indentation','-Wno-int-to-pointer-cast','-Wno-pointer-to-int-cast','-Wno-unused-function','-I',str(tests/'STUB'),'-I',str(here),'-I',str(fx),'-I',str(shared),'-DBEATS_RAW_TEST_SOURCE="'+(tests/'TEST_BURST.C').as_posix()+'"']+list(map(str,sources))+['-o',str(out/'clock.exe')]
env=dict(os.environ)
if os.name=='nt':env['PATH']='C:/msys64/ucrt64/bin;'+env.get('PATH','')
build=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=30)
(out/'BUILD.LOG').write_text(build.stdout+build.stderr)
if build.returncode:raise RuntimeError(build.stdout+build.stderr)
r=subprocess.run([str(out/'clock.exe')],capture_output=True,text=True,env=env,timeout=30)
(out/'TEST.LOG').write_text(r.stdout+r.stderr)
if r.returncode:raise RuntimeError(r.stdout+r.stderr)
pins={str(s):hashlib.sha256(s.read_bytes()).hexdigest() for s in sources}
for d in (here,fx,shared):
 for h in d.glob('*.H'):pins[str(h)]=hashlib.sha256(h.read_bytes()).hexdigest()
for s in (win/'SOURCE/TIMECLK.C',tests/'TEST_BURST.C',tests/'STUB/windows.h'):pins[str(s)]=hashlib.sha256(s.read_bytes()).hexdigest()
(out/'RESULT.JSON').write_text(json.dumps(dict(status='PASS',scope='Host only; reproduces underrun4 without GUI timer, then verifies continuous refill, pause and 350ms foreground gap. Native scheduler qualification remains separate.',sources=pins,result=r.stdout.strip()),indent=2))
print(r.stdout.strip())
