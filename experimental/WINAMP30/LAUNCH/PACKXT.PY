"""Build a new local Winamp packet from the unchanged published UX3 packet."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile

HERE = Path(__file__).resolve().parent
BASE_SHA = '389bab087507339ee1183c6ba7e6fc63a5142a85b1de1be53b085017f43a40c2'
SOUND_SHA = '7461d227846830f3ff604c40cf31ad0e3fb34122a13de6269e86de02cc4209e5'
RSV_SHA = '644290e234cab9a6ac3799fa32959bb493d82d566501a8925720bb5a28c6afa8'
SOURCES = ('TPLAY.C', 'TPLAYPOL.H', 'TPLAY.RSP', 'DOSSTAT.ASM', 'BUILD.BAT')


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def build(base, native, out, source_commit):
    if out.exists():
        raise ValueError('Output must be a new directory')
    raw = base.read_bytes()
    if hashlib.sha256(raw).hexdigest() != BASE_SHA:
        raise ValueError('Expected the published UX3 baseline ZIP')
    record = json.loads((native / 'BUILD.JSON').read_text())
    for name in SOURCES:
        if record['sources'][name] != identity((HERE / name).read_bytes()):
            raise ValueError('Build source differs: ' + name)
    launcher = (native / 'TPLAY.EXE').read_bytes()
    if record['runtime'] != identity(launcher):
        raise ValueError('Rebuilt launcher differs from BUILD.JSON')
    helper = (HERE / 'RESERVE.COM').read_bytes()
    if hashlib.sha256(helper).hexdigest() != RSV_SHA:
        raise ValueError('Expected the exact first-party XR1 reservation helper')
    sound = (HERE / 'TSOUND.DRV').read_bytes()
    if hashlib.sha256(sound).hexdigest() != SOUND_SHA:
        raise ValueError('Expected the exact tested TSOUND driver')
    with zipfile.ZipFile(base) as z:
        files = {n[len('TWUX3/'):]: z.read(n) for n in z.namelist()
                 if n.startswith('TWUX3/') and not n.endswith('/')}
    files['TPLAY.EXE'] = launcher
    files['RESERVE.COM'] = helper
    files['TSOUND.DRV'] = sound
    sound_source = HERE.parents[2] / 'experimental/PITPUBLIC/PITWIN/SOURCE'
    driver = sound_source / 'src/win30/DRIVER'
    sound_names = ('ABI.MD', 'BUILD.BAT', 'BUILD.PY', 'DRVTEST.C', 'DRVTEST.DEF', 'DRVTEST.RSP', 'INTRUD.C', 'INTRUD.DEF', 'INTRUD.RSP', 'LEASEQA.C', 'PITAPI.H', 'PITCORE.H', 'PITTEST.C', 'README.MD', 'SHIM.ASM', 'TESTS.JSON', 'TSOUND.C', 'TSOUND.DEF', 'TSOUND.RSP', 'hosttest/windows.h')
    for name in sound_names:
        files['DEPS/TSOUND/' + name] = (driver / name).read_bytes().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    files['DEPS/TSOUND/LICENSE'] = (sound_source / 'LICENSE').read_bytes().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    for name in SOURCES:
        if name != 'BUILD.BAT':
            files['DEPS/' + name] = (HERE / name).read_bytes()
    files['DEPS/BLDTPLAY.BAT'] = (HERE / 'BUILD.BAT').read_bytes()
    for name in ('RESERVE.ASM', 'RSV-MIT.TXT', 'POLICY.MD'):
        files['DEPS/' + name] = (HERE / name).read_bytes()
    for name in ('INSTALL.TXT', 'README.TXT', 'GO.BAT', 'TESTS.JSON'):
        files[name] = (HERE / name).read_bytes()
    files['BUILD.JSON'] = (json.dumps({'baseline_ux3': BASE_SHA,
        'source_commit': source_commit, 'launcher_build': record,
        'unchanged_player': identity(files['MINIPLAY.EXE']),
        'reservation': identity(helper), 'paired_sound': identity(sound)}, indent=2) + '\n').encode()
    runtime = [{ 'name': n, **identity(data)} for n, data in sorted(files.items())
               if '/' not in n and n not in ('MANIFEST.JSON', 'RUNTIME.JSON')]
    files['RUNTIME.JSON'] = (json.dumps(runtime, indent=2) + '\n').encode()
    files.pop('MANIFEST.JSON', None)
    manifest = [{'path': 'TWXT1/' + n, **identity(data)}
                for n, data in sorted(files.items())]
    files['MANIFEST.JSON'] = (json.dumps(manifest, indent=2) + '\n').encode()
    out.mkdir(parents=True)
    with zipfile.ZipFile(out / 'TWXT1.ZIP', 'w', compression=zipfile.ZIP_STORED) as z:
        for name, data in sorted(files.items()):
            if name.startswith('/') or '..' in name.split('/') or '\\' in name:
                raise ValueError('Unsafe archive member')
            info = zipfile.ZipInfo('TWXT1/' + name, (1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o100644 << 16
            z.writestr(info, data)
    for name, data in files.items():
        target = out / 'TWXT1' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    packet = identity((out / 'TWXT1.ZIP').read_bytes())
    (out / 'PACKET.JSON').write_text(json.dumps(packet, indent=2) + '\n')
    return packet


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--source-commit', required=True)
    a = p.parse_args()
    print(json.dumps(build(a.baseline, a.build, a.out, a.source_commit), indent=2))
