from pathlib import Path
import argparse,json,wave,numpy as np,hashlib
p=argparse.ArgumentParser();p.add_argument('root',type=Path);a=p.parse_args();out=[]
for case in sorted(a.root.iterdir()):
 if not (case/'RESULT.JSON').exists():continue
 r=json.loads((case/'RESULT.JSON').read_text());wav=next((case/'captures').glob('*.wav'))
 with wave.open(str(wav)) as f:rate=f.getframerate();v=np.frombuffer(f.readframes(f.getnframes()),'<i2').reshape(-1,f.getnchannels()).mean(1)
 amps=[]
 for period in [240,285,359]:
  hz=3579545/(32*period);best=0
  for i in range(0,len(v)-rate//8,rate//16):
   s=v[i:i+rate//8].copy();s-=s.mean();win=np.hanning(len(s));best=max(best,float(abs(np.dot(s*win,np.exp(-2j*np.pi*hz*np.arange(len(s))/rate)))*2/win.sum()))
  amps.append(round(best,2))
 edges=np.flatnonzero(abs(np.diff(v))>250);span=(int(edges[-1])-int(edges[0]))/rate if len(edges) else 0
 texit=r['TEXIT.LOG'] or '';obs=r['EXITOBS.LOG'] or '';kind=r['case'];sound=kind not in ['b','d','c'];checks={'process_exit':r['exit']==0,'returned_DOS':r['EXITDOS.LOG']=='Returned to DOS\n','observer_no_failure':'FAIL' not in obs,'audio_or_busy_silence':all(x>500 for x in amps) if sound else bool(np.max(abs(v))==0),'bounded_phrase':.8<span<1.15 if sound else span==0}
 if kind=='a':checks['natural_exit']='MUTED WINDOW GONE\nCALL ExitWindows\n' in texit and 'ENDSESSION TRUE' in obs and 'FIXTURE' not in obs
 elif kind=='k':checks['prompt_accept_exit']='SEND Tab Return accept' in obs and 'ENDSESSION TRUE' in obs and 'FIXTURE' not in obs
 elif kind=='v':checks['veto_respected']='VETO OR FAILURE' in texit and 'QUERY veto FALSE' in obs and 'ENDSESSION FALSE' in obs and 'PASS unloaded' in obs
 elif kind=='p':checks['preview_no_exit']='NO EXIT REQUEST' in texit and 'CALL ExitWindows' not in texit and 'PASS no exit request' in obs
 elif kind=='c':checks['confirmation_cancel']=not texit and 'SEND default Return cancel' in obs and 'PASS unloaded' in obs and 'PASS no exit request' in obs
 else:checks['busy_no_exit']=texit=='BUSY: NO PSG OR EXIT\n' and 'PASS no exit request' in obs
 result={'case':case.name,'passed':all(checks.values()),'checks':checks,'tone_amplitudes':amps,'audible_span_s':span,'wave_sha256':hashlib.sha256(wav.read_bytes()).hexdigest()};out.append(result);print(json.dumps(result))
(a.root/'AUDIO.JSON').write_text(json.dumps(out,indent=2));assert out and all(x['passed'] for x in out)
