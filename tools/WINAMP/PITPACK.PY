"""Repack unchanged PIT clients with the exact tested shared DOS installer."""
from pathlib import Path, PurePosixPath
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
import argparse
import hashlib
import json
import re
import stat
import zipfile

HERE = Path(__file__).resolve().parent
PITROOT = HERE.parents[1]/'experimental/PITPUBLIC'
KITS = {
    'MML3AUTH': ('EVIDENCE/MANIFEST.json', 'PITMML.TXT'),
    'PITEXPR': ('EVIDENCE/EVIDENCE.json', 'PITEXPR.TXT'),
}
ORIGINALS = {
    'MML3AUTH': {'bytes': 168577, 'sha256':
        'e0b4e68365e24397688d6f41995b8f21fdc87d320f56f7a632ed1591301a25c2'},
    'PITEXPR': {'bytes': 201343, 'sha256':
        '2e3e43461496345eaff50b80f43f2aa82cf56031832b9725e8684040cc352453'},
}
OLD_SOUND = '7461d227846830f3ff604c40cf31ad0e3fb34122a13de6269e86de02cc4209e5'
OLD_GUIDES = {'README.TXT', 'README.MD', 'INSTALL.TXT', 'INSTALL.MD',
              'PITVOICE.MD', 'PITCONS.MD', 'PITMOVIE.MD', 'PITAUTH.MD'}
OLD_INDEXES = {'MANIFEST.JSON', 'RUNTIME.JSON', 'PROVENANCE.JSON', 'SHA256SUMS'}
NEW_NAMES = {'TWSETUP.EXE', 'TWXT2.ZIP', 'BUILD.JSN', 'MANIFEST.JSN'}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def safe(name):
    parts = PurePosixPath(name).parts
    require(name and not name.startswith('/') and '\\' not in name and
            ':' not in name and all(p not in ('', '.', '..') for p in parts),
            'Unsafe archive member: '+name)
    require(name == '/'.join(parts), 'Noncanonical archive member: '+name)


def read_zip(path, wanted):
    require(re.fullmatch('[0-9a-f]{64}', wanted) is not None,
            'An explicit lowercase SHA256 input pin is required')
    raw = path.read_bytes()
    require(identity(raw)['sha256'] == wanted, 'Input archive SHA256 differs')
    files, seen = {}, set()
    with zipfile.ZipFile(path) as archive:
        require(sum(i.file_size for i in archive.infolist()) <= 64*1024*1024,
                'Input archive is too large')
        for item in archive.infolist():
            name = item.filename.rstrip('/')
            safe(name)
            require(name.upper() not in seen, 'Duplicate archive member: '+name)
            seen.add(name.upper())
            require(not stat.S_ISLNK(item.external_attr >> 16), 'Linked archive member')
            if not item.is_dir():
                files[name] = archive.read(item)
    return raw, files


def load_module(name, path):
    loader = SourceFileLoader(name, str(path))
    spec = spec_from_loader(loader.name, loader)
    module = module_from_spec(spec)
    loader.exec_module(module)
    return module


def shared(package, package_sha, installer, tests_path):
    raw, files = read_zip(package, package_sha)
    verifier = load_module('pit_shared_verifier', HERE/'VERIFY.PY')
    verifier.verify(files, current=False)
    tests = tests_path.read_bytes()
    require(tests == files['TWXT2/TESTS.JSN'], 'External/shared native tests differ')
    receipt = json.loads(tests)
    require(receipt.get('status') == 'PASS', 'Shared native tests have not passed')
    require(receipt.get('release_readiness', {}).get('status') == 'READY',
            'Shared current-baseline reconciliation is incomplete')
    exe = installer.read_bytes()
    verifier.verify_sfx(exe, files)
    require(identity(exe) == receipt.get('installer'), 'Untested shared installer')
    return raw, exe, {
        'package': identity(raw), 'installer': identity(exe),
        'tests': identity(tests), 'sound': receipt['sound'],
        'source_package': 'TWXT2.ZIP',
    }


