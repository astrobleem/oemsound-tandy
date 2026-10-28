"""Private 8088/HX acceptance. Existing lease, owned child, bounded disk/audio.
Developer fixture only: licensed DOS/Windows/ROMs and emulator are external.
"""
from pathlib import Path
import argparse,base64,hashlib,json,msvcrt,os,re,shutil,socket,subprocess,sys,threading,time,traceback,zipfile
p=argparse.ArgumentParser(description=__doc__)
for n in ('out','build','raw_build','seed','windows_seed','emulator','roms','lease','deps','control','owned_helpers'):
 p.add_argument('--'+n.replace('_','-'),type=Path,required=True)
p.add_argument('--bound',type=int,default=480)
p.add_argument('--play-only',action='store_true',help='Diagnostic playback only; cannot qualify a consumer package')
a=p.parse_args();assert 60<=a.bound<=600
sys.path[:0]=[str(a.deps),str(a.control),str(a.owned_helpers)]
from pyfatfs.PyFatFS import PyFatFS
from src.gdbclient import GdbClient
from src.headless_input import HeadlessCtl,hold_modifier_and_tap,SC_ALT
from owned_job import OwnedJob,stop_owned
here=Path(__file__).resolve().parent;repo=here.parents[2]
out=a.out.resolve();out.mkdir();build=a.build.resolve();offset=17408
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
raw_build=json.loads((a.raw_build/'BUILD.JSON').read_bytes())
raw_runtime=(a.raw_build/'RAWCLK.DLL').read_bytes();assert pin(raw_runtime)==raw_build['runtime']
for name,wanted in raw_build['sources'].items():
 assert pin((repo/'experimental/WINLIB30/SOURCE'/name).read_bytes())==wanted,name
r=dict(status='NOT_RUN',playback_only=a.play_only,fixture='Tandy HX 8088 7.16 MHz, 640 KB, Windows 3.0 real mode, 320x200',runtime=pin((build/'BEATS.EXE').read_bytes()),raw_runtime=pin(raw_runtime),harness=pin(Path(__file__).read_bytes()),capture_rates=dict(psg=48000,speaker=49716),actions=[],bound_seconds=a.bound)
assert shutil.disk_usage(out).free>1024**3
assert (a.seed/'DISK.IMG').stat().st_size<100*1024**2
shutil.copyfile(a.seed/'DISK.IMG',out/'DISK.IMG')
cfg='''[General]
confirm_exit = 0
confirm_reset = 0
confirm_save = 0
[Machine]
cpu_family = 8088
cpu_multi = 1
cpu_speed = 7159092
cpu_use_dynarec = 0
machine = tandy1000hx
mem_size = 640
time_sync = disabled
[Video]
gfxcard = internal
[Input devices]
keyboard_type = internal
mouse_type = msserial
[Storage controllers]
hdc_1 = xtide
[Hard disks]
hdd_01_fn = DISK.IMG
hdd_01_ide_channel = 0:0
hdd_01_parameters = 34, 5, 969, 0, ide
hdd_01_speed = ramdisk
[PC/XT XTIDE #1]
bios = xt
'''
(out/'86box.cfg').write_text(cfg)
with zipfile.ZipFile(repo/'artifacts/win30/TWXT2.ZIP') as z:
 runtime={n:z.read('TWXT2/'+n) for n in ('RAWCLK.DLL','SOUND.DRV','TBRIDGE.EXE','RESERVE.COM')}
