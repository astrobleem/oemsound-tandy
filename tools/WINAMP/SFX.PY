"""Build the self-contained native DOS installer; no external extractor needed."""
from pathlib import Path
import argparse
import json
import re
import struct
import zlib
import hashlib

FOOTER = struct.Struct('<8s6I')
ENTRY = struct.Struct('<13s3x4I')


def make(setup, payload):
    exe = setup.read_bytes()
    if exe[:2] != b'MZ':
        raise ValueError('Expected the native DOS Setup EXE')
    rows = (payload/'PAYLOAD.LST').read_text().splitlines()
    if rows.pop(0) != 'TWSETUP1':
        raise ValueError('Wrong native Setup payload')
    files = {'PAYLOAD.LST': (payload/'PAYLOAD.LST').read_bytes()}
    for row in rows:
        kind, source, dest, size, crc = row.split('|')
        if source != dest or source in files or not re.fullmatch(
                r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}', source):
            raise ValueError('Unsafe or duplicate payload')
        data = (payload/source).read_bytes()
        if len(data) != int(size) or zlib.crc32(data) != int(crc,16):
            raise ValueError('Payload identity differs: '+source)
        files[source] = data
    if files.get('SETUP.EXE') != exe:
        raise ValueError('The embedded plain Setup must match the native loader')
    if not 1 <= len(files) <= 64:
        raise ValueError('Native overlay file limit')
    offset = len(files)*ENTRY.size
    table, body = bytearray(), bytearray()
    for name,data in sorted(files.items()):
        table.extend(ENTRY.pack(name.encode('ascii'), offset, len(data),
                                zlib.crc32(data), 0))
        body.extend(data)
        offset += len(data)
    bundle = bytes(table+body)
    if len(bundle) > 4*1024*1024:
        raise ValueError('Native overlay size limit')
    footer = FOOTER.pack(b'TWINSFX1', len(exe), len(bundle), len(files),
                         zlib.crc32(table), zlib.crc32(bundle), 0)
    return exe+bundle+footer


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--setup',type=Path,required=True)
    p.add_argument('--payload',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a = p.parse_args()
    if a.out.exists():
        p.error('Output must be new')
    data = make(a.setup, a.payload)
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_bytes(data)
    print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
                      'format':'TWINSFX1','native_extractor':'embedded SETUP.EXE'}))


if __name__ == '__main__':
    main()
