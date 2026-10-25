"""Generate exact consumer pins from the canonical native SOUND artifact."""
from pathlib import Path
import argparse,hashlib,json,zlib
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--driver',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args();data=a.driver.read_bytes()
if len(data)<64 or data[:2]!=b'MZ':p.error('Expected a native MZ/NE SOUND driver')
header=('/* Generated from the canonical SOUND.DRV; never hand-edit. */\n'
        '#ifndef SOUNDID_H\n#define SOUNDID_H\n'
        '#define SOUND_FILE "SOUND.DRV"\n'
        '#define SOUND_BYTES %dU\n#define SOUND_CRC 0x%08xUL\n'
        '#endif\n')%(len(data),zlib.crc32(data))
a.out.mkdir(parents=True,exist_ok=True)
(a.out/'SOUNDID.H').write_bytes(header.replace('\n','\r\n').encode('ascii'))
(a.out/'SOUNDID.JSON').write_text(json.dumps({'file':'SOUND.DRV','bytes':len(data),'crc32':'%08x'%zlib.crc32(data),'sha256':hashlib.sha256(data).hexdigest()},indent=2)+'\n')