runtime['RAWCLK.DLL']=raw_runtime
with PyFatFS(str(out/'DISK.IMG'),offset=offset) as fs,PyFatFS(str(a.windows_seed/'DISK.IMG'),offset=8704,read_only=True) as win:
 def put(n,b):
  if fs.exists(n):fs.remove(n)
  fs.writebytes(n,b)
 def copydir(source):
  if not fs.exists(source):fs.makedir(source)
  for n in win.listdir(source):
   path=source+'/'+n
   if win.isdir(path):copydir(path)
   else:put(path,win.readbytes(path))
 copydir('WINDOWS')
 if not fs.exists('MMLAB'):fs.makedir('MMLAB')
 for n,b in runtime.items():put('MMLAB/'+n,b)
 for n in ('BEATS.EXE','BTSTART.EXE'):put('MMLAB/'+n,(build/n).read_bytes())
 put('MMLAB/RAWGUI.EXE',(build/'BEATS.EXE').read_bytes())
 put('WINDOWS/SYSTEM/SOUND.DRV',runtime['SOUND.DRV'])
 for n in ('WINDOWS/SOUND.DRV','WINDOWS/SYSTEM.DRV'):
  if fs.exists(n):fs.remove(n)
 ini=fs.readtext('WINDOWS/WIN.INI').replace('\r','')
 ini=re.sub(r'(?im)^(load|run)=.*$',lambda m:m[1]+'=',ini)
 put('WINDOWS/WIN.INI',ini.replace('\n','\r\n').encode('latin1'))
 put('CONFIG.SYS',b'FILES=40\r\nBUFFERS=8\r\n')
 put('AUTOEXEC.BAT',b'@ECHO OFF\r\nPATH C:\\;C:\\WINDOWS;C:\\DOS\r\nSET TEMP=C:\\WINDOWS\\TEMP\r\nCD \\MMLAB\r\nECHO TEST>C:\\PHASE.TXT\r\nBTSTART C:\\WINDOWS /TEST > C:\\START.LOG\r\nIF ERRORLEVEL 1 GOTO FAILED\r\nECHO VISUAL>C:\\PHASE.TXT\r\nBTSTART C:\\WINDOWS > C:\\VISUAL.LOG\r\nIF ERRORLEVEL 1 GOTO FAILED\r\nECHO RETURNED>C:\\DONE.TXT\r\nGOTO END\r\n:FAILED\r\nECHO FAILED>C:\\DONE.TXT\r\n:END\r\n')
 if a.play_only:
  auto=fs.readbytes('AUTOEXEC.BAT');first=auto.index(b'ECHO VISUAL');last=auto.index(b'ECHO RETURNED')
  put('AUTOEXEC.BAT',auto[:first]+auto[last:]);put('MMLAB/BEATFAST.FLG',b'Diagnostic only\r\n')
 elif fs.exists('MMLAB/BEATFAST.FLG'):fs.remove('MMLAB/BEATFAST.FLG')
 for n in ('BEATS.LOG','DONE.TXT','PHASE.TXT','MMLAB/IMPORT.FXP','MMLAB/FXSAVE.BTS','MMLAB/FXSAVE.BAK','MMLAB/NATIVE.BTS','MMLAB/NATIVE.BAK','MMLAB/NATIVE.MML'):
  if fs.exists(n):fs.remove(n)
