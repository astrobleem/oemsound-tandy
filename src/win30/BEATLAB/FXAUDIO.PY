"""Check bounded emulator PSG/speaker recordings against native action marks.
Developer evidence only; recordings are never a consumer playback input.
"""
from pathlib import Path
import argparse,array,hashlib,json,math,sys

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--native',type=Path,required=True)
a=p.parse_args();root=a.native.resolve()
native=json.loads((root/'RESULT.JSON').read_bytes())
assert native['status']=='PASS' and native['normal_dos_return']
rates=dict(psg=48000,speaker=49716)
assert native['capture_rates']==rates
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
actions={row['name']:row for row in native['actions']}
result=dict(status='FAIL',runtime=native['runtime'],capture_rates=rates,validator=pin(Path(__file__).read_bytes()),native_result=pin((root/'RESULT.JSON').read_bytes()),scope='Emulated hardware recordings; no physical-hardware or subjective sound-quality claim',segments=[],recordings={})

def stats(path,begin,end):
    # Stereo signed16 captures. Trim capture-buffer boundaries and transients.
    trim=min(12000,(end-begin)//8);begin+=trim;end-=trim
    assert end-begin>=8000,'Audio segment too short'
    with path.open('rb') as f:f.seek(begin*4);data=f.read((end-begin)*4)
    samples=array.array('h',data)
    if sys.byteorder!='little':samples.byteswap()
    n=len(samples);mean=sum(samples)/n
    rms=math.sqrt(sum((v-mean)**2 for v in samples)/n)
    return dict(frames=end-begin,peak=max(abs(v) for v in samples),rms=round(rms,3),mean=round(mean,3))

for channel in ('psg','speaker'):
    path=root/(channel+'.s16le');rate=rates[channel]
    assert path.stat().st_size<160*1024**2 and path.stat().st_size%4==0
    result['recordings'][path.name]=dict(bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    for mode in ('PLAYING','PAUSED','RESUMED','STOPPED'):
        begin=actions[mode+'_BEGIN'][channel+'_frames'];end=actions[mode+'_END'][channel+'_frames']
        assert end>begin and end*4<=path.stat().st_size
        if mode=='PLAYING':assert end-begin>=10*rate,'Playback capture does not exceed startup buffer'
        portions=3 if mode=='PLAYING' else 1
        for part in range(portions):
            s=stats(path,begin+(end-begin)*part//portions,begin+(end-begin)*(part+1)//portions)
            active=mode in ('PLAYING','RESUMED')
            okay=s['rms']>50 if active else s['rms']<=4 and s['peak']<=64
            result['segments'].append(dict(channel=channel,mode=mode,part=part+1,pass_check=okay,**s))
        if mode=='PLAYING':
            # Starts >=9s after BEGIN, beyond512/60=8.533s of startup data.
            s=stats(path,end-rate,end)
            result['segments'].append(dict(channel=channel,mode='PLAYING_TAIL',part=1,pass_check=s['rms']>50,**s))
result['status']='PASS' if all(s['pass_check'] for s in result['segments']) else 'FAIL'
(root/'AUDIO.JSON').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(0 if result['status']=='PASS' else 1)
