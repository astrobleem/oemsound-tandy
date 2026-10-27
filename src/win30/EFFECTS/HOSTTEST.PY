"""Compile actual fixed C89 core and export adapter through managed GCC."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--libtandy',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args()
here=Path(__file__).resolve().parent;out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);lib=a.libtandy.resolve()
cmd=[sys.executable,r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py','--','-x','c','-std=c89','-pedantic','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I'+str(here),'-I'+str(lib/'include'),str(here/'HOST.C'),str(here/'FXCORE.C'),str(lib/'src/lt_audio.c'),str(lib/'src/lt_chip.c'),'-o',str(out/'HOST.EXE')]
r=subprocess.run(cmd,capture_output=True,text=True);(out/'BUILD.LOG').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');r.check_returncode()
r=subprocess.run([str(out/'HOST.EXE'),str(out)],capture_output=True,text=True);(out/'TEST.LOG').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');r.check_returncode()
if a.native:
 (out/'ROUND.C').write_text('#include "FXCORE.H"\nint main(int n,char **v){FXPATCH p;static FXCLIP c;FILE *f;if(n!=3||FXLoad(&p,v[1])||!FXCompile(&p,&c))return 1;f=fopen(v[2],"wb");if(!f||!FXHeader(&c,f))return 2;return fclose(f)?3:0;}\n')
 cc=cmd[:cmd.index(str(here/'HOST.C'))]+[str(out/'ROUND.C'),str(here/'FXCORE.C'),'-o',str(out/'ROUND.EXE')]
 subprocess.run(cc,check=True,capture_output=True)
 subprocess.run([str(out/'ROUND.EXE'),str(a.native/'CROOT/MMLAB/NATIVE.FXP'),str(out/'NATIVE.H')],check=True)
 assert (out/'NATIVE.H').read_bytes()==(a.native/'CROOT/MMLAB/NATIVE.H').read_bytes(),'Native and host exported headers differ'
 print('PASS actual native saved patch recompiles to byte-identical exported header')
pins={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in (here/'HOST.C',here/'FXCORE.C',here/'FXCORE.H',here/'FXPLAY.H',lib/'include/lt_audio.h',lib/'include/lt_sound.h',lib/'src/lt_audio.c',lib/'src/lt_chip.c')}
(out/'RESULT.JSON').write_text(json.dumps(dict(status='PASS',command=cmd,result=r.stdout,sources=pins,native_header_equal=bool(a.native),scope='host compiler, codec, libtandy per-tick observable state equality; not native timing'),indent=2))
