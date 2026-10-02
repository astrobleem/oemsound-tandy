from pathlib import Path
import re,json,wave,hashlib
import numpy as np
import argparse
p=argparse.ArgumentParser();p.add_argument('root',type=Path);root=p.parse_args().root.resolve();out=[]
for d in sorted(root.iterdir()):
 if not d.is_dir() or not (d/'RESULT.JSON').exists():continue
 r=json.loads((d/'RESULT.JSON').read_text());ls=r['trace'].splitlines(); head=ls[0].split();tick=ls[1].split();irq=ls[2].split();events=[(int(x.split()[0]),int(x.split()[1],16)) for x in ls[3:]];bs=[v for t,v in events];mode=int(head[1]); tone=[0x8b,0x1a,0x94]+([0xa3,0x15,0xb4,0xcd,0x11,0xd4] if mode==1 else []);expected=[0x9f,0xbf,0xdf,0xff]*2+tone+[0x9f,0xbf,0xdf,0xff]*(3 if mode==3 else 4)
 checks={'exit0':r['exit']==0,'mode':mode=={'test':1,'stoptest':2,'closetest':3}[r['arg']],'no_trace_overflow':head[3]=='0','exact_port_trace':bs==expected,'irq8_unchanged':irq[1]==irq[2],'irq1c_unchanged':irq[4]==irq[5],'clock_delta_error_under_tick':abs((int(tick[2])-int(tick[1]))-(int(tick[5])-int(tick[4]))*54.9254)<55}
 wav=list((d/'captures').glob('*.wav'))[0];f=wave.open(str(wav)); rate=f.getframerate();a=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,f.getnchannels()).mean(axis=1)
 # Maximum narrow-band sinusoidal amplitude at each expected fundamental,
 # across overlapping quarter-second windows. Removes DC/filter tail.
 freqs=[3579545/(32*x) for x in [427,339,285]];amps=[]
 for hz in freqs:
  best=0
  for n in range(0,len(a)-rate//4,rate//8):
   s=a[n:n+rate//4];s=s-s.mean();win=np.hanning(len(s));mag=abs(np.dot(s*win,np.exp(-2j*np.pi*hz*np.arange(len(s))/rate)))*2/win.sum();best=max(best,mag)
  amps.append(round(float(best),2))
 checks['audible_tone_bands']=all(x>500 for x in amps) if mode==1 else amps[0]>500 and amps[1]<100 and amps[2]<100
 result={'case':d.name,'checks':checks,'passed':all(checks.values()),'tone_amplitudes_pcm16':amps,'duration_seconds':len(a)/rate,'trace_sha256':hashlib.sha256(r['trace'].encode()).hexdigest(),'wave_sha256':hashlib.sha256(wav.read_bytes()).hexdigest(),'wave_file':str(wav),'clock_delta_ms':int(tick[2])-int(tick[1]),'bios_ticks_elapsed':int(tick[5])-int(tick[4])};out.append(result)
(root/'VERIFY.JSON').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
assert len(out)==7 and all(x['passed'] for x in out)