def original(kit, archive, archive_sha):
    raw, files = read_zip(archive, archive_sha)
    require(identity(raw) == ORIGINALS[kit], 'Expected the exact published PIT archive')
    prefix = kit+'/'
    if files and all(name.startswith(prefix) for name in files):
        files = {name[len(prefix):]: data for name,data in files.items()}
    require(files and not any(name.startswith(prefix) for name in files),
            'Mixed or unexpected kit root')
    receipt_path = PITROOT/kit/KITS[kit][0]
    manifest = dict(json.loads(files['MANIFEST.json'])['members'])
    historical_self = manifest.pop('MANIFEST.json', None)
    # The frozen authoring publication's self-named row identifies its original
    # stage receipt, preserved in the checkout, rather than the new index bytes.
    require(historical_self is None or (kit == 'MML3AUTH' and
            historical_self == identity(receipt_path.read_bytes())),
            'Original receipt row differs')
    require(set(manifest) == set(files)-{'MANIFEST.json'},
            'Original published member inventory differs')
    for name,pin in manifest.items():
        require(identity(files[name]) == pin, 'Original published member differs: '+name)
    accepted = json.loads(receipt_path.read_bytes())['files']
    source_pins = json.loads((PITROOT/'SOURCE-PINS.json').read_bytes())[kit]
    runtimes = {name: pin for name,pin in accepted.items()
                if PurePosixPath(name).suffix.upper() in ('.EXE','.DRV') and
                name.upper() != 'TSOUND.DRV'}
    require(runtimes, 'Missing accepted client runtime pins')
    for name,wanted in runtimes.items():
        require(name in files and identity(files[name])['sha256'] == wanted,
                'Accepted PIT client differs or is absent: '+name)
    kept, removed = {}, {}
    for name,data in files.items():
        upper = name.upper()
        suffix = PurePosixPath(name).suffix.upper()
        if name in accepted:
            require(identity(data)['sha256'] == accepted[name],
                    'Accepted PIT member differs: '+name)
        if upper == 'TSOUND.DRV':
            require(len(data) == 6168 and identity(data)['sha256'] == OLD_SOUND,
                    'Unexpected original provider')
        if suffix in ('.EXE','.DRV') and upper != 'TSOUND.DRV':
            require(name in runtimes, 'Unpinned PIT runtime: '+name)
        if name.startswith('SOURCE/'):
            rel = name[len('SOURCE/'):]
            require(source_pins.get(rel) == identity(data)['sha256'],
                    'PIT source snapshot differs: '+name)
        obsolete = (upper in OLD_GUIDES or upper in OLD_INDEXES or
                    upper == 'TSOUND.DRV' or
                    upper.startswith('SOURCE/SOUND/SRC/WIN30/DRIVER/') or
                    upper.startswith('SOURCE/SRC/WIN30/DRIVER/'))
        if obsolete:
            removed[name] = identity(data)
        else:
            require(upper not in NEW_NAMES, 'Reserved new packet filename: '+name)
            kept[name] = data
    require(all(name in kept for name in runtimes), 'A client was removed')
    return kept, {
        'archive': identity(raw), 'receipt': identity(receipt_path.read_bytes()),
        'runtime_sha256': runtimes,
        'preserved': {name: identity(data) for name,data in sorted(kept.items())},
        'removed': removed,
    }


def write_zip(path, kit, files):
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as archive:
        for name,data in sorted(files.items()):
            safe(name)
            info = zipfile.ZipInfo(kit+'/'+name, (1980,1,1,0,0,0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def verify(path, kit):
    raw = path.read_bytes()
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        require(len(names) == len(set(n.upper() for n in names)), 'Duplicate output member')
        require(all(name.startswith(kit+'/') for name in names), 'Wrong output root')
        files = {name[len(kit)+1:]: archive.read(name) for name in names}
    manifest = json.loads(files['MANIFEST.JSN'])
    require(set(manifest) == set(files)-{'MANIFEST.JSN'}, 'Output inventory differs')
    for name,pin in manifest.items():
        safe(name)
        require(identity(files[name]) == pin, 'Output member differs: '+name)
    record = json.loads(files['BUILD.JSN'])
    if record.get('schema') == 2:
        current = load_module('current_beats_packets',
            HERE.parents[1]/'src/win30/BEATLAB/PACK.PY')
        return current.verify_release_packet(path, kit)
    for name,pin in record['original']['preserved'].items():
        require(identity(files[name]) == pin, 'Preserved client/data/source changed: '+name)
    for name,wanted in record['original']['runtime_sha256'].items():
        require(identity(files[name])['sha256'] == wanted, 'PIT runtime changed: '+name)
    require(identity(files['TWSETUP.EXE']) == record['shared']['installer'] and
            identity(files['TWXT2.ZIP']) == record['shared']['package'],
            'Shared companion identity differs')
    require(not any(name.upper() == 'TSOUND.DRV' for name in files),
            'Obsolete provider in current payload')
    return identity(raw)


def build(kit, archive, archive_sha, package, package_sha, installer, tests, out):
    require(kit in KITS, 'Unknown PIT kit')
    require(not out.exists(), 'Output directory must be new')
    files, original_record = original(kit, archive, archive_sha)
    package_raw, exe, shared_record = shared(package, package_sha, installer, tests)
    guide = (HERE/KITS[kit][1]).read_bytes()
    files.update({'TWSETUP.EXE': exe, 'TWXT2.ZIP': package_raw,
                  'INSTALL.TXT': guide, 'README.TXT': guide})
    record = {
        'kit': kit, 'operation': 'unchanged PIT clients plus shared native Setup',
        'original': original_record, 'shared': shared_record,
        'qualification': {
            'shared': 'Exact PASS receipt inside TWXT2.ZIP',
            'pit_clients': 'Unchanged accepted application bytes; original receipts retain their scope',
            'fresh_pitexpr_native_claim': False,
        },
    }
    files['BUILD.JSN'] = (json.dumps(record, indent=2, sort_keys=True)+'\n').encode()
    files['MANIFEST.JSN'] = (json.dumps({name: identity(data) for name,data in
        sorted(files.items())}, indent=2, sort_keys=True)+'\n').encode()
    out.mkdir(parents=True)
    target = out/(kit+'.ZIP')
    write_zip(target, kit, files)
    pin = verify(target, kit)
    (out/'PACKET.JSN').write_text(json.dumps(pin, indent=2)+'\n')
    return pin


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit', choices=KITS, required=True)
    for name in ('input', 'shared', 'installer', 'tests', 'out'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--input-sha256', required=True)
    parser.add_argument('--shared-sha256', required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.kit, args.input, args.input_sha256,
        args.shared, args.shared_sha256, args.installer, args.tests, args.out)))
