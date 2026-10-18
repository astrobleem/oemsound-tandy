"""Compile and run the portable playlist tests in a temporary directory."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parent.parent
env=os.environ.copy()
if os.name=='nt' and Path('C:/msys64/ucrt64/bin/gcc.exe').is_file():
 cc='C:/msys64/ucrt64/bin/gcc.exe';env['PATH']='C:/msys64/ucrt64/bin;'+env.get('PATH','')
else:cc=env.get('CC') or shutil.which('cc') or shutil.which('gcc')
if not cc:raise SystemExit('Set CC to an available C89 compiler')
with tempfile.TemporaryDirectory(prefix='winamp-playlist-') as tmp:
 exe=Path(tmp)/('playlist.exe' if os.name=='nt' else 'playlist')
 subprocess.run([cc,'-x','c','-std=c89','-Wall','-Wextra','-pedantic','-I',str(root/'SOURCE'),str(root/'HOST/TEST_PLLIST.C'),str(root/'SOURCE/PLLIST.C'),'-o',str(exe)],env=env,check=True)
 subprocess.run([str(exe)],env=env,check=True)
