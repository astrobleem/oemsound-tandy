"""Rebuild the actual 8086 DOS launcher with caller-owned MSC6/MASM/LINK4."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
from RUNDOS import run

HERE = Path(__file__).resolve().parent
SOURCES = ('TPLAY.C', 'TPLAYPOL.H', 'SOUNDID.H', 'DOSSTAT.ASM', 'TPLAY.RSP', 'BUILD.BAT')


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dosbox', type=Path, required=True)
    p.add_argument('--toolchain', type=Path, required=True)
    p.add_argument('--lease', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    out, tool = a.out.resolve(), a.toolchain.resolve()
    for name in ('BIN/CL.EXE', 'BIN/MASM.EXE', 'BIN/LINK4.EXE', 'LIB/SLIBCE.LIB'):
        if not (tool / name).is_file():
            p.error('Missing external toolchain file: ' + name)
    out.mkdir(parents=True, exist_ok=False)
    for name in SOURCES:
        shutil.copyfile(HERE / name, out / name)
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
BUILD.BAT
exit
'''
    (out / 'BUILD.CNF').write_text(config)
    run(a.dosbox, out / 'BUILD.CNF', a.lease, 60)
    log = (out / 'BUILD.LOG').read_text(errors='replace')
    if 'PASS' not in log or 'FAIL' in log or not (out / 'TPLAY.EXE').is_file():
        raise RuntimeError('Build failed; inspect BUILD.LOG')
    record = {'target': 'MSC6 / MASM5.10 / LINK4; 8086 DOS small model',
              'sources': {n: identity((out / n).read_bytes()) for n in SOURCES},
              'runtime': identity((out / 'TPLAY.EXE').read_bytes())}
    (out / 'BUILD.JSON').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
