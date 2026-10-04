#!/usr/bin/env python3
"""Strict, read-only host validation of the DOS files and boundary arithmetic."""
from pathlib import Path
import argparse,struct,json,hashlib
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);a=p.parse_args();root=a.directory
v=(root/'WEEZER.WZV').read_bytes();m=(root/'WEEZER.WZM').read_bytes();c=(root/'WEEZER.CUE').read_bytes()
magic,w,h,fps,hs,n,d,fs=struct.unpack_from('<4sHHHHIII',v)
assert magic==b'WZV2' and hs==24 and (w,h,fps) in [(64,48,4),(128,96,8)]
assert fs==w*h//2 and fs<=6144 and 0<n<=4800 and 0<d<=600000
assert (n-1)*(1000//fps)<d<=n*(1000//fps) and len(v)==24+n*fs
magic,hs,rs,ne,dm,res=struct.unpack_from('<4sHHIII',m)
assert magic==b'WZM1' and hs==20 and rs==14 and dm==d and not res and 0<ne<=10000 and len(m)==20+14*ne
last=-1
for row in struct.iter_unpack('<I4B4BBB',m[20:]):
 t=row[0];notes=row[1:5];vel=row[5:9];retrigger,res=row[9:11]
 assert last<t<=d and not retrigger&240 and not res
 for ch,(nn,vv) in enumerate(zip(notes,vel)):
  assert 0<=vv<=127 and ((nn==0 and vv==0) or (1<=vv<=127 and (35 if ch==3 else 45)<=nn<=(81 if ch==3 else 96)))
 last=t
assert len(c)==16
magic,dc,start,end=struct.unpack('<4sIII',c)
assert magic==b'WZC1' and dc==d and 0<=start<end<=d
# Same bank mapping as DOS, with non-overlapping scanlines and bounded offsets.
ranges=[]
for y in range(h):
 sy=y+(200-h)//2;lo=(sy&3)*8192+(sy>>2)*160+(320-w)//4;hi=lo+w//2
 assert 0<=lo<hi<=32768
 for a0,a1 in ranges:assert hi<=a0 or a1<=lo
 ranges.append((lo,hi))
# Last-frame seek above 64K must retain all high words.
lastoff=24+(n-1)*fs;assert v[lastoff:lastoff+fs] and lastoff+fs==len(v)
result={'video':{'width':w,'height':h,'fps':fps,'frames':n,'bytes':len(v),'last_frame_offset':lastoff},'music':{'events':ne,'bytes':len(m)},'full_duration_ms':d,'song_range_ms':[start,end],'song_duration_ms':end-start,'Tandy_video_address_ranges_checked':len(ranges),'sha256':{name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in ['WEEZER.WZV','WEEZER.WZM','WEEZER.CUE']}}
print(json.dumps(result,indent=2))
