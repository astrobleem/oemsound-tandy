"""Synthetic acceptance-gate regressions. No native or audible-quality claim."""
from pathlib import Path
import argparse,array,hashlib,json,subprocess,sys
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path,required=True)
a=p.parse_args();out=a.out.resolve();out.mkdir();validator=Path(__file__).with_name('FXAUDIO.PY')
rates=dict(psg=48000,speaker=49716);checks=[]
marks=dict(COLD_PLAY_REQUEST=0,PLAYING_BEGIN=2,PLAYING_END=14,REPAINT_BEFORE_PAUSE=14,PAUSED_BEGIN=15,PAUSED_END=16,RESUMED_BEGIN=17,RESUMED_END=19,REPAINT_BEFORE_STOP=19,STOPPED_BEGIN=20,STOPPED_END=21,WARM_PLAY_REQUEST=21,WARM_BEGIN=23,WARM_END=25)
for case in ('sustained_responsive','within_deadline_overlap','late_silence','short_zero_tail','silent_psg','startup_only','short_window','slow_wall_response','slow_guest_response','no_warm_onset'):
 d=out/case;d.mkdir();actions=[]
 for name,sec in marks.items():
  row=dict(name=name,wall=sec)
  row.update({n+'_frames':int(sec*r) for n,r in rates.items()});actions.append(row)
 byname={x['name']:x for x in actions}
 if case in ('within_deadline_overlap','late_silence','short_zero_tail'):
  sec=16.3 if case=='within_deadline_overlap' else 16.7 if case=='late_silence' else 16.1
  byname['PAUSED_END']['wall']=min(sec,16.4)
  for n,r in rates.items():byname['PAUSED_END'][n+'_frames']=int(sec*r)
 if case=='short_window':
  for n,r in rates.items():byname['PLAYING_END'][n+'_frames']=4*r
 if case=='slow_wall_response':byname['PAUSED_BEGIN']['wall']=17
 if case=='slow_guest_response':
  for n,r in rates.items():byname['REPAINT_BEFORE_STOP'][n+'_frames']=17*r
 for channel,rate in rates.items():
  samples=array.array('h')
  for tenth in range(260):
   active=10<=tenth<145 or 165<=tenth<195 or 220<=tenth<255
   if case in ('within_deadline_overlap','late_silence','short_zero_tail'):
    stop=161 if case=='late_silence' else 157
    active=10<=tenth<stop or 170<=tenth<195 or 220<=tenth<255
   if case=='silent_psg' and channel=='psg':active=False
   if case=='startup_only' and 70<=tenth<145:active=False
   if case=='no_warm_onset' and tenth>=210:active=False
   n=(tenth+1)*rate//10-tenth*rate//10
   samples.extend((1200 if i&8 else -1200) if active else 0 for i in range(n*2))
  if sys.byteorder!='little':samples.byteswap()
  (d/(channel+'.s16le')).write_bytes(samples.tobytes())
 (d/'RESULT.JSON').write_text(json.dumps(dict(status='PASS',normal_dos_return=True,capture_rates=rates,runtime=dict(test=True),actions=actions)))
 run=subprocess.run([sys.executable,str(validator),'--native',str(d)],capture_output=True,text=True,timeout=30)
 (d/'TEST.LOG').write_text(run.stdout+run.stderr)
 okay=(run.returncode==0)==(case in ('sustained_responsive','within_deadline_overlap'));checks.append(dict(case=case,passed=okay));assert okay,case
result=dict(status='PASS',scope='Synthetic audio/onset gate fixtures only; no native evidence',validator_sha256=hashlib.sha256(validator.read_bytes()).hexdigest(),checks=checks)
(out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
