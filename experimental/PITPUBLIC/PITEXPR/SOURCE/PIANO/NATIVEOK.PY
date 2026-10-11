#!/usr/bin/env python3
"""Exact native event oracle, independent of application-generated counts."""
import argparse,json,re
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('results',nargs='+',type=Path,help='TESTWIN.PY RESULT.JSON files')
args=parser.parse_args()
on=lambda n:(144,n,100)
off=lambda n:(128,n,0)
expected=[on(60),off(60),on(63),off(63),on(60),off(60),on(60),on(64),on(67),on(72),off(60),off(64),off(67),off(72),on(60),off(60),on(72),off(72),on(60),off(60),on(60),off(60),on(60),off(60),on(60),off(60),on(60),off(60),on(61),off(61),on(60),off(60),on(96),off(96),on(48),off(48)]
demo=[e for n in [60,64,67,72,71,67,64,60] for e in (on(n),off(n))]
rows=[]
for path in args.results:
    result=json.loads(path.read_text())
    mode=result['arg']
    if mode in ('/test','/ownertest'):
        want=expected
    elif mode=='/demo':
        want=demo
    elif mode=='/busytest':
        want=[on(60),off(60)]
    elif mode in ('/missing','/sendtest'):
        want=[]
    else:
        raise ValueError('No expected sequence for mode '+mode)
    log=result['logs']['MINIPIAN.LOG']
    got=[tuple(map(int,x)) for x in re.findall(
        r'^EVENT (\d+) (\d+) (\d+)$',log,re.M)]
    assert result['passed'],mode
    assert 'TOTAL_FAILURES=0' in log,mode
    assert 'RETURN.LOG' in result['logs'],mode
    assert got==want,(mode,got,want)
    if mode=='/ownertest':
        assert 'TOTAL_FAILURES got=0 want=0' in result['logs'].get(
            'MIDIINTR.LOG',''),mode
    rows.append({'mode':mode,'passed':True,'exact_events':len(want),
                 'app_sha256':result['app_sha256']})
print(json.dumps(rows,indent=2))
