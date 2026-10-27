"""Focused private 640 KB HX/Win3.0 fixture. Never changes the seed image."""
from pathlib import Path
import argparse,base64,hashlib,json,msvcrt,os,re,shutil,socket,struct,subprocess,sys,time,zipfile
sys.path[:0]=[r'C:\Users\chad\Documents\Codex\2026-10-14\task-16\deps-local',r'E:\DOSMCP\mcp-server']
from pyfatfs.PyFatFS import PyFatFS
from src.gdbclient import GdbClient
from src.headless_input import HeadlessCtl,hold_modifier_and_tap,SC_ALT
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--build',type=Path,required=True)
p.add_argument('--seed',type=Path,required=True);p.add_argument('--bound',type=int,default=360);a=p.parse_args()
root=Path(__file__).resolve().parents[3];out=a.out.resolve();out.mkdir();build=a.build.resolve()
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
record=dict(complete=False,fixture='Tandy HX 8088 7.16 MHz, 640KB, Win3.0 real mode, qualified fixed 320x200 display',exe=pin((build/'EFFECTS.EXE').read_bytes()),launcher=pin((build/'FXSTART.EXE').read_bytes()))
shutil.copyfile(a.seed/'DISK.IMG',out/'DISK.IMG');shutil.copyfile(a.seed/'86box.cfg',out/'86box.cfg')
record['seed']=pin((a.seed/'DISK.IMG').read_bytes());offset=8704
with zipfile.ZipFile(root/'artifacts/win30/TWXT2.ZIP') as z:
 runtime={name:z.read('TWXT2/'+name) for name in ('RAWCLK.DLL','SOUND.DRV','TBRIDGE.EXE','RESERVE.COM')}
record['dependencies']={n:pin(b) for n,b in runtime.items()}
with PyFatFS(str(out/'DISK.IMG'),offset=offset) as fs:
 def put(n,b):
  if fs.exists(n):fs.remove(n)
  fs.writebytes(n,b)
 if not fs.exists('MMLAB'):fs.makedir('MMLAB')
 for n,b in runtime.items():put('MMLAB/'+n,b)
 for n in ('EFFECTS.EXE','FXSTART.EXE'):put('MMLAB/'+n,(build/n).read_bytes())
 put('MMLAB/RAWGUI.EXE',(build/'EFFECTS.EXE').read_bytes())
 put('WINDOWS/SYSTEM/SOUND.DRV',runtime['SOUND.DRV'])
 if fs.exists('WINDOWS/SOUND.DRV'):fs.remove('WINDOWS/SOUND.DRV')
 put('CONFIG.SYS',b'FILES=40\r\nBUFFERS=8\r\n')
 win=fs.readbytes('WINDOWS/WIN.INI').decode('latin1').replace('\r','');win=re.sub(r'(?im)^(load|run)=.*$',lambda m:m[1]+'=',win)
 put('WINDOWS/WIN.INI',win.replace('\n','\r\n').encode('latin1'))
 ini=fs.readbytes('WINDOWS/SYSTEM.INI').decode('latin1');assert 'display.drv=TR53216.DRV' in ini
 bat='''@ECHO OFF
PATH C:\\;C:\\WINDOWS;C:\\DOS
SET TEMP=C:\\WINDOWS\\TEMP
C:
CD \\MMLAB
ECHO TEST>C:\\PHASE.TXT
FXSTART C:\\WINDOWS /TEST > C:\\START.LOG
IF ERRORLEVEL 1 GOTO FAILED
ECHO VISUAL>C:\\PHASE.TXT
FXSTART C:\\WINDOWS > C:\\VISUAL.LOG
IF ERRORLEVEL 1 GOTO FAILED
ECHO RETURNED>C:\\DONE.TXT
GOTO END
:FAILED
ECHO FAILED>C:\\DONE.TXT
:END
'''
 put('AUTOEXEC.BAT',bat.replace('\n','\r\n').encode('ascii'))
 for n in ('EFFECTS.LOG','DONE.TXT','PHASE.TXT','MMLAB/NATIVE.FXP','MMLAB/NATIVE.H'):
  if fs.exists(n):fs.remove(n)
