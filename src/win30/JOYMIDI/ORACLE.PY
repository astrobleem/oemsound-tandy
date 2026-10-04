#!/usr/bin/env python3
"""Independent explicit native event-sequence oracle. Supply RESULT.JSON files."""
import json,re,sys
expected=[(144,66,72),(128,66,0),(144,72,127),(128,72,0),(144,60,16),(128,60,0),(144,66,72),(128,66,0),(176,123,0),(176,120,0)]+[(144,66,72),(128,66,0),(176,123,0),(176,120,0)]*3
results=[]
for filename in sys.argv[1:]:
 r=json.load(open(filename));log=r['logs']['JOYMIDI.LOG'];got=[tuple(map(int,m)) for m in re.findall(r'^EVENT (\d+) (\d+) (\d+)$',log,re.M)]
 want=expected if r['arg'] in ('/test','/ownertest') else [(144,66,72),(128,66,0),(176,123,0),(176,120,0)] if r['arg']=='/busytest' else []
 assert r['passed'] and 'TOTAL_FAILURES=0' in log
 assert got==want,(filename,got,want)
 if r['arg']=='/ownertest':assert 'TOTAL_FAILURES got=0 want=0' in r['logs']['MIDIINTR.LOG']
 results.append({'test':r['arg'],'app_sha256':r['app_sha256'],'expected_messages':len(want),'actual_messages':got,'exact_match':True})
print(json.dumps(results,indent=2))
