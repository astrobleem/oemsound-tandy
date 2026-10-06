from pathlib import Path
import argparse,shutil,subprocess,os,time,json,concurrent.futures
p=argparse.ArgumentParser();p.add_argument('--runtime-root',type=Path,required=True);p.add_argument('--exe',type=Path,required=True);p.add_argument('--observer',type=Path,required=True);p.add_argument('--driver',type=Path);p.add_argument('--dosbox',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--case',default=None);a=p.parse_args();root=a.out.resolve()
if root.exists():p.error('--out must be new')
root.mkdir()
cases=[('6404','a'),('160','a'),('baseline','a'),('6404','v'),('6404','p'),('6404','b'),('6404','c'),('6404','k')]
if a.driver:cases.append(('6404','d'))
if a.case:cases=[x for x in cases if x[1]==a.case]
def run(case):
 mode,test=case;src=a.runtime_root.resolve()/mode;dst=root/(mode+'-'+test);dst.mkdir();shutil.copytree(src/'guest',dst/'guest');(dst/'captures').mkdir()
 shutil.copyfile(a.exe,dst/'guest/TEXIT.EXE');shutil.copyfile(a.observer,dst/'guest/EXITOBS.EXE')
 if test=='d':shutil.copyfile(a.driver,dst/'guest/WINDOWS/SYSTEM/SOUND.DRV')
 s=(src/'RUN.CNF').read_text().replace(str(src.resolve()),str(dst)).replace('nosound=true','nosound=false').replace('cycles=fixed 200000','cycles=fixed 20000').replace('win /r c:\\cookie.exe','DX-CAPTURE /A /-V win /r c:\\exitobs.exe '+test+'\necho Returned to DOS > C:\\EXITDOS.LOG\nexit');(dst/'RUN.CNF').write_text(s)
 env=os.environ.copy();env['SDL_VIDEODRIVER']='dummy';env['SDL_AUDIODRIVER']='dummy';start=time.monotonic()
 with (dst/'HOST.LOG').open('wb') as log:
  proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(dst/'RUN.CNF')],stdout=log,stderr=subprocess.STDOUT,env=env)
  try:proc.wait(timeout=180)
  except subprocess.TimeoutExpired:proc.kill();proc.wait()
 result={'mode':mode,'case':test,'exit':proc.returncode,'seconds':round(time.monotonic()-start,2)}
 for n in ['TEXIT.LOG','EXITOBS.LOG','EXITDOS.LOG']:result[n]=(dst/'guest'/n).read_text() if (dst/'guest'/n).exists() else None
 (dst/'RESULT.JSON').write_text(json.dumps(result,indent=2));print(json.dumps(result),flush=True);return result
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:r=list(pool.map(run,cases))
(root/'RESULTS.JSON').write_text(json.dumps(r,indent=2))