proc=g=ctl=job=watch=host=None;held=False;start=time.monotonic();visual=False
lease=a.lease.open('rb')
def inventory():return subprocess.check_output(['powershell.exe','-NoProfile','-Command',"@(Get-Process -Name dosbox*,86Box,TRACEIO,FIXBOX,pcem,tandybox -ErrorAction SilentlyContinue).Count"],text=True).strip()
try:
 assert lease.read(1);lease.seek(0);msvcrt.locking(lease.fileno(),msvcrt.LK_NBLCK,1);held=True;r['lease_acquired']=True
 assert inventory()=='0','Existing emulator left untouched'
 for port in (12345,12346):
  with socket.socket() as s:assert s.connect_ex(('127.0.0.1',port))!=0,'Control port occupied'
 env=dict(os.environ,QT_QPA_PLATFORM='offscreen',CGA_PCM_CAPTURE=str(out/'speaker.s16le'),CGA_PSG_CAPTURE=str(out/'psg.s16le'),PATH=r'C:\msys64\ucrt64\bin;'+os.environ.get('PATH',''))
 si=subprocess.STARTUPINFO();si.dwFlags|=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
 host=(out/'HOST.LOG').open('wb');job=OwnedJob()
 proc=subprocess.Popen([str(a.emulator),'-P',str(out),'-R',str(a.roms),'-N'],cwd=out,env=env,stdout=host,stderr=subprocess.STDOUT,startupinfo=si,creationflags=subprocess.CREATE_NO_WINDOW);job.assign(proc)
 r['owned_pid']=proc.pid;r['emulator']=pin(a.emulator.read_bytes());(out/'OWNED.json').write_text(json.dumps(r))
 watch=threading.Timer(a.bound,stop_owned,args=(proc,));watch.daemon=True;watch.start()
 print('Owned 8088 emulator',proc.pid,flush=True)
 for unused in range(60):
  assert proc.poll() is None
  try:g=GdbClient(timeout=3);g.connect();ctl=HeadlessCtl('127.0.0.1',12346);break
  except (OSError,ConnectionError):
   if g:g.close()
   g=None;time.sleep(.3)
 assert ctl and g
 def rest(t):g.run_while(lambda:None,settle_seconds=t,start_delay=.01)
 def key(sc):g.run_while(lambda:ctl.tap(sc),settle_seconds=.7,start_delay=.01)
 def shot(name):(out/(name+'.PNG')).write_bytes(base64.b64decode(ctl.capture_frame_png_b64()))
 def mark(name):
  row=dict(name=name,wall=round(time.monotonic()-start,2),gate=g.in_port(0x61)&3)
  for n in ('psg','speaker'):
   path=out/(n+'.s16le');row[n+'_frames']=path.stat().st_size//4 if path.exists() else 0
  r['actions'].append(row);(out/'ACTIONS.json').write_text(json.dumps(r['actions'],indent=2));return row
 last=None
 while time.monotonic()-start<a.bound-10:
  assert proc.poll() is None;rest(.6)
  mode=g.read_memory(0x449,1)[0]
  with PyFatFS(str(out/'DISK.IMG'),offset=offset,read_only=True) as fs:
   phase=fs.readtext('PHASE.TXT').strip() if fs.exists('PHASE.TXT') else 'boot'
   text=fs.readtext('BEATS.LOG') if fs.exists('BEATS.LOG') else ''
   done=fs.readtext('DONE.TXT').strip() if fs.exists('DONE.TXT') else ''
   launch=fs.readtext('START.LOG') if fs.exists('START.LOG') else ''
  (out/'BEATS.LOG').write_text(text);(out/'START.LOG').write_text(launch)
  sig=(mode,phase,len(text))
  if sig!=last:print(round(time.monotonic()-start,1),sig,text[-130:],flush=True);last=sig
  (out/'LIVE.json').write_text(json.dumps(dict(seconds=round(time.monotonic()-start,1),mode=mode,phase=phase)))
  if 'FAIL ' in text or re.search(r'TOTAL_FAILURES=[1-9]',text):raise RuntimeError('Native assertions failed')
  if phase=='VISUAL' and mode==9 and not visual:
   rest(55);shot('EDITOR')
   paint_wait=20 # Finish each stopped demo grid; test active repaint below.
   for demo in range(5):
    key(0x20)
    if demo<4:rest(paint_wait)
   # Play immediately after the last Demo; pending grid work must not delay onset.
   prep=re.search(r'prepare (\d+) ms',text);ready_wait=max(2,(int(prep[1])/1000+2) if prep else 15)
   mark('COLD_PLAY_REQUEST');key(0x3f);rest(min(ready_wait+3,55));shot('PLAYING');begin=mark('PLAYING_BEGIN')
   # Measured emulated time, not a host sleep, must exceed startup lookahead.
   limit=time.monotonic()+40
   while any((out/(n+'.s16le')).stat().st_size//4-begin[n+'_frames']<11*rate for n,rate in r['capture_rates'].items()):
    assert time.monotonic()<limit,'Capture did not advance11 emulated seconds';rest(1)
   mark('PLAYING_END')
   key(0x4d);mark('REPAINT_BEFORE_PAUSE');key(0x40);rest(.5);shot('PAUSED');mark('PAUSED_BEGIN');rest(1);assert mark('PAUSED_END')['gate']==0
   key(0x40);rest(1);mark('RESUMED_BEGIN');rest(2);mark('RESUMED_END')
   key(0x4d);mark('REPAINT_BEFORE_STOP');key(0x01);rest(.5);shot('STOPPED');mark('STOPPED_BEGIN');rest(1);assert mark('STOPPED_END')['gate']==0
   mark('WARM_PLAY_REQUEST');key(0x3f);rest(min(ready_wait+3,55));mark('WARM_BEGIN');rest(2);mark('WARM_END')
   key(0x01);rest(1);assert mark('WARM_STOPPED')['gate']==0
   g.run_while(lambda:hold_modifier_and_tap(ctl,SC_ALT,0x3e),settle_seconds=2,start_delay=.01)
   # Program Manager's normal exit confirmation.
   g.run_while(lambda:hold_modifier_and_tap(ctl,SC_ALT,0x3e),settle_seconds=1,start_delay=.01);key(0x1c)
   visual=True
  if done:
   assert done=='RETURNED',launch
   assert mode==3 and 'TOTAL_FAILURES=0' in text and (visual or a.play_only)
   with PyFatFS(str(out/'DISK.IMG'),offset=offset,read_only=True) as fs:
    if not a.play_only:(out/'FXSAVE.BTS').write_bytes(fs.readbytes('MMLAB/FXSAVE.BTS'))
   r.update(status='DIAGNOSTIC_PASS' if a.play_only else 'PASS',assertions=text.count('PASS '),normal_dos_return=True);shot('DOS');break
  assert shutil.disk_usage(out).free>512*1024**2
  assert sum(x.stat().st_size for x in out.iterdir() if x.is_file())<400*1024**2
 else:raise TimeoutError('8088 acceptance bound expired')
except BaseException as e:
 r['status']='FAIL' if proc is not None else 'BLOCKED'
 r['error']=repr(e);r['traceback']=traceback.format_exc()
 if ctl:
  try:shot('FAILURE')
  except Exception:pass
finally:
 if ctl:ctl.close()
 if g:g.close()
 stop_owned(proc)
 if watch:watch.cancel()
 if job:job.close()
 if host:host.close()
 r['owned_child_stopped']=proc is None or proc.poll() is not None
 if held:lease.seek(0);msvcrt.locking(lease.fileno(),msvcrt.LK_UNLCK,1)
 lease.close();r['lease_released']=held;r['inventory_after']=inventory();r['seconds']=round(time.monotonic()-start,1)
 (out/'RESULT.JSON').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True)
raise SystemExit(0 if r['status'] in ('PASS','DIAGNOSTIC_PASS') else 1)
