"""Check bounded emulator PSG/speaker recordings against native action marks.
Developer evidence only; recordings are never a consumer playback input.
"""
from pathlib import Path
import argparse,array,hashlib,json,math,sys

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--native',type=Path,required=True)
p.add_argument('--diagnostic',action='store_true',help='Analyze diagnostic capture without qualifying packaging')
a=p.parse_args();root=a.native.resolve()
native=json.loads((root/'RESULT.JSON').read_bytes())
assert native['status']==('DIAGNOSTIC_PASS' if a.diagnostic else 'PASS') and native['normal_dos_return']
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

result['play_onset']={}
for channel in ('psg','speaker'):
    path=root/(channel+'.s16le');rate=rates[channel]
    assert path.stat().st_size<160*1024**2 and path.stat().st_size%4==0
    result['recordings'][path.name]=dict(bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
    result['play_onset'][channel]={}
    for mode,request in (('PLAYING','COLD_PLAY_REQUEST'),('WARM','WARM_PLAY_REQUEST')):
        begin=actions[request][channel+'_frames'];end=actions[mode+'_BEGIN'][channel+'_frames']
        assert end>begin and end*4<=path.stat().st_size
        with path.open('rb') as f:f.seek(begin*4);samples=array.array('h',f.read((end-begin)*4))
        if sys.byteorder!='little':samples.byteswap()
        first=next((i//2 for i,v in enumerate(samples) if abs(v)>64),None)
        assert first is not None,'No audible onset after '+request
        result['play_onset'][channel]['cold' if mode=='PLAYING' else 'warm']=dict(first_active_frame=first,request_to_onset_capture_seconds=round(first/rate,3),note='Effects groove begins with PSG and ordinary speaker notes. Capture timing, not exact host key-handler latency.')
    for mode in ('PLAYING','PAUSED','RESUMED','STOPPED','WARM'):
        begin=actions[mode+'_BEGIN'][channel+'_frames'];end=actions[mode+'_END'][channel+'_frames']
        assert end>begin and end*4<=path.stat().st_size
        if mode=='PLAYING':assert end-begin>=10*rate,'Playback capture does not exceed startup buffer'
        portions=3 if mode=='PLAYING' else 1
        for part in range(portions):
            s=stats(path,begin+(end-begin)*part//portions,begin+(end-begin)*(part+1)//portions)
            active=mode in ('PLAYING','RESUMED','WARM')
            okay=s['rms']>50 if active else s['rms']<=4 and s['peak']<=64
            result['segments'].append(dict(channel=channel,mode=mode,part=part+1,pass_check=okay,**s))
        if mode=='PLAYING':
            # Starts >=9s after BEGIN, beyond512/60=8.533s of startup data.
            s=stats(path,end-rate,end)
            result['segments'].append(dict(channel=channel,mode='PLAYING_TAIL',part=1,pass_check=s['rms']>50,**s))
result['repaint_transport_bounds']={}
for mode,request in (('PAUSED','REPAINT_BEFORE_PAUSE'),('STOPPED','REPAINT_BEFORE_STOP')):
    elapsed=actions[mode+'_BEGIN']['wall']-actions[request]['wall']
    assert 0<elapsed<=2.5,'Transport response capture exceeded2.5s after repaint request'
    observed={channel:actions[mode+'_BEGIN'][channel+'_frames']-actions[request][channel+'_frames'] for channel in rates}
    assert all(0<observed[channel]<=2*rates[channel] for channel in rates),'Repaint transport silence capture exceeded2 emulated seconds'
    result['repaint_transport_bounds'][mode]=dict(request_to_window_seconds=round(elapsed,3),request_to_window_frames=observed,silent_window_checks=all(s['pass_check'] for s in result['segments'] if s['mode']==mode),note='Measured silent window trims at most0.25s for capture buffering')
result['status']='PASS' if all(s['pass_check'] for s in result['segments']) else 'FAIL'
if a.diagnostic and result['status']=='PASS':result['status']='DIAGNOSTIC_PASS'
(root/'AUDIO.JSON').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
raise SystemExit(0 if result['status'] in ('PASS','DIAGNOSTIC_PASS') else 1)
