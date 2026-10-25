"""Build the native 8086 installer with caller-owned Microsoft C 6 tools."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import shutil

HERE = Path(__file__).resolve().parent
WIN30 = HERE.parent
SOURCES = ('SETUP.C', 'SETUP.RSP', 'BUILD.BAT', 'XTCLIENT.H', 'SFXCORE.H')


def identity(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dosbox', required=True, type=Path)
    parser.add_argument('--toolchain', required=True, type=Path)
    parser.add_argument('--lease', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    tool, out = args.toolchain.resolve(), args.out.resolve()
    for name in ('BIN/CL.EXE', 'BIN/LINK4.EXE', 'LIB/SLIBCE.LIB'):
        if not (tool / name).is_file():
            parser.error('Missing external toolchain file: ' + name)
    out.mkdir(parents=True, exist_ok=False)
    for name in SOURCES:
        shutil.copyfile(HERE / name, out / name)
    shutil.copyfile(WIN30 / 'DRIVER/SOUNDID.H', out / 'SOUNDID.H')
    shutil.copyfile(WIN30 / 'LAUNCH/TPLAYPOL.H', out / 'TPLAYPOL.H')
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
    runpath = WIN30 / 'LAUNCH/RUNDOS.PY'
    spec = importlib.util.spec_from_loader('setup_rundos',
        SourceFileLoader('setup_rundos', str(runpath)))
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    runner.run(args.dosbox, out / 'BUILD.CNF', args.lease, 60)
    log = (out / 'BUILD.LOG').read_text(errors='replace')
    if 'PASS' not in log or 'FAIL' in log:
        raise RuntimeError('Installer build failed; inspect BUILD.LOG')
    names = (*SOURCES, 'SOUNDID.H', 'TPLAYPOL.H')
    record = {'target': 'DOS3+, MSC6 /G0 /AS, LINK4, 8086, 8KB stack',
              'sources': {name: identity(out / name) for name in names},
              'runtime': identity(out / 'SETUP.EXE'),
              'test_runtime': identity(out / 'SETUPTST.EXE'),
              'test_only_option': '/FAIL:n; absent in production'}
    (out / 'BUILD.JSON').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