exe=Path(r'C:\Users\chad\Documents\Codex\2026-10-16\task-10\EMU-STOS1\fixed\86Box.exe')
lease=proc=g=ctl=log=None;held=False;start=time.monotonic();last=None;visual=False;shots=0
try:
 lease=Path(r'C:\Users\chad\Documents\Codex\2026-10-06\tandy-emulator.lock').open('rb');assert lease.read(1),'Lease empty; no permission to initialize'
 lease.seek(0);msvcrt.locking(lease.fileno(),msvcrt.LK_NBLCK,1);held=True;record['lease_acquired']=True
 inv=subprocess.check_output(['powershell.exe','-NoProfile','-Command','@(Get-Process -Name dosbox*,86Box,TRACEIO,FIXBOX,pcem,tandybox -ErrorAction SilentlyContinue).Count'],text=True).strip();assert inv=='0','Another emulator is active'
 for port in (12345,12346):
  with socket.socket() as s:assert s.connect_ex(('127.0.0.1',port))!=0,'Control port occupied'
 env=dict(os.environ,QT_QPA_PLATFORM='offscreen',PATH=r'C:\msys64\ucrt64\bin;'+os.environ.get('PATH',''))
 si=subprocess.STARTUPINFO();si.dwFlags|=subprocess.STARTF_USESHOWWINDOW;si.wShowWindow=0
 log=(out/'HOST.LOG').open('wb');proc=subprocess.Popen([str(exe),'-P',str(out),'-R',r'E:\x86box\roms','-N'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,startupinfo=si,creationflags=subprocess.CREATE_NO_WINDOW)
 record['pid']=proc.pid;print('Owned native child',proc.pid,flush=True)
 for _ in range(100):
  assert proc.poll() is None,'Emulator exited'
  try:g=GdbClient(timeout=3);g.connect();ctl=HeadlessCtl('127.0.0.1',12346);break
  except (OSError,ConnectionError):
   if g:g.close();g=None
   time.sleep(.2)
 assert g and ctl,'Control unavailable'
 def rest(seconds):g.run_while(lambda:None,settle_seconds=seconds,start_delay=.01)
 def key(sc):g.run_while(lambda:ctl.tap(sc),settle_seconds=1,start_delay=.01)
 def shot(name):(out/name).write_bytes(base64.b64decode(ctl.capture_frame_png_b64()))
 while time.monotonic()-start<a.bound:
  assert proc.poll() is None;rest(.4);mode=g.read_memory(0x449,1)[0]
  with PyFatFS(str(out/'DISK.IMG'),offset=offset,read_only=True) as fs:
   phase=fs.readtext('PHASE.TXT').strip() if fs.exists('PHASE.TXT') else 'boot'
   text=fs.readtext('EFFECTS.LOG') if fs.exists('EFFECTS.LOG') else ''
   done=fs.readtext('DONE.TXT').strip() if fs.exists('DONE.TXT') else ''
   launch=fs.readtext('START.LOG') if fs.exists('START.LOG') else ''
  sig=phase,mode,len(text)
  if sig!=last:print(round(time.monotonic()-start,1),sig,text[-180:],flush=True);last=sig
  (out/'EFFECTS.LOG').write_text(text);(out/'START.LOG').write_text(launch)
  if 'FAIL ' in text or re.search(r'TOTAL_FAILURES=[1-9]',text):raise RuntimeError('Native assertions failed')
  if mode==9 and shots<3:shot('FRAME%u.PNG'%shots);shots+=1
  if phase=='VISUAL' and mode==9 and not visual:
   rest(30);shot('EDITOR.PNG');key(0x3f);rest(.5);record['playing_pit_gate']=g.in_port(0x61)&3;key(0x01);rest(1)
   # Tab navigation selects first preset, Space activates a native button.
   key(0x0f);key(0x39);rest(2);key(0x01);rest(1);shot('KEYBOARD.PNG')
   # Six tabs from first preset reach Play; target is later. Use main
   # window arrows to reach target, then Right selects speaker.
   for _ in range(11):key(0x50)
   key(0x4d);rest(.2);shot('SPEAKER.PNG');record['speaker_gate']=g.in_port(0x61)&3
   key(0x40);rest(.3);record['paused_speaker_gate']=g.in_port(0x61)&3;shot('PAUSED.PNG');key(0x01);rest(1);record['stopped_speaker_gate']=g.in_port(0x61)&3
   g.run_while(lambda:hold_modifier_and_tap(ctl,SC_ALT,0x3e),settle_seconds=2,start_delay=.01);key(0x15);rest(2)
   # Exit Windows normally after the app closes.
   g.run_while(lambda:hold_modifier_and_tap(ctl,SC_ALT,0x3e),settle_seconds=2,start_delay=.01);key(0x1c);visual=True
  if done:
   assert done=='RETURNED',launch
   assert mode==3 and 'TOTAL_FAILURES=0' in text
   with PyFatFS(str(out/'DISK.IMG'),offset=offset,read_only=True) as fs:
    for n in ('MMLAB/NATIVE.FXP','MMLAB/NATIVE.H'):(out/Path(n).name).write_bytes(fs.readbytes(n))
   record.update(complete=True,dos_return=True,assertions=text.count('PASS '),visual=visual);shot('DOS.PNG');break
 else:raise TimeoutError('Native bound exhausted')
except BaseException as e:
 record['error']=repr(e)
 if ctl:
  try:shot('STOP.PNG')
  except Exception:pass
finally:
 if ctl:ctl.close()
 if g:g.close()
 if proc and proc.poll() is None:proc.kill();proc.wait(timeout=10)
 if log:log.close()
 record['owned_child_stopped']=proc is None or proc.poll() is not None
 if held:lease.seek(0);msvcrt.locking(lease.fileno(),msvcrt.LK_UNLCK,1);record['lease_released']=True
 if lease:lease.close()
 record['seconds']=round(time.monotonic()-start,1);(out/'RESULT.JSON').write_text(json.dumps(record,indent=2));print(json.dumps(record),flush=True)
raise SystemExit(0 if record['complete'] else 1)
