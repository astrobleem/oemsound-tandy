"""Generate the original short PSG fixtures used by the playlist UI cases."""
from pathlib import Path
import hashlib,json,struct,sys
out=Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parent.parent/'INPUTS'
out.mkdir(parents=True,exist_ok=True)
h=bytearray(64);h[:4]=b'Vgm '
struct.pack_into('<I',h,8,0x150);struct.pack_into('<I',h,12,3579545)
struct.pack_into('<HB',h,40,6,15)
commands=bytearray()
for i in range(60):
 values=[0x88,0x1a,0x94,0xa0,0x21,0xbf,0xc8,0x2b,0xdf,0xe7,0xf8] if i==0 else [0x94+i%8,0xf8+i%5]
 if i==5:values += [0xe7,0xe7]
 for value in values:commands += bytes([0x50,value])
 commands += b'\x62'
h += commands+b'\x66';struct.pack_into('<I',h,4,len(h)-4);struct.pack_into('<I',h,24,60*735)
names=['SHORT.VGM']+['F%02d.VGM'%n for n in range(2,6)]
for name in names:(out/name).write_bytes(h)
print(json.dumps({'files':names,'bytes_each':len(h),'sha256_each':hashlib.sha256(h).hexdigest()}))
