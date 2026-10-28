"""Developer-only lossless WZV2 -> WZP1 RGBI bitmap conversion."""
import argparse,struct,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
v=a.input.read_bytes();h=struct.unpack('<4sHHHHIII',v[:24])
assert h[:5]==(b'WZV2',64,48,4,24) and h[7]==1536 and len(v)==24+h[5]*1536
assert not a.output.exists(),'Output must be new'
out=bytearray(v[:24]);out[:4]=b'WZP1'
for pos in range(24,len(v),1536):
 data=v[pos:pos+1536]
 for row in range(48):
  indices=[n for b in data[row*32:(row+1)*32] for n in (b>>4,b&15)]
  for mask in (4,2,1,8):
   for x in range(0,64,8):
    value=0
    for n in indices[x:x+8]:value=(value<<1)|bool(n&mask)
    out.append(value)
a.output.write_bytes(out);print(len(out),hashlib.sha256(out).hexdigest())
