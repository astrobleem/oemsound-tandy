"""Check native logs and captured PCM; does not claim human intelligibility."""
from pathlib import Path
import json,sys,wave
import numpy as np
root=Path(sys.argv[1]); results=[]
for case in sorted(root.iterdir()):
 if not case.is_dir() or not (case/'RESULT.JSON').exists(): continue
 r=json.loads((case/'RESULT.JSON').read_text()); rows=[x.split() for x in r['trace'].splitlines()]
 values=[int(x[2]) for x in rows if x[1]=='OUT'];frames=[int(x[2]) for x in rows if x[1]=='FRAME']
 mode=int(r['arg']); expected={1:[1,2,3,4,5,0],6:[6,7,8,6,9,0],7:[10,2,11,0],8:[8,4,5,0]}
 checks={'exit':r['exit']==0,'no_late_frames':not any(x[1]=='LATE' for x in rows),'all_four_muted':values[-4:]==[159,191,223,255] if values else mode==9,'input_bound':all(int(x[2])<=16 for x in rows if x[1]=='INPUT')}
 if mode in expected: checks['sequence']=frames==expected[mode];checks['completed']=any(x[1]=='DONE' for x in rows)
 if mode==3:checks['repeat_restart']=sum(x[1]=='START' for x in rows)==2 and frames[-6:]==expected[1]
 if mode in [4,9]:checks['unknown_rejected']=any(x[1]=='UNKNOWN' for x in rows)
 if mode in [2,5]:checks['interrupted']=0<len(frames)<6 and not any(x[1]=='DONE' for x in rows)
 wav=next((case/'captures').glob('*.wav'))
 with wave.open(str(wav)) as f:
  rate=f.getframerate(); a=np.frombuffer(f.readframes(f.getnframes()),'<i2');channels=f.getnchannels()
 a=a.reshape(-1,channels);aud=np.max(np.abs(a.astype('int32')),axis=1);nz=np.flatnonzero(aud>100)
 checks['audio_tail_silent']=bool(np.max(np.abs(np.diff(a[-rate//2:,0].astype('int32'))))<=1)
 checks['audio_present_or_rejected']=bool(len(nz)>0) if mode!=9 else bool(len(nz)==0)
 checks['no_pcm_clipping']=bool(np.max(aud)<32767)
 item={'case':case.name,'checks':checks,'passed':all(checks.values()),'frames':frames,'peak_pcm16':int(aud.max()),'tail_peak_pcm16':int(aud[-rate//2:].max()),'audible_seconds':round((int(nz[-1])-int(nz[0]))/rate,3) if len(nz) else 0}
 results.append(item);print(json.dumps(item))
(root/'CHECKS.JSON').write_text(json.dumps(results,indent=2)+'\n')
assert len(results)==13 and all(x['passed'] for x in results)
