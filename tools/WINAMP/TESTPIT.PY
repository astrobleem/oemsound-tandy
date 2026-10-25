"""Focused repack tests with synthetic unit fixtures, never native qualification."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
import json
import tempfile
import zipfile
import zlib

HERE = Path(__file__).resolve().parent
loader = SourceFileLoader('pit_repack', str(HERE/'PITPACK.PY'))
spec = spec_from_loader(loader.name, loader)
pack = module_from_spec(spec)
loader.exec_module(pack)
checks = 0


def check(yes):
    global checks
    checks += 1
    if not yes:
        raise AssertionError('Check failed: '+str(checks))


def rejected(function, text=None):
    global checks
    checks += 1
    try:
        function()
    except (ValueError, KeyError):
        return
    raise AssertionError('Invalid package accepted: '+str(text))


def dump(value):
    return (json.dumps(value, sort_keys=True)+'\n').encode()


def zip_files(path, root, files):
    pack.write_zip(path, root, files)
    return pack.identity(path.read_bytes())['sha256']


def shared_fixture(root):
    folder = root/'shared'
    folder.mkdir()
    # These bytes model the package contract, not runnable DOS/Windows binaries.
    files = {name: b'UNIT FIXTURE '+name.encode() for name in (
        'SOUND.DRV','RAWCLK.DLL','TPLAY.EXE','MINIPLAY.EXE','MIDICLK.DLL',
        'MCITNDY.DLL','LWFILE.DLL','TBRIDGE.EXE','RESERVE.COM')}
    files['SETUP.EXE'] = b'MZUNIT TEST ONLY'
    clients = {}
    for name, kind in (
            ('TSHELL.EXE','W'),('TCHIME.EXE','X'),('XPCHIME.EXE','X'),
            ('TEXIT.EXE','X'),('TP.EXE','X'),('FIXXT.EXE','X'),('TSINPUT.DLL','W')):
        files[name] = b'UNIT CLIENT '+name.encode()
        clients[name] = {'kind': kind, 'runtime': pack.identity(files[name]),
                         'crc32': '%08x'%zlib.crc32(files[name]), 'sources': {}}
    header = (HERE.parents[1]/'src/win30/DRIVER/SOUNDID.H').read_bytes()
    builds = {}
    for label, name in (('driver','SOUND.DRV'),
                        ('launch','TPLAY.EXE'),('setup','SETUP.EXE')):
        source = label.encode()
        builds[label] = {'runtime': pack.identity(files[name]),
                          'sources': {'UNIT.C': pack.identity(source)}}
        files['SOURCE/'+label.upper()+'/UNIT.C'] = source
        if label in ('launch','setup'):
            files['SOURCE/'+label.upper()+'/SOUNDID.H'] = header
            builds[label]['sources']['SOUNDID.H'] = pack.identity(header)
    builder = pack.load_module('pit_unit_shared_builder', HERE/'PACK.PY')
    process = dump({'return': 0, 'owned_child_stopped': True, 'lease_released': True})
    for label, names in builder.CLOCK_SOURCES.items():
        sources = {name: b'UNIT SOURCE '+name.encode() for name in names}
        builds[label] = {'runtime': pack.identity(files[builder.CLOCK_FILES[label]]),
                         'sources': {n: pack.identity(d) for n,d in sources.items()},
                         'common_process_identity': pack.identity(process)}
        for name, data in sources.items():
            files['SOURCE/'+label.upper()+'/'+name] = data
            files['DEPS/'+name] = data
        files['EVIDENCE/'+label.upper()+'.JSN'] = dump(builds[label])
        files['EVIDENCE/'+label.upper()+'.LOG'] = b'PASS MIDI THREE DLL BUILD\r\n'
        files['EVIDENCE/'+label.upper()+'PROC.JSN'] = process
    sound = dict(pack.identity(files['SOUND.DRV']),
                 crc32='%08x'%zlib.crc32(files['SOUND.DRV']))
    files['EVIDENCE/CLIENTS.JSN'] = dump(clients)
    files['BUILD.JSN'] = dump({'native_builds': builds, 'canonical_sound': sound})
    lines = ['TWSETUP1']
    for name in sorted(n for n in files if '/' not in n and n != 'BUILD.JSN'):
        kind = clients[name]['kind'] if name in clients else ('S' if name == 'SOUND.DRV' else 'A')
        data = files[name]
        lines.append('%s|%s|%s|%d|%08x'%(kind,name,name,len(data),zlib.crc32(data)))
    files['PAYLOAD.LST'] = ('\r\n'.join(lines)+'\r\n').encode()
    for name,data in files.items():
        if '/' not in name:
            (folder/name).write_bytes(data)
    sfx = pack.load_module('pit_unit_sfx', HERE/'SFX.PY')
    exe = sfx.make(folder/'SETUP.EXE', folder)
    tests = {'status': 'PASS', 'release_readiness': {'status': 'READY'},
             'scope': 'Synthetic unit fixture only',
             'installer': pack.identity(exe), 'clients':
             {name: row['runtime'] for name,row in clients.items()}}
    for key,name in (('sound','SOUND.DRV'),('rawclk','RAWCLK.DLL'),
                     ('midiclk','MIDICLK.DLL'),('mcitndy','MCITNDY.DLL'),
                     ('setup','SETUP.EXE'),('launcher','TPLAY.EXE')):
        tests[key] = pack.identity(files[name])
    files['TESTS.JSN'] = dump(tests)
    files['MANIFEST.JSN'] = dump({name: pack.identity(data) for name,data in files.items()})
    archive = root/'TWXT2.ZIP'
    sha = zip_files(archive, 'TWXT2', files)
    installer, receipt = root/'TWSETUP.EXE', root/'TESTS.JSN'
    installer.write_bytes(exe)
    receipt.write_bytes(files['TESTS.JSN'])
    return archive, sha, installer, receipt, files


def write_original(path, kit, files):
    files = dict(files)
    files.pop('MANIFEST.json', None)
    files['MANIFEST.json'] = dump({'members': {
        name: pack.identity(data) for name,data in files.items()}})
    sha = zip_files(path, kit, files)
    pack.ORIGINALS[kit] = pack.identity(path.read_bytes())
    return sha


def kit_fixture(root, kit):
    files = {'MIDIMAP.DRV': b'unit mapper'} if kit == 'PITEXPR' else {}
    if kit == 'MML3AUTH':
        files.update({'BEATS/BEATS.EXE': b'authoring beats',
                      'BEATS/MIDIMAP.DRV': b'unit mapper',
                      'BEATS/DEFAULT2.SNG': b'accepted beats data',
                      'FIDDLE/MMLFIDL.EXE': b'accepted fiddle',
                      'FIDDLE/MIDIMAP.DRV': b'unit mapper',
                      'FIDDLE/PIT.MML': b'accepted [P] musical data'})
    else:
        files.update({name: b'expressive '+name.encode() for name in
                      ('BEATS.EXE','JOYMIDI.EXE','MINIMIDI.EXE','MINIPIAN.EXE')})
        files['DEFAULT2.SNG'] = b'accepted expressive data'
    files.update({'SOURCE/CLIENT.C': b'unchanged client source',
                  'SOURCE/SOUND/src/win30/DRIVER/TSOUND.C': b'obsolete provider source',
                  'TSOUND.DRV': b'T'*6168,
                  'INSTALL.TXT': b'old manual driver copy instructions',
                  'README.TXT': b'old guide', 'PITVOICE.MD': b'old top guide',
                  'LICENSE': b'unchanged client notice'})
    pins = {name: pack.identity(data)['sha256'] for name,data in files.items()}
    receipt = root/'pitroot'/kit/pack.KITS[kit][0]
    receipt.parent.mkdir(parents=True)
    receipt.write_bytes(dump({'files': pins}))
    archive = root/(kit+'-original.zip')
    return archive, write_original(archive, kit, files), files


def main():
    with tempfile.TemporaryDirectory(prefix='pit-repack-unit-') as tmp:
        root = Path(tmp)
        pack.PITROOT = root/'pitroot'
        pack.OLD_SOUND = pack.identity(b'T'*6168)['sha256']
        original = {}
        for kit in pack.KITS:
            original[kit] = kit_fixture(root, kit)
        source_pins = {kit: {name[7:]: pack.identity(data)['sha256']
                            for name,data in row[2].items() if name.startswith('SOURCE/')}
                       for kit,row in original.items()}
        (pack.PITROOT/'SOURCE-PINS.json').write_bytes(dump(source_pins))
        shared, shared_sha, exe, tests, shared_files = shared_fixture(root)
        pending = dict(shared_files)
        pending_tests = json.loads(pending['TESTS.JSN'])
        pending_tests['release_readiness'] = {'status': 'PENDING_SEPARATE_RECONCILIATION'}
        pending['TESTS.JSN'] = dump(pending_tests)
        pending['MANIFEST.JSN'] = dump({name: pack.identity(data) for name,data in pending.items()
                                        if name != 'MANIFEST.JSN'})
        pending_zip = root/'PENDING.ZIP'
        pending_sha = zip_files(pending_zip, 'TWXT2', pending)
        pending_receipt = root/'PENDING.JSN'
        pending_receipt.write_bytes(pending['TESTS.JSN'])
        rejected(lambda: pack.shared(pending_zip, pending_sha, exe, pending_receipt),
                 'pending current baseline must not publish')

        for kit,(archive,sha,files) in original.items():
            def build(out, archive=archive, sha=sha):
                return pack.build(kit, archive, sha, shared, shared_sha, exe, tests, out)
            first, second = root/(kit+'-one'), root/(kit+'-two')
            check(build(first) == build(second))
            check((first/(kit+'.ZIP')).read_bytes() == (second/(kit+'.ZIP')).read_bytes())
            with zipfile.ZipFile(first/(kit+'.ZIP')) as z:
                output = {name[len(kit)+1:]: z.read(name) for name in z.namelist()}
            for name,data in files.items():
                if name.startswith(('BEATS/','FIDDLE/')) or name in (
                        'BEATS.EXE','JOYMIDI.EXE','MINIMIDI.EXE','MINIPIAN.EXE',
                        'MIDIMAP.DRV','DEFAULT2.SNG','SOURCE/CLIENT.C','LICENSE'):
                    check(output[name] == data)
            check(output['TWSETUP.EXE'] == exe.read_bytes())
            check(output['TWXT2.ZIP'] == shared.read_bytes())
            check('SOURCE/SOUND/src/win30/DRIVER/TSOUND.C' not in output)
            check('PITVOICE.MD' not in output)
            check('TSOUND.DRV' not in output)
            check(output['INSTALL.TXT'] != files['INSTALL.TXT'])
            check(json.loads(output['BUILD.JSN'])['qualification']['fresh_pitexpr_native_claim'] is False)
            rejected(lambda: pack.original(kit, archive, '0'*64), 'wrong original input pin')
            corrupt = dict(files)
            runtime = next(name for name in files if name.endswith('.EXE'))
            corrupt[runtime] = b'changed application'
            bad = root/(kit+'-wrong.zip')
            bad_sha = write_original(bad, kit, corrupt)
            rejected(lambda: pack.original(kit, bad, bad_sha), 'changed runtime')
            corrupt = dict(files, **{'SOURCE/CLIENT.C': b'changed source'})
            bad = root/(kit+'-source.zip')
            bad_sha = write_original(bad, kit, corrupt)
            rejected(lambda: pack.original(kit, bad, bad_sha), 'changed source')
            corrupt = dict(files, **{'UNPIN.EXE': b'new unaccepted runtime'})
            bad = root/(kit+'-extra.zip')
            bad_sha = write_original(bad, kit, corrupt)
            rejected(lambda: pack.original(kit, bad, bad_sha), 'extra runtime')
            pack.ORIGINALS[kit] = pack.identity(archive.read_bytes())
            rejected(lambda: build(first), 'existing output')
            corrupt = dict(output)
            corrupt[runtime] = b'changed after packaging'
            bad = root/(kit+'-output.zip')
            zip_files(bad, kit, corrupt)
            rejected(lambda: pack.verify(bad, kit), 'readback tamper')
        rejected(lambda: pack.shared(shared, '0'*64, exe, tests), 'wrong shared input pin')
        wrong = root/'wrong-tests.json'
        wrong.write_bytes(b'{"status":"PASS"}')
        rejected(lambda: pack.shared(shared, shared_sha, exe, wrong), 'another receipt')
        wrong = root/'wrong-setup.exe'
        wrong.write_bytes(exe.read_bytes()[:-1])
        rejected(lambda: pack.shared(shared, shared_sha, wrong, tests), 'truncated SFX')
        bad_files = dict(shared_files)
        bad_files['TWXT2/SOUND.DRV'] = b'wrong driver'
        # write_zip expects unprefixed names, so use a fresh mapping directly.
        bad_files = {name[6:] if name.startswith('TWXT2/') else name: data
                     for name,data in bad_files.items()}
        bad = root/'wrong-shared.zip'
        bad_sha = zip_files(bad, 'TWXT2', bad_files)
        rejected(lambda: pack.shared(bad, bad_sha, exe, tests), 'changed shared manifest')
        unsafe = root/'unsafe.zip'
        with zipfile.ZipFile(unsafe, 'w') as z:
            z.writestr('../escape', b'bad')
        rejected(lambda: pack.read_zip(unsafe, pack.identity(unsafe.read_bytes())['sha256']),
                 'unsafe archive path')
        duplicate = root/'duplicate.zip'
        with zipfile.ZipFile(duplicate, 'w') as z:
            z.writestr('ONE.TXT', b'a')
            z.writestr('one.txt', b'b')
        rejected(lambda: pack.read_zip(duplicate, pack.identity(duplicate.read_bytes())['sha256']),
                 'DOS duplicate')
        print('PASS PIT repack unit checks=%d; synthetic fixtures, no native qualification'%checks)


if __name__ == '__main__':
    main()
