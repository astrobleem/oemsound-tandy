from pathlib import Path
import shutil,subprocess,os,time,json,concurrent.futures
import argparse
ap=argparse.ArgumentParser(description='Clone own installed five-mode test guests; run native PSG cases.')
ap.add_argument('--runtime-root',type=Path,required=True)
ap.add_argument('--exe',type=Path,required=True)
ap.add_argument('--dosbox',type=Path,required=True)
ap.add_argument('--out',type=Path,required=True)
args=ap.parse_args(); root=args.out.resolve()
if root.exists(): ap.error('--out must be new')
root.mkdir(parents=True)
cases=[('6404','test'),('6404','stoptest'),('6404','closetest'),('160','test'),('3204','test'),('6402','test'),('baseline','test')]
def run(case):
 mode,arg=case;src=args.runtime_root.resolve()/mode;dst=root/(mode+'-'+arg);dst.mkdir();shutil.copytree(src/'guest',dst/'guest');(dst/'captures').mkdir();shutil.copyfile(args.exe.resolve(),dst/'guest/PSGPLAY.EXE')
 s=(src/'RUN.CNF').read_text().replace(str(src.resolve()),str(dst)).replace('nosound=true','nosound=false').replace('cycles=fixed 200000','cycles=fixed 5000').replace('win /r c:\\cookie.exe','DX-CAPTURE /A /-V win /r c:\\psgplay.exe /'+arg+'\nexit')
 (dst/'RUN.CNF').write_text(s); env=os.environ.copy();env['SDL_VIDEODRIVER']='dummy';env['SDL_AUDIODRIVER']='dummy';start=time.monotonic()
 with (dst/'HOST.LOG').open('wb') as f:
  p=subprocess.Popen([str(args.dosbox.resolve()),'-nopromptfolder','-conf',str(dst/'RUN.CNF')],stdout=f,stderr=subprocess.STDOUT,env=env)
  try:p.wait(timeout=240)
  except subprocess.TimeoutExpired:p.kill();p.wait()
 r={'mode':mode,'arg':arg,'exit':p.returncode,'seconds':round(time.monotonic()-start,2),'trace':(dst/'guest/PSGTRACE.LOG').read_text() if (dst/'guest/PSGTRACE.LOG').exists() else None};(dst/'RESULT.JSON').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True);return r
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:r=list(pool.map(run,cases))
(root/'RESULTS.JSON').write_text(json.dumps(r,indent=2))
