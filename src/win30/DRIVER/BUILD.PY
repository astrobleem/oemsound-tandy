"""Build canonical SOUND with external MSC6/MASM/LINK4 and shared lease."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
from RUNDOS import run

HERE = Path(__file__).resolve().parent
SOURCES = ('TSOUND.C', 'TSOUND.DEF', 'TSOUND.RSP', 'PITCORE.H',
           'PITAPI.H', 'PITLINK.H', 'PITLEASE.H', 'PSGLINK.H', 'SHIM.ASM',
           'SNDTEST.DEF', 'SNDTEST.RSP', 'DRVTEST.C', 'DRVTEST.DEF',
           'DRVTEST.RSP', 'INTRUD.C', 'INTRUD.DEF', 'INTRUD.RSP', 'BUILD.BAT')
TOOLS = ('BIN/CL.EXE', 'BIN/MASM.EXE', 'BIN/LINK4.EXE',
         'LIB/SLIBCEW.LIB', 'LIB/LIBW.LIB', 'DDK/286/INC/WINDOWS.H')


def identity(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dosbox', type=Path, required=True)
    p.add_argument('--toolchain', type=Path, required=True)
    p.add_argument('--lease', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    out, tool = a.out.resolve(), a.toolchain.resolve()
    for name in TOOLS:
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
    if 'PASS' not in log or 'FAIL' in log:
        raise RuntimeError('Build failed; inspect BUILD.LOG')
    production = (out / 'SOUND.MAP').read_text(errors='replace').upper()
    test = (out / 'TSOUND.MAP').read_text(errors='replace').upper()
    hooks = ('DEBUGPSGCOUNT', 'DEBUGPSGTICK', 'DEBUGFAILNEXTTIMER',
             'DEBUGVOICEELAPSED', 'DEBUGBEEPRETURNWAITING',
             'DEBUGHOLDBEEPACTIVE', 'DEBUGTESTBEGINBEEP',
             'DEBUGTESTTICK', 'DEBUGTESTENDBEEP')
    if any(h in production or h not in test for h in hooks):
        raise RuntimeError('Test hooks leaked or are missing from variants')
    for name in ('TANDYPSGLEASE', 'TANDYPITLEASE', 'TANDYPITVOICE'):
        if name not in production or name not in test:
            raise RuntimeError('Missing production extension: ' + name)
    record = {'target': 'Windows 3.0 real mode; 8086; MSC6/MASM5.10/LINK4',
              'compiler_flags': '/G0 /Gw /Gs /AS /W3 /Od',
              'sources': {n: identity(out / n) for n in SOURCES},
              'toolchain': {n: identity(tool / n) for n in TOOLS},
              'runtime': identity(out / 'SOUND.DRV'),
              'test_runtime': identity(out / 'TSOUND.DRV'),
              'production_test_hooks': 'absent',
              'test_build_test_hooks': 'present'}
    (out / 'BUILD.JSON').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
