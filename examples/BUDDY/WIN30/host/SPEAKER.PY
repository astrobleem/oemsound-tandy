"""Reduce the monophonic arrangement using Chad's listening feedback."""
from pathlib import Path
import json,sys,collections,argparse
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'host'))
from ARRANGE import BeatMap,encode
s=json.loads((ROOT/'score/buddy_holly_score.json').read_text())
bm=BeatMap(json.loads((ROOT/'sync/buddy_holly_beatmap.json').read_text()))
notes=[];prev=None;dropped=collections.Counter()
for e in s['lead_sustained_events']:
 at=e['absolute_beat'];pitch=e['midi'];duration=e['duration']
 if pitch is None:prev=None;continue
 if duration<.5:dropped['sixteenth passing notes']+=1;continue
 if prev and pitch==prev['midi'] and at==prev['absolute_beat']+prev['duration'] and at%2:
  dropped['weak repeated attacks']+=1;prev=e;continue
 # Deliberate 110-250 ms breath after every attack; never fill source rests.
 gate=min(1.0,max(.25,duration*.58))
 for a,b in bm.spans(at,gate):notes.append(dict(a=a,b=b,n=pitch-25,priority=1,role='short low melody',beat=at))
 prev=e
prev=None
for e in s['bass_events']:
 if e['source']!='notated bass override':continue
 at=e['absolute_beat'];pitch=e['midi']
 if pitch is None:continue
 if prev and pitch==prev['midi'] and at%2:
  dropped['repeated bass pulses']+=1;prev=e;continue
 gate=min(.75,max(.25,e['duration']*.58))
 for a,b in bm.spans(at,gate):notes.append(dict(a=a,b=b,n=pitch-1,priority=2,role='spaced written riff',beat=at))
 prev=e
times=sorted({0,bm.duration}|{t for e in notes for t in (e['a'],e['b'])});states=[];active=[];last=-1
for t in times:
 available=[e for e in notes if e['a']<=t<e['b']]
 e=max(available,key=lambda x:(x['priority'],x['a'])) if available else None;n=e['n'] if e else 0
 if n!=last:
  states.append((t,(0,n,0,0),(0,100 if n else 0,0,0),2 if n else 0));active.append(dict(time_ms=t,note=n,role=e['role'] if e else 'rest'));last=n
raw=encode(states,bm.duration)
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'SPEAKER.WZM').write_bytes(raw)
(a.out/'SPEAKER.JSON').write_text(json.dumps(active,indent=2)+'\n')
print('Prepared',len(states),'monophonic states with musical rests.')
