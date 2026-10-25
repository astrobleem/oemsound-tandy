'''Build an 8086 Win16 consumer with external DOS tools and a guarded lease.'''
from pathlib import Path
import argparse
import hashlib
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import shutil
import struct

TOOLS = ('BIN/CL.EXE', 'BIN/LINK4.EXE', 'LIB/SLIBCEW.LIB',
         'LIB/LIBW.LIB', 'DDK/286/INC/WINDOWS.H')

def identity(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

def module_imports(exe):
    if exe[:2] != b'MZ' or len(exe) < 64:
        raise ValueError('Missing DOS application header')
    ne = struct.unpack_from('<I', exe, 0x3c)[0]
    if ne+64 > len(exe) or exe[ne:ne+2] != b'NE':
        raise ValueError('Missing Win16 NE header')
    count = struct.unpack_from('<H', exe, ne+0x1e)[0]
    module_table = ne+struct.unpack_from('<H', exe, ne+0x28)[0]
    import_table = ne+struct.unpack_from('<H', exe, ne+0x2a)[0]
    if count > 64 or module_table+2*count > len(exe):
        raise ValueError('Invalid module reference table')
    names = []
    for i in range(count):
        at = import_table+struct.unpack_from('<H', exe, module_table+2*i)[0]
        if at >= len(exe) or not 1 <= exe[at] <= 64 or at+1+exe[at] > len(exe):
            raise ValueError('Invalid imported module name')
        names.append(exe[at+1:at+1+exe[at]].decode('ascii').upper())
    return names


def build(here, name, sources, header):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dosbox', type=Path, required=True)
    p.add_argument('--toolchain', type=Path, required=True)
    p.add_argument('--lease', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    out, tool = a.out.resolve(), a.toolchain.resolve()
    for path in (out, tool):
        if any(c in str(path) for c in ('"', '\r', '\n')):
            p.error('Unsafe DOS mount path')
    for relative in TOOLS:
        if not (tool / relative).is_file():
            p.error('Missing external toolchain file: ' + relative)
    if not a.dosbox.is_file():
        p.error('Installed DOSBox-X executable required')
    if not a.lease.is_file():
        p.error('Existing shared emulator lease required')
    inputs = {n: here/n for n in sources}
    inputs['PSGOWN.H'] = header
    for label, path in inputs.items():
        if not path.is_file():
            p.error('Missing consumer source: ' + label)
    out.mkdir(parents=True, exist_ok=False)
    for label, path in inputs.items():
        shutil.copyfile(path, out/label)
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
    (out/'BUILD.CNF').write_text(config)
    guard = Path(__file__).parent/'RUNDOS.PY'
    spec = importlib.util.spec_from_file_location('consumer_rundos', guard,
        loader=SourceFileLoader('consumer_rundos', str(guard)))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.run(a.dosbox, out/'BUILD.CNF', a.lease, 60)
    log = (out/'BUILD.LOG').read_text(errors='replace')
    if 'PASS' not in log or 'FAIL' in log:
        raise RuntimeError('Native build failed; inspect BUILD.LOG')
    binary = out/(name+'.EXE')
    exe = bytearray(binary.read_bytes())
    ne = struct.unpack_from('<I', exe, 0x3c)[0]
    if exe[ne:ne+2] != b'NE' or struct.unpack_from('<H', exe, ne+0xc)[0] & 8:
        raise RuntimeError('Unexpected Win16 EXE target')
    struct.pack_into('<H', exe, ne+0x3e, 0x0300)
    binary.write_bytes(exe)
    imports = module_imports(exe)
    if 'SOUND' in imports:
        raise RuntimeError('Early SOUND import: use dynamic installed-driver entry points')
    result = {'target': 'Windows 3.0 real mode; 8086; MSC6/LINK4',
              'compiler_flags': '/G0 /Gw /AS /W3 /Od', 'imports': imports,
              'sources': {n: identity(path) for n, path in inputs.items()},
              'toolchain': {n: identity(tool/n) for n in TOOLS},
              'build_wrapper': identity(here/'BUILD.PY'),
              'shared_builder': identity(Path(__file__)),
              'guard': identity(guard),
              'bytes': len(exe), 'sha256': hashlib.sha256(exe).hexdigest()}
    for record in ('BUILD.JSON', 'RESULT.JSON'):
        (out/record).write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
