"""Run ownership/timer models against exact current Buddy sound C sources."""
import argparse,hashlib,json,shutil,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];a.out.mkdir(parents=True,exist_ok=False)
for f in ['source/CLOCK/SPKCLK.C','source/FASTCLK/FASTCLK.C','source/SPKAPI.H','source/SPEAKER.H','DRIVER/MIDIMAP.C','DRIVER/MIDIAPI.H','DRIVER/PERIODS.H']:shutil.copyfile(root/f,a.out/Path(f).name)
compiler=([sys.executable,str(a.cc.resolve()),'--']if a.cc.suffix.lower()=='.py'else[str(a.cc.resolve())]);rows={}
for case in ['CLOCKQA','FASTQA','PSGQA']:
 folder=a.out/case;folder.mkdir()
 for n in ['windows.h','CHECK.C']:shutil.copyfile(root/'QA'/case/n,folder/n)
 exe=folder.resolve()/'CHECK.EXE'
 r=subprocess.run(compiler+['-x','c','-std=c89','-Wall','-Wextra','-I'+str(folder.resolve()),str(folder.resolve()/'CHECK.C'),'-o',str(exe)],capture_output=True,text=True)
 (folder/'BUILD.LOG').write_text(r.stdout+r.stderr);r.check_returncode()
 r=subprocess.run([str(exe)],cwd=root,capture_output=True,text=True);(folder/'TEST.LOG').write_text(r.stdout+r.stderr);r.check_returncode();print(r.stdout,end='');rows[case]=r.stdout.strip()
(a.out/'RESULT.JSON').write_text(json.dumps(dict(passed=True,models=rows,scope='Host models, not Win16 ABI or physical timing'),indent=2)+'\n')
