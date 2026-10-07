from pathlib import Path
import wave,numpy as np,json,hashlib,re,argparse
p=argparse.ArgumentParser();p.add_argument('root',type=Path);args=p.parse_args();out=[]
periods=[90,180,120,135,90,60];onsets=[0,.3,.45,.9,1.5,1.8];ends=onsets[1:]+[2.4]
for case in sorted(args.root.iterdir()):
 if not case.is_dir() or not (case/'RESULT.JSON').exists():continue
 r=json.loads((case/'RESULT.JSON').read_text());wav=next((case/'captures').glob('*.wav'))
 with wave.open(str(wav)) as f: rate=f.getframerate();a=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,f.getnchannels()).mean(1)
 edges=np.flatnonzero(abs(np.diff(a))>250);enabled=r['enabled'];start=(int(edges[0])+1)/rate if len(edges) else 0;span=(int(edges[-1])-int(edges[0]))/rate if len(edges) else 0
 def amp(hz,t):
  s=a[int((start+t-.03)*rate):int((start+t+.03)*rate)].copy();s-=s.mean();win=np.hanning(len(s));return float(abs(np.dot(s*win,np.exp(-2j*np.pi*hz*np.arange(len(s))/rate)))*2/win.sum())
 melody=[]; harmony=[]
 if enabled:
  for i,d in enumerate(periods):
   t=(onsets[i]+ends[i])/2;vals=[amp(3579545/(32*q),t) for q in [90,180,120,135,60]];winner=[90,180,120,135,60][int(np.argmax(vals))];melody.append({'note_period':d,'observed_peak_period':winner,'center_s':t,'amplitude':round(amp(3579545/(32*d),t),2)})
 if enabled:
  for period,t in [(269,.65),(285,2.1),(641,.65),(539,1.2),(719,2.55)]:harmony.append({'period':period,'center_s':t,'amplitude':round(amp(3579545/(32*period),t),2)})
 tail=a[-rate//3:];tail_rms=float(np.sqrt(np.mean((tail-tail.mean())**2)))
 lines=r['log'].splitlines();b=re.search(r'module=(\d+) window=(\d+) visible=(\d+)',lines[0]);e=re.search(r'module=(\d+) window=(\d+) visible=(\d+)',lines[1])
 checks={'exit':r['exit']==0,'observer':lines[-1]=='PASS','unloaded':e[1]=='0' and e[2]=='0','hidden':b[3]=='0','concurrent':not enabled or int(b[1])>0,'six_ordered_notes':all(x['note_period']==x['observed_peak_period'] and x['amplitude']>500 for x in melody) if enabled else bool(np.max(abs(a))==0),'harmony':all(x['amplitude']>100 for x in harmony) if enabled else True,'duration':2.65<span<2.9 if enabled else span==0,'silent_tail':tail_rms<10}
 result={'case':case.name,'checks':checks,'passed':all(checks.values()),'melody':melody,'harmony':harmony,'start_s':start,'audible_span_s':span,'tail_ac_rms':tail_rms,'wave_sha256':hashlib.sha256(wav.read_bytes()).hexdigest()};out.append(result);print(json.dumps(result))
(args.root/'AUDIO.JSON').write_text(json.dumps(out,indent=2));assert len(out)==4 and all(r['passed'] for r in out)
