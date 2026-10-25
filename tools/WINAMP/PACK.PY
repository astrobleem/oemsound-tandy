"""Package native Setup and one canonical SOUND driver from qualified builds."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import zipfile
import zlib

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
BASE_SHA = '055f872e4af09d5f2a8f8bc961dfadbcf274d56380e827006d5e3d91a3d3b061'
RUNTIME = ('CHIME.SND', 'LWFILE.DLL', 'MCITNDY.DLL', 'MIDICLK.DLL',
           'MINIPLAY.EXE', 'PLUCK.TND', 'RESERVE.COM', 'SHORT.VGM',
           'STREAM.MID', 'TBRIDGE.EXE')


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def native(directory, source, filename, remap=None):
    record = json.loads((directory / 'BUILD.JSON').read_bytes())
    for name, pin in record['sources'].items():
        path = (remap or {}).get(name, source / name)
        require(identity(path.read_bytes()) == pin, 'Stale native source: ' + str(path))
        require(identity((directory / name).read_bytes()) == pin,
                'Staged native source changed: ' + name)
    binary = (directory / filename).read_bytes()
    require(identity(binary) == record['runtime'], 'Native binary changed: ' + filename)
    process = json.loads((directory / 'PROCESS.JSON').read_bytes())
    require(process.get('return') == 0 and process.get('owned_child_stopped') and
            process.get('lease_released') and 'error' not in process,
            'Native build did not stop/release normally: ' + str(directory))
    return binary, record


def build(a):
    require(not a.out.exists(), 'Output directory must be new')
    base = a.baseline.read_bytes()
    require(identity(base)['sha256'] == BASE_SHA, 'Expected the published XT1 baseline')
    with zipfile.ZipFile(a.baseline) as z:
        previous = {n[6:]: z.read(n) for n in z.namelist()
                    if n.startswith('TWXT1/') and not n.endswith('/')}
    files = {n: previous[n] for n in RUNTIME}
    records = {}
    specs = (
        ('driver', a.driver, REPO/'src/win30/DRIVER', 'SOUND.DRV', {}),
        ('raw', a.raw, REPO/'experimental/WINLIB30/SOURCE', 'RAWCLK.DLL', {}),
        ('launch', a.launch, REPO/'src/win30/LAUNCH', 'TPLAY.EXE', {}),
        ('setup', a.setup, REPO/'src/win30/SETUP', 'SETUP.EXE', {
            'SOUNDID.H': REPO/'src/win30/DRIVER/SOUNDID.H',
            'TPLAYPOL.H': REPO/'src/win30/LAUNCH/TPLAYPOL.H'}),
    )
    for label, directory, source, name, remap in specs:
        files[name], records[label] = native(directory, source, name, remap)
        for member in records[label]['sources']:
            files['SOURCE/'+label.upper()+'/'+member] = (directory/member).read_bytes()
        for extra in source.rglob('*'):
            if extra.is_file() and '__pycache__' not in extra.parts and extra.suffix.upper() in (
                    '.C','.H','.ASM','.DEF','.RSP','.BAT','.PY','.MD','.TXT','.JSON'):
                rel=extra.relative_to(source).as_posix()
                files.setdefault('SOURCE/'+label.upper()+'/'+rel,extra.read_bytes())
        files['EVIDENCE/'+label.upper()+'.JSN'] = (directory/'BUILD.JSON').read_bytes()
        files['EVIDENCE/'+label.upper()+'.LOG'] = (directory/'BUILD.LOG' if label != 'raw'
            else directory/'RAWBUILD.LOG').read_bytes()
    clients = json.loads((a.clients/'PINS.JSN').read_bytes())
    require(len(clients) == 7, 'Expected the current Windows XT clients and paired input dependency')
    for name,record in clients.items():
        data = (a.clients/name).read_bytes()
        require(identity(data) == record['runtime'], 'Changed native client: '+name)
        require('%08x'%zlib.crc32(data) == record['crc32'], 'Changed native client CRC')
        for source,pin in record['sources'].items():
            require(identity((a.workspace/source).read_bytes()) == pin,
                    'Current client source changed: '+source)
        files[name] = data
    for source in (a.clients/'SOURCE').rglob('*'):
        if source.is_file():
            files['SOURCE/CLIENTS/'+source.relative_to(a.clients/'SOURCE').as_posix()] = source.read_bytes()
    files['EVIDENCE/CLIENTS.JSN'] = (a.clients/'PINS.JSN').read_bytes()
    sound = json.loads((REPO/'src/win30/DRIVER/SOUNDID.JSON').read_bytes())
    require(identity(files['SOUND.DRV']) == {k: sound[k] for k in ('bytes','sha256')},
            'Canonical generated sound pin differs')
    require('%08x' % zlib.crc32(files['SOUND.DRV']) == sound['crc32'], 'Sound CRC differs')
    for label in ('driver','launch','setup'):
        data = files.get('SOURCE/'+label.upper()+'/SOUNDID.H')
        if data is not None:
            require(data == (REPO/'src/win30/DRIVER/SOUNDID.H').read_bytes(),
                    'Consumers use different canonical driver expectations')
    for name in ('INSTALL.TXT','README.TXT'):
        files[name] = (HERE/name).read_bytes()
    files['TESTS.JSN'] = a.tests.read_bytes()
    tests = json.loads(files['TESTS.JSN'])
    require(tests.get('status') == 'PASS', 'Native acceptance has not passed')
    require(tests.get('release_readiness', {}).get('status') == 'READY',
            'Current published baseline reconciliation is incomplete')
    require(tests.get('sound') == identity(files['SOUND.DRV']), 'Tests cover another driver')
    require(tests.get('rawclk') == identity(files['RAWCLK.DLL']), 'Tests cover another RAWCLK')
    require(tests.get('setup') == identity(files['SETUP.EXE']), 'Tests cover another Setup')
    require(tests.get('launcher') == identity(files['TPLAY.EXE']), 'Tests cover another launcher')
    require(tests.get('clients') == {n:r['runtime'] for n,r in clients.items()},
            'Tests cover different shared client files')
    # Native Setup installs only these flat runtime files. The generated manifest
    # includes SETUP itself; neither its text nor the driver embeds the manifest.
    rows = ['TWSETUP1']
    for name in sorted(n for n in files if '/' not in n and n not in ('TESTS.JSN',)):
        require(re.fullmatch(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}', name), 'Non-DOS runtime name')
        data = files[name]
        kind = clients[name]['kind'] if name in clients else ('S' if name == 'SOUND.DRV' else 'A')
        rows.append('%s|%s|%s|%d|%08x' %
                    (kind, name, name,
                     len(data), zlib.crc32(data)))
    files['PAYLOAD.LST'] = ('\r\n'.join(rows)+'\r\n').encode('ascii')
    # Preserve unchanged application source and notices from the accepted player.
    for name, data in previous.items():
        if name.startswith('SRC/'):
            if name in ('SRC/INSTALL.TXT','SRC/README.md'):
                continue
            files[name] = data
        elif name.startswith('DEPS/') and not name.startswith('DEPS/TSOUND/'):
            if name.rsplit('/',1)[-1] in (
                    'README.TXT','POLICY.MD','BUILD.BAT','BLDTPLAY.BAT'):
                continue
            if name.rsplit('/',1)[-1] not in records['raw']['sources'] and name.rsplit('/',1)[-1] not in records['launch']['sources']:
                files[name] = data
    files['DEPS/README.TXT'] = (
        'Corresponding source for unchanged MIDI/MCI and timer-bridge components.\r\n'
        'Current driver, raw clock, launcher and installer sources and builders\r\n'
        'are in SOURCE/DRIVER, SOURCE/RAW, SOURCE/LAUNCH and SOURCE/SETUP.\r\n'
        'The player and file browser source is in SRC. Native build receipts\r\n'
        'bind the current runtime bytes to those source snapshots.\r\n').encode('ascii')
    files['SRC/README.TXT'] = (
        'Corresponding source for the unchanged player and file browser.\r\n'
        'Run the native installer described by the packet-root INSTALL.TXT.\r\n'
        'Current shared driver, raw clock, launcher and Setup are in SOURCE.\r\n'
        'Historical test records retain their original binary scope.\r\n').encode('ascii')
    files['SOURCE/DRIVER/LICENSE'] = (REPO/'LICENSE').read_bytes()
    files['BUILD.JSN'] = (json.dumps({'baseline_xt1': BASE_SHA,
        'source_commit': a.source_commit, 'canonical_sound': sound,
        'native_builds': records, 'clients': clients}, indent=2)+'\n').encode()
    files['MANIFEST.JSN'] = (json.dumps({n: identity(data) for n,data in
        sorted(files.items())}, indent=2)+'\n').encode()
    a.out.mkdir(parents=True)
    with zipfile.ZipFile(a.out/'TWXT2.ZIP','w',compression=zipfile.ZIP_STORED) as z:
        for name,data in sorted(files.items()):
            require(not name.startswith('/') and '..' not in name.split('/'), 'Unsafe ZIP path')
            info = zipfile.ZipInfo('TWXT2/'+name, (1980,1,1,0,0,0))
            info.external_attr = 0o100644 << 16
            z.writestr(info,data)
            target = a.out/'TWXT2'/name
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(data)
    result = identity((a.out/'TWXT2.ZIP').read_bytes())
    # Same native Setup body, with its validated flat payload overlay appended.
    from importlib.machinery import SourceFileLoader
    from importlib.util import spec_from_loader, module_from_spec
    loader=SourceFileLoader('native_sfx',str(HERE/'SFX.PY'))
    spec=spec_from_loader(loader.name,loader);sfx=module_from_spec(spec);loader.exec_module(sfx)
    exe=sfx.make(a.out/'TWXT2/SETUP.EXE',a.out/'TWXT2')
    require(tests.get('installer') == identity(exe),
            'Tests cover another self-contained installer payload')
    (a.out/'TWSETUP.EXE').write_bytes(exe)
    (a.out/'PACKET.JSN').write_text(json.dumps(
        {**result, 'installer': identity(exe)},indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for n in ('baseline','driver','raw','launch','setup','clients','workspace','tests','out'):
        p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--source-commit',required=True)
    build(p.parse_args())
