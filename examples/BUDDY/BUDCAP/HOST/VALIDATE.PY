#!/usr/bin/env python3
"""Read-only exact-release verification; accepts original ZIP or extracted folder."""
import argparse, hashlib, json, struct, zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('package',type=Path);a=p.parse_args()
if a.package.is_file():
    assert hashlib.sha256(a.package.read_bytes()).hexdigest()=='0ea79bf57992e408115c9b4daeb36daee224a0ea56a36dd0b1fcdc2d0ef8b75d', 'Release ZIP hash mismatch'
    with zipfile.ZipFile(a.package) as z: files={n.removeprefix('BUDCAP/'):z.read(n) for n in z.namelist() if not n.endswith('/')}
    assert len(files)==33
else: files={str(f.relative_to(a.package)):f.read_bytes() for f in a.package.rglob('*') if f.is_file()}
checks=0
for line in files['SHA256.TXT'].decode().splitlines():
    digest,name=line.split(None,1);assert hashlib.sha256(files[name.replace(chr(92),'/')]).hexdigest()==digest,name;checks+=1
v=files['WEEZER.WZV'];magic,w,h,fps,hs,n,d,fs=struct.unpack_from('<4sHHHHIII',v)
assert (magic,w,h,fps,hs,n,d,fs)==(b'WZV2',256,160,4,24,966,241400,20480)
assert len(v)==24+n*fs
m=files['WEEZER.WZM'];magic,hs,rs,ne,dm,res=struct.unpack_from('<4sHHIII',m)
assert (magic,hs,rs,dm,res)==(b'WZM1',20,14,d,0) and len(m)==20+14*ne
last=-1
for row in struct.iter_unpack('<I4B4BBB',m[20:]):
    t=row[0];assert last<t<=d and not row[9]&240 and row[10]==0;last=t
assert len(files['WEEZER.CUE'])==16
magic,dc,start,end=struct.unpack('<4sIII',files['WEEZER.CUE']);assert magic==b'WZC1' and dc==d and 0<=start<end<=d
b=files['DIALOG.PCM'];assert struct.unpack_from('<4sHH',b)==(b'SPC1',4,0)
clips=json.loads(files['CLIPS.JSON']);offset=8;total=0
for clip in clips:
    start,end,size,res=struct.unpack_from('<IIHH',b,offset);offset+=12;raw=b[offset:offset+size];offset+=size;total+=size
    assert (start,end,size,res)==(clip['start_ms'],clip['end_ms'],clip['samples'],0)
    assert len(raw)==size==(end-start)*6 and 1<=min(raw)<=max(raw)<=72
    assert hashlib.sha256(raw).hexdigest()==clip['sha256']
assert offset==len(b) and total==63600
last=-1;cues=0
for line in files['WEEZER.LRC'].decode('ascii').splitlines():
    if not line or line.startswith('#'):continue
    when,text=line.split('|',1);when=int(when);assert last<when<=d and len(text)<=52 and all(32<=ord(c)<=126 for c in text);last=when;cues+=1
assert cues==84
for name in ['VERIFY/R240.LOG','VERIFY/R3000.LOG']:
    log=dict(line.split('=',1) for line in files[name].decode().splitlines() if '=' in line)
    for k,value in {'build':'BUDCAP04','error':'0','speech_completed':'4','speech_guarded':'0','speech_late':'0','music_events_skipped':'0','unexpected_video_drops':'0','caption_cues':'84','caption_updates':'84','speaker_bits_after_stop':'0','sound_mask_after_stop':'0','restored_mode':'3'}.items():assert log[k]==value,(name,k)
    assert int(log['frames'])+int(log['speech_held_frames'])==966
print(json.dumps({'status':'PASS','release_hashes_checked':checks,'video_frames':n,'music_records':ne,'caption_cues':cues,'speech_samples':total,'archived_full_run_logs_checked':2},indent=2))
