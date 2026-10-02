from pathlib import Path
import wave,numpy as np,json,hashlib,re,argparse
p=argparse.ArgumentParser();p.add_argument('root',type=Path);args=p.parse_args();out=[]
for case in args.root.iterdir():
 if not case.is_dir() or not (case/'RESULT.JSON').exists():continue
 result=json.loads((case/'RESULT.JSON').read_text());wav=next((case/'captures').glob('*.wav'));f=wave.open(str(wav));a=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,2).mean(1);rate=f.getframerate();edges=np.flatnonzero(abs(np.diff(a))>250);amps=[]
 for hz in [3579545/(32*d) for d in [214,170,127]]:
  best=0
  for i in range(0,len(a)-rate//8,rate//16):
   s=a[i:i+rate//8].copy();s-=s.mean();win=np.hanning(len(s));v=abs(np.dot(s*win,np.exp(-2j*np.pi*hz*np.arange(len(s))/rate)))*2/win.sum();best=max(best,v)
  amps.append(round(float(best),2))
 enabled=result['enabled'];lines=result['log'].splitlines();begin=re.search(r'module=(\d+) window=(\d+) visible=(\d+)',lines[0]);end=re.search(r'module=(\d+) window=(\d+) visible=(\d+)',lines[1]);span=(int(edges[-1])-int(edges[0]))/rate if len(edges) else 0
 checks={'exited':result['exit']==0,'observer_passed':lines[-1]=='PASS','unloaded':end[1]=='0' and end[2]=='0','hidden':begin[3]=='0','startup_concurrent':not enabled or int(begin[1])>0,'tones_or_disabled_silence':all(x>500 for x in amps) if enabled else all(x==0 for x in amps),'short_sound':span<1.2 if enabled else span==0}
 r={'case':case.name,'checks':checks,'passed':all(checks.values()),'seconds':len(a)/rate,'tone_amplitudes_pcm16':amps,'audible_span_s':span,'wave_sha256':hashlib.sha256(wav.read_bytes()).hexdigest()};out.append(r);print(json.dumps(r))
(args.root/'AUDIO.JSON').write_text(json.dumps(out,indent=2));assert len(out)==4 and all(x['passed'] for x in out)
