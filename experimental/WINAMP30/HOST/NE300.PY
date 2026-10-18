"""Set the NE expected Windows version, preserving all other bytes."""
from pathlib import Path
import struct,sys
for name in sys.argv[1:]:
 p=Path(name);b=bytearray(p.read_bytes())
 if b[:2]!=b'MZ' or len(b)<64:raise SystemExit('Not MZ: '+str(p))
 at=struct.unpack_from('<I',b,60)[0]
 if b[at:at+2]!=b'NE':raise SystemExit('Not NE: '+str(p))
 struct.pack_into('<H',b,at+62,0x0300);p.write_bytes(b)
