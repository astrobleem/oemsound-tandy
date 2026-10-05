#!/usr/bin/env python3
"""Portable headless DOSBox-X smoke/full tests, without custom SDL hooks."""
import argparse,json,os,shutil,struct,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('runtime',type=Path);p.add_argument('output',type=Path);p.add_argument('--dosbox',type=Path,required=True);p.add_argument('--cycles',type=int,default=3000);p.add_argument('--case',choices=['preview','full','no-captions','bad-captions','bad-pcm'],default='preview');p.add_argument('--timeout',type=int,default=360);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True);guest=out/'GUEST';guest.mkdir()
for n in ['BUDTALK.EXE','WEEZER.WZV','WEEZER.WZM','WEEZER.CUE','WEEZER.LRC','DIALOG.PCM']:shutil.copyfile(a.runtime/n,guest/n)
if a.case=='no-captions':(guest/'WEEZER.LRC').unlink()
if a.case=='bad-captions':(guest/'WEEZER.LRC').write_bytes(b'bad caption file')
if a.case=='bad-pcm':
    f=guest/'DIALOG.PCM';f.write_bytes(f.read_bytes()[:-1])
# /P selects the built-in 18–42s preview; /L enables captions.
command='BUDTALK' if a.case=='full' else 'BUDTALK /B /P /L'
(guest/'START.BAT').write_bytes(('@echo off\r\n'+command+'\r\nECHO RETURNED > RETURNED.TXT\r\nEXIT\r\n').encode())
conf=f'''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=0
memsizekb=640
allow more than 640kb base memory=false
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed {a.cycles}
fpu=false
[mixer]
nosound=true
[midi]
mididevice=none
[dos]
xms=false
ems=false
umb=false
[autoexec]
mount C "{guest}"
C:
START.BAT
'''
(out/'RUN.CNF').write_text(conf)
env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'}
with (out/'HOST.LOG').open('wb') as f:
    proc=subprocess.Popen([str(a.dosbox.resolve()),'-nopromptfolder','-conf',str(out/'RUN.CNF')],stdout=f,stderr=subprocess.STDOUT,env=env)
    try:proc.wait(timeout=a.timeout)
    except subprocess.TimeoutExpired:proc.kill();proc.wait();raise SystemExit('DOSBox-X timed out; inspect HOST.LOG')
assert (guest/'RETURNED.TXT').exists(),'DOS did not return'
log=(guest/'BUDTALK.LOG').read_text();fields=dict(x.split('=',1) for x in log.splitlines() if '=' in x)
assert fields['build']=='BUDCAP04'
for key in ['speaker_bits_after_stop','sound_mask_after_stop']:assert fields[key]=='0',(key,fields[key])
assert fields['restored_mode']==fields['old_mode']
assert fields['error']==('61' if a.case=='bad-pcm' else '0')
if a.case in ['bad-captions','no-captions']:
    assert fields['caption_status']==('-1' if a.case=='bad-captions' else '0')
    assert fields['caption_cues']=='0' and fields['caption_updates']=='0'
if a.case=='preview':
    for k,v in {'play_start_ms':'18000','play_end_ms':'42000','speech_completed':'2','caption_enabled':'1','caption_cues':'84','caption_updates':'10','speech_guarded':'0','music_events_skipped':'0','unexpected_video_drops':'0'}.items():assert fields[k]==v,(k,fields[k])
if a.case=='full':
    for k,v in {'caption_updates':'84','speech_completed':'4','speech_guarded':'0','music_events_skipped':'0','unexpected_video_drops':'0'}.items():assert fields[k]==v,(k,fields[k])
result={'status':'PASS','case':a.case,'cycles':a.cycles,'log':fields,'scope':'Emulator test; not physical hardware or listening certification.'}
(out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
