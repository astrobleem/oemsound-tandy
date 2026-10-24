"""Verify the exact XT1 download, source/build links and native receipts."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(okay, reason):
    if not okay:
        raise ValueError(reason)


def verify_members(files):
    for name in files:
        require(name.startswith('TWXT1/') and '\\' not in name and
                all(p not in ('', '.', '..') for p in name.split('/')),
                'Unsafe member: ' + name)
    rows = json.loads(files['TWXT1/MANIFEST.JSON'])
    names = {r['path'] for r in rows}
    require(len(names) == len(rows), 'Duplicate manifest member')
    require(names == set(files) - {'TWXT1/MANIFEST.JSON'}, 'Manifest member set')
    for row in rows:
        require(identity(files[row['path']]) ==
                {'bytes': row['bytes'], 'sha256': row['sha256']},
                'Manifest identity: ' + row['path'])
    build = json.loads(files['TWXT1/BUILD.JSON'])
    native = json.loads((HERE/'EVIDENCE/XRSYNC-BUILD.JSON').read_bytes())
    require(build['launcher_build'] == native, 'Native build record differs')
    for name, pin in native['sources'].items():
        require(identity((HERE/name).read_bytes()) == pin, 'Source pin: ' + name)
        member = 'BLDTPLAY.BAT' if name == 'BUILD.BAT' else name
        require(identity(files['TWXT1/DEPS/'+member]) == pin,
                'Packaged source pin: ' + name)
    require(identity(files['TWXT1/TPLAY.EXE']) == native['runtime'],
            'Launcher differs from native-tested bytes')
    require(identity(files['TWXT1/RESERVE.COM']) == identity((HERE/'RESERVE.COM').read_bytes()),
            'Wrong reservation helper')
    sound = files['TWXT1/TSOUND.DRV']
    require(identity(sound) == {'bytes': 6168, 'sha256':
        '7461d227846830f3ff604c40cf31ad0e3fb34122a13de6269e86de02cc4209e5'},
        'Wrong paired TSOUND driver')
    require(sound == (HERE/'TSOUND.DRV').read_bytes(), 'Bundled sound differs')
    require(files['TWXT1/DEPS/TSOUND/TSOUND.C'] ==
        (ROOT/'experimental/PITPUBLIC/PITWIN/SOURCE/src/win30/DRIVER/TSOUND.C').read_bytes().replace(b'\r\n',b'\n').replace(b'\n',b'\r\n'),
        'Paired sound source differs')
    require(files['TWXT1/DEPS/TSOUND/LICENSE'] ==
        (ROOT/'experimental/PITPUBLIC/PITWIN/SOURCE/LICENSE').read_bytes().replace(b'\r\n',b'\n').replace(b'\n',b'\r\n'),
        'Paired sound license missing or changed')
    for name in ('INSTALL.TXT','README.TXT','GO.BAT','TESTS.JSON'):
        require(files['TWXT1/'+name] == (HERE/name).read_bytes(),
                'Packaged guide/receipt differs: ' + name)
    tests = json.loads(files['TWXT1/TESTS.JSON'])
    require(tests['launcher'] == native['runtime'], 'Test launcher differs')
    require(tests['host_policy_checks'] == 49 and tests['native_preflight_cases'] == 31,
            'Focused test counts')
    run = tests['xrsync_integration']
    require(run['launcher'] == native['runtime'], 'XRSYNC tested launcher')
    for check in ('selected_display_file=1','play_started=1','graphics_updates=1',
                  'playing_during_graphics=1','sound_released=1','all_pass=1'):
        require(check in run['checks'], 'Native receipt: ' + check)
    require(any('owned bridge removed; Windows return=0' in x for x in run['launch']),
            'Native cleanup receipt')
    for name in ('lease_acquired','lease_released','owned_child_stopped'):
        require(run['process'][name] is True, 'Native lifecycle: ' + name)
    require(run['process']['return'] == 0, 'Native child exit')
    return len(rows)


def self_test(files):
    def rejected(case, label):
        try:
            verify_members(case)
        except (ValueError, KeyError):
            return
        raise ValueError('Corruption was accepted: ' + label)
    def altered(name, data):
        case = files.copy();case[name] = data
        rows = json.loads(case['TWXT1/MANIFEST.JSON'])
        for row in rows:
            if row['path'] == name:
                row.update(identity(data))
        case['TWXT1/MANIFEST.JSON'] = json.dumps(rows).encode()
        return case
    case = files.copy();del case['TWXT1/RESERVE.COM'];rejected(case,'missing helper')
    rejected(altered('TWXT1/RESERVE.COM', b'wrong'), 'wrong helper')
    rejected(altered('TWXT1/TSOUND.DRV', b'wrong'), 'wrong paired sound')
    rejected(altered('TWXT1/DEPS/TPLAYPOL.H', b'wrong'), 'unlinked source')
    case = files.copy();case['TWXT1/EXTRA.TXT'] = b'extra';rejected(case,'extra member')
    print('PASS 5 package corruption checks')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    release = json.loads((HERE/'RELEASE.JSON').read_bytes())
    path = ROOT/release['archive']['path']
    data = path.read_bytes()
    require(identity(data) == {k:release['archive'][k] for k in ('bytes','sha256')},
            'Download archive identity differs')
    with zipfile.ZipFile(path) as z:
        require(len(set(z.namelist())) == len(z.namelist()), 'Duplicate ZIP member')
        files = {n:z.read(n) for n in z.namelist()}
    count = verify_members(files)
    if args.self_test:
        self_test(files)
    print('PASS',count,'manifest identities; exact source/runtime/native receipt links')
    print(release['archive']['sha256'], path.name)


if __name__ == '__main__':
    main()
