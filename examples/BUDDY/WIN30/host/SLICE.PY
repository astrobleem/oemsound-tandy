#!/usr/bin/env python3
"""Make a frame-aligned bounded excerpt from existing WZV2/WZM1."""
import argparse,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('output',type=Path);p.add_argument('--start',type=int,required=True);p.add_argument('--duration',type=int,required=True);a=p.parse_args()
v=(a.input/'WEEZER.WZV').read_bytes();h=list(struct.unpack('<4sHHHHIII',v[:24]));step=1000//h[3]
assert h[0]==b'WZV2' and h[4]==24 and 1000%h[3]==0
assert a.start>=0 and a.duration>0 and a.start%step==0 and a.duration%step==0 and a.start+a.duration<=h[6]
start=a.start//step;count=a.duration//step;h[5]=count;h[6]=a.duration
a.output.mkdir(parents=True,exist_ok=True);(a.output/'WEEZER.WZV').write_bytes(struct.pack('<4sHHHHIII',*h)+v[24+start*h[7]:24+(start+count)*h[7]])
m=(a.input/'WEEZER.WZM').read_bytes();events=[struct.unpack('<I4B4BBB',m[i:i+14]) for i in range(20,len(m),14)];state=(0,0,0,0,0,0,0,0,0,0,0)
for e in events:
 if e[0]<=a.start:state=(0,)+e[1:9]+(0,0)
selected=[state]+[(e[0]-a.start,)+e[1:] for e in events if a.start<e[0]<a.start+a.duration]
(a.output/'WEEZER.WZM').write_bytes(struct.pack('<4sHHIII',b'WZM1',20,14,len(selected),a.duration,0)+b''.join(struct.pack('<I4B4BBB',*e) for e in selected))
(a.output/'WEEZER.CUE').write_bytes(struct.pack('<4sIII',b'WZC1',a.duration,0,a.duration));print(count,'frames;',len(selected),'music states')
