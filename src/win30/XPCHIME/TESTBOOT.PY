from pathlib import Path
import os,shutil,subprocess,json,time,concurrent.futures
import argparse
parser=argparse.ArgumentParser(description='Test startup in new clones of own Windows guest fixtures.')
parser.add_argument('--runtime-root',type=Path,required=True)
parser.add_argument('--exe',type=Path,required=True)
parser.add_argument('--observer',type=Path,required=True)
parser.add_argument('--dosbox',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args();root=args.out.resolve()
if root.exists():parser.error('--out must be new')
root.mkdir(parents=True)
def run(case):
 mode,enabled=case;src=args.runtime_root.resolve()/mode;dst=root/(mode+('-on' if enabled else '-off'));dst.mkdir();shutil.copytree(src/'guest',dst/'guest');(dst/'captures').mkdir()
 shutil.copyfile(args.exe.resolve(),dst/'guest/XPCHIME.EXE');shutil.copyfile(args.observer.resolve(),dst/'guest/BOOTTEST.EXE')
 ini=dst/'guest/WINDOWS/WIN.INI';s=ini.read_text();assert 'run=\n' in s;runline=('c:\\xpchime.exe ' if enabled else '')+'c:\\boottest.exe';ini.write_bytes(s.replace('run=\n','run='+runline+'\n',1).replace('\n','\r\n').encode())
 s=(src/'RUN.CNF').read_text().replace(str(src.resolve()),str(dst)).replace('nosound=true','nosound=false').replace('cycles=fixed 200000','cycles=fixed 20000').replace('win /r c:\\cookie.exe','DX-CAPTURE /A /-V win /r\nexit');(dst/'RUN.CNF').write_text(s)
 env=os.environ.copy();env['SDL_VIDEODRIVER']='dummy';env['SDL_AUDIODRIVER']='dummy';start=time.monotonic()
 with (dst/'HOST.LOG').open('wb') as log:
  p=subprocess.Popen([str(args.dosbox.resolve()),'-nopromptfolder','-conf',str(dst/'RUN.CNF')],stdout=log,stderr=subprocess.STDOUT,env=env)
  try:p.wait(timeout=180)
  except subprocess.TimeoutExpired:p.kill();p.wait()
 r={'mode':mode,'enabled':enabled,'exit':p.returncode,'seconds':round(time.monotonic()-start,2),'log':(dst/'guest/BOOTTEST.LOG').read_text() if (dst/'guest/BOOTTEST.LOG').exists() else None};(dst/'RESULT.JSON').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True);return r
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:r=list(pool.map(run,[('6404',True),('6404',False),('160',True),('baseline',True)]))
(root/'RESULTS.JSON').write_text(json.dumps(r,indent=2))
