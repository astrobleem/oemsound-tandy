#!/usr/bin/env python3
"""Run one isolated guest at a time. No user Windows install is changed."""
from pathlib import Path
import argparse,shutil,subprocess,os,time,json
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True)
p.add_argument('--build',type=Path,required=True);p.add_argument('--mode',default='160')
p.add_argument('--app',default='BEATS');p.add_argument('--arg',default='/test')
p.add_argument('--stock-sound',action='store_true')
p.add_argument('--cycles',default='5000');p.add_argument('--video',action='store_true')
a=p.parse_args();out=a.out.resolve();base=Path('cookie-finalqa')/a.mode
base=base.resolve();out.mkdir();shutil.copytree(base/'guest',out/'guest');(out/'captures').mkdir()
for f in a.build.glob('*'):
 if f.suffix.upper() in ('.EXE','.DRV'):shutil.copyfile(f,out/'guest'/f.name)
if not a.stock_sound:shutil.copyfile('psg-package/TSOUND.DRV',out/'guest/WINDOWS/SYSTEM/SOUND.DRV')
s=(base/'RUN.CNF').read_text().replace(str(base),str(out)).replace('nosound=true','nosound=false').replace('cycles=fixed 200000','cycles=fixed '+a.cycles)
s=s.replace('win /r c:\\cookie.exe','DX-CAPTURE /A /'+('V' if a.video else '-V')+' win /r c:\\'+a.app+'.exe '+a.arg+'\necho RETURNED > C:\\RETURN.LOG\nexit')
(out/'RUN.CNF').write_text(s);env=os.environ.copy();env['SDL_VIDEODRIVER']='dummy';env['SDL_AUDIODRIVER']='dummy';start=time.monotonic()
with (out/'HOST.LOG').open('wb') as log:
 proc=subprocess.Popen(['/workspace/shared/dosbox-x/run','-nopromptfolder','-conf',str(out/'RUN.CNF')],stdout=log,stderr=subprocess.STDOUT,env=env)
 try:proc.wait(timeout=120)
 except subprocess.TimeoutExpired:proc.kill();proc.wait()
r={'exit':proc.returncode,'seconds':round(time.monotonic()-start,2),'mode':a.mode,'app':a.app,'arg':a.arg,'returned':(out/'guest/RETURN.LOG').exists(),'logs':{f.name:f.read_text(errors='replace') for f in (out/'guest').glob('*.LOG')}}
(out/'RESULT.JSON').write_text(json.dumps(r,indent=2));print(json.dumps(r))
