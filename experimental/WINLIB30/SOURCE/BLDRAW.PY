"""Build the canonical RAWCLK provider with external MSC6/MASM/LINK4."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import shutil
import struct

HERE = Path(__file__).resolve().parent
SOURCES = ('TIMECLK.C', 'PSGOWN.H', 'TIMECLK.H', 'TCBURST.H',
           'TIMECLK.DEF', 'TIMECLK.RSP', 'CLOCK.ASM', 'WINHOOK.ASM',
           'BLDRAW.BAT')


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dosbox', type=Path, required=True)
    parser.add_argument('--toolchain', type=Path, required=True)
    parser.add_argument('--lease', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    tool, out = args.toolchain.resolve(), args.out.resolve()
    for name in ('BIN/CL.EXE', 'BIN/MASM.EXE', 'BIN/LINK4.EXE',
                 'LIB/SLIBCEW.LIB', 'LIB/LIBW.LIB',
                 'DDK/286/INC/WINDOWS.H'):
        if not (tool/name).is_file():
            parser.error('Missing external toolchain input: '+name)
    out.mkdir(parents=True, exist_ok=False)
    for name in SOURCES:
        shutil.copyfile(HERE/name, out/name)
    config = f'''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=1
working directory option=force
working directory default={out}
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 200000
fpu=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound=true
[midi]
mididevice=none
[autoexec]
@echo off
mount C "{tool}" -ro
mount D "{out}"
D:
CALL BLDraw.BAT
exit
'''
    (out/'BUILD.CNF').write_text(config)
    helper = HERE.parents[2]/'src/win30/LAUNCH/RUNDOS.PY'
    loader = SourceFileLoader('owned_dos_child', str(helper))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.run(args.dosbox, out/'BUILD.CNF', args.lease, 60)
    log = (out/'RAWBUILD.LOG').read_text(errors='replace')
    if 'PASS' not in log or 'FAIL' in log:
        raise RuntimeError('RAWCLK build failed; inspect RAWBUILD.LOG')
    raw = bytearray((out/'RAWCLK.DLL').read_bytes())
    ne = struct.unpack_from('<I', raw, 0x3c)[0]
    if raw[ne:ne+2] != b'NE':
        raise RuntimeError('Expected Win16 RAWCLK library')
    # LINK4 defaults to3.1. This source uses only the Windows3.0 ABI.
    struct.pack_into('<H', raw, ne+0x3e, 0x0300)
    (out/'RAWCLK.DLL').write_bytes(raw)
    record = {'target': '8086 Windows3.0 real mode; MSC6/MASM/LINK4',
              'sources': {name: identity((out/name).read_bytes())
                          for name in SOURCES},
              'ne_expected_version': '3.0',
              'runtime': identity(raw),
              'status': 'compiled; native qualification separate'}
    (out/'BUILD.JSON').write_text(json.dumps(record, indent=2)+'\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
