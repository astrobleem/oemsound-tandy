"""Focused synthetic MIDI native-package regressions; no native qualification."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
from types import SimpleNamespace
from unittest.mock import patch
from contextlib import redirect_stdout
import io
import json
import tempfile
import unittest
import zipfile
import zlib

HERE = Path(__file__).resolve().parent


def load(name, filename):
    loader = SourceFileLoader(name, str(HERE/filename))
    spec = spec_from_loader(loader.name, loader)
    module = module_from_spec(spec)
    loader.exec_module(module)
    return module


pack = load('midi_unit_builder', 'PACK.PY')
verify = load('midi_unit_verifier', 'VERIFY.PY')
sfx = load('midi_unit_sfx', 'SFX.PY')
fixtures = load('midi_unit_fixtures', 'TESTPIT.PY')


def dump(value):
    return (json.dumps(value, sort_keys=True)+'\n').encode()


def put(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


class MidiPackage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='midi-pack-unit-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.repo = self.root/'repo'
        _, _, _, _, files = fixtures.shared_fixture(self.root)
        self.files = files
        self.builds = json.loads(files['BUILD.JSN'])['native_builds']
        source_paths = {'driver': 'src/win30/DRIVER', 'raw': 'experimental/WINLIB30/SOURCE',
                        'midi': 'experimental/WINLIB30/SOURCE',
                        'mci': 'experimental/WINLIB30/SOURCE',
                        'launch': 'src/win30/LAUNCH', 'setup': 'src/win30/SETUP'}
        runtimes = dict(pack.CLOCK_FILES, driver='SOUND.DRV', launch='TPLAY.EXE', setup='SETUP.EXE')
        process = files['EVIDENCE/RAWPROC.JSN']
        self.stages = {}
        for label, record in self.builds.items():
            if label=='player':
                continue
            stage = self.root/label
            self.stages[label] = stage
            for name in record['sources']:
                data = files['SOURCE/'+label.upper()+'/'+name]
                put(stage/name, data)
                target = self.repo/source_paths[label]/name
                if label == 'setup' and name == 'SOUNDID.H':
                    target = self.repo/'src/win30/DRIVER/SOUNDID.H'
                put(target, data)
            put(stage/runtimes[label], files[runtimes[label]])
            put(stage/'BUILD.JSON', dump(record))
            put(stage/'PROCESS.JSON', process)
            put(stage/'BUILD.LOG', b'PASS MIDI THREE DLL BUILD\r\n')
        playerstage=self.root/'player';self.stages['player']=playerstage
        for name in self.builds['player']['sources']:
            data=files['SOURCE/PLAYER/'+name]
            put(playerstage/name,data);put(self.repo/'src/win30/PLAYER'/name,data)
        for member,original in (('PLAYERSR.JSN','SOURCES.JSN'),('PLAYER.JSN','RESULT.JSN'),
                                ('PLAYERPR.JSN','PROCESS.JSON'),('PLAYER.LOG','BUILD.LOG'),
                                ('PLAYER.MAP','MINIPLAY.MAP')):
            put(playerstage/original,files['EVIDENCE/'+member])
        put(playerstage/'MINIPLAY.EXE',files['MINIPLAY.EXE'])
        runtimes['player']='MINIPLAY.EXE'
        canonical = json.loads(files['BUILD.JSN'])['canonical_sound']
        put(self.repo/'src/win30/DRIVER/SOUNDID.JSON', dump(canonical))
        put(self.repo/'LICENSE', b'UNIT LICENSE')
        clients = json.loads(files['EVIDENCE/CLIENTS.JSN'])
        clientdir = self.root/'clients'
        put(clientdir/'PINS.JSN', dump(clients))
        for name in clients:
            put(clientdir/name, files[name])
        baseline = self.root/'baseline.zip'
        self.basefiles = {name: files.get(name, b'UNIT BASE '+name.encode()) for name in pack.RUNTIME}
        self.basefiles.update({'MIDICLK.DLL': b'OLD MIDI DLL', 'MCITNDY.DLL': b'OLD MCI DLL',
                               'SRC/MINIPLAY.C': b'UNCHANGED PLAYER SOURCE'})
        for record in self.builds.values():
            for name in record['sources']:
                self.basefiles['DEPS/'+name] = b'STALE BASE '+name.encode()
                self.basefiles['DEPS/OLD/'+name] = b'STALE DUPLICATE '+name.encode()
        with zipfile.ZipFile(baseline, 'w') as archive:
            for name, data in self.basefiles.items():
                archive.writestr('TWXT1/'+name, data)
        flat = {name: self.basefiles[name] for name in pack.RUNTIME}
        for label, name in runtimes.items():
            flat[name] = files[name]
        flat.update({name: files[name] for name in clients})
        flat.update({name: (HERE/name).read_bytes() for name in ('INSTALL.TXT','README.TXT')})
        rows = ['TWSETUP1']
        for name, data in sorted(flat.items()):
            kind = clients[name]['kind'] if name in clients else ('S' if name == 'SOUND.DRV' else 'A')
            rows.append('%s|%s|%s|%d|%08x'%(kind,name,name,len(data),zlib.crc32(data)))
        flat['PAYLOAD.LST'] = ('\r\n'.join(rows)+'\r\n').encode()
        sfxdir = self.root/'sfx-input'
        for name, data in flat.items():
            put(sfxdir/name, data)
        tests = json.loads(files['TESTS.JSN'])
        tests['installer'] = pack.identity(sfx.make(sfxdir/'SETUP.EXE', sfxdir))
        self.tests = self.root/'accepted.json'
        put(self.tests, dump(tests))
        self.args = SimpleNamespace(baseline=baseline, clients=clientdir, workspace=self.root,
                                    tests=self.tests, out=self.root/'out', source_commit='UNIT')
        for label, stage in self.stages.items():
            setattr(self.args, label, stage)
        self.basepin = pack.identity(baseline.read_bytes())['sha256']

    def build(self):
        with patch.object(pack, 'REPO', self.repo), patch.object(pack, 'BASE_SHA', self.basepin):
            with redirect_stdout(io.StringIO()):
                pack.build(self.args)
        with zipfile.ZipFile(self.args.out/'TWXT2.ZIP') as archive:
            return {name: archive.read(name) for name in archive.namelist()}

    def refused(self, function):
        with self.assertRaises((ValueError, FileNotFoundError)):
            function()

    def readback(self, files, current=False):
        files = dict(files)
        manifest = {name[6:]: pack.identity(data) for name, data in files.items()
                    if name != 'TWXT2/MANIFEST.JSN'}
        files['TWXT2/MANIFEST.JSN'] = dump(manifest)
        with patch.object(verify, 'REPO', self.repo):
            return verify.verify(files, current=current)

    def test_rebuilt_dlls_and_source_replace_baseline(self):
        files = self.build()
        for label, name in pack.CLOCK_FILES.items():
            self.assertEqual(files['TWXT2/'+name], self.files[name])
            self.assertNotIn(name, pack.RUNTIME)
            for source in self.builds[label]['sources']:
                self.assertEqual(files['TWXT2/DEPS/'+source],
                                 files['TWXT2/SOURCE/'+label.upper()+'/'+source])
                self.assertNotIn('TWXT2/DEPS/OLD/'+source, files)
        self.assertGreater(self.readback(files, current=True), 100)
        verify.verify_sfx((self.args.out/'TWSETUP.EXE').read_bytes(), files)

    def test_old_or_changed_native_dll_refused(self):
        for label, name in (('midi','MIDICLK.DLL'),('mci','MCITNDY.DLL')):
            with self.subTest(label=label):
                path = self.stages[label]/name
                expected = path.read_bytes()
                path.write_bytes(self.basefiles[name])
                self.refused(self.build)
                path.write_bytes(expected)

    def test_missing_pinned_required_inputs_refused(self):
        for label, name in (('midi','LTPBASE.H'),('midi','LTMATH.H'),
                            ('midi','MIDIMSG.H'),('midi','PITNOTES.H'),
                            ('mci','MIDISKIP.H'),('midi','TCXBASS.H'),
                            ('midi','PITLINK.H'),('midi','MIDICLK.C'),
                            ('mci','MCITNDY.C'),('mci','BLDMIDI.BAT')):
            with self.subTest(source=name):
                path = self.stages[label]/'BUILD.JSON'
                original = path.read_bytes()
                record = json.loads(original)
                del record['sources'][name]
                path.write_bytes(dump(record))
                self.refused(self.build)
                path.write_bytes(original)

    def test_stale_current_or_staged_source_refused(self):
        for path in (self.repo/'experimental/WINLIB30/SOURCE/MIDIMSG.H',
                     self.stages['mci']/'MIDISKIP.H'):
            with self.subTest(path=path):
                original = path.read_bytes()
                path.write_bytes(b'WRONG SOURCE')
                self.refused(self.build)
                path.write_bytes(original)

    def test_unfinished_native_process_refused(self):
        path = self.stages['midi']/'PROCESS.JSON'
        original = path.read_bytes()
        for key, value in (('return',1),('owned_child_stopped',False),
                           ('lease_released',False),('watchdog',True),('error','failed')):
            with self.subTest(key=key):
                process = json.loads(original)
                process[key] = value
                path.write_bytes(dump(process))
                self.refused(self.build)
        path.write_bytes(original)

    def test_different_native_build_process_refused(self):
        path = self.stages['mci']/'PROCESS.JSON'
        process = dict(json.loads(path.read_bytes()), pid=999)
        path.write_bytes(dump(process))
        recordpath = self.stages['mci']/'BUILD.JSON'
        record = json.loads(recordpath.read_bytes())
        record['common_process_identity'] = pack.identity(path.read_bytes())
        recordpath.write_bytes(dump(record))
        self.refused(self.build)

    def test_missing_or_failed_native_log_refused(self):
        path = self.stages['mci']/'BUILD.LOG'
        for data in (b'BUILD STARTED', b'FAIL MIDI THREE DLL BUILD\r\nPASS MIDI THREE DLL BUILD'):
            with self.subTest(log=data):
                path.write_bytes(data)
                self.refused(self.build)

    def test_unaccepted_midi_runtime_refused(self):
        original = self.tests.read_bytes()
        for key, name in (('midiclk','MIDICLK.DLL'),('mcitndy','MCITNDY.DLL')):
            for missing in (False, True):
                with self.subTest(key=key, missing=missing):
                    tests = json.loads(original)
                    if missing:
                        del tests[key]
                    else:
                        tests[key] = pack.identity(self.basefiles[name])
                    self.tests.write_bytes(dump(tests))
                    self.refused(self.build)
        self.tests.write_bytes(original)

    def test_pending_native_acceptance_refused(self):
        tests = json.loads(self.tests.read_bytes())
        tests['release_readiness']['status'] = 'PENDING'
        self.tests.write_bytes(dump(tests))
        self.refused(self.build)

    def test_packet_refuses_old_dll_or_stale_deps_with_new_manifest(self):
        files = self.build()
        for name in ('MIDICLK.DLL','MCITNDY.DLL','DEPS/MIDISKIP.H','DEPS/PITNOTES.H'):
            with self.subTest(member=name):
                changed = dict(files)
                changed['TWXT2/'+name] = b'OLD OR WRONG'
                self.refused(lambda: self.readback(changed))

    def test_packet_refuses_omitted_header_even_if_unpinned_copy_remains(self):
        files = self.build()
        for label, name in (('midi','LTMATH.H'),('mci','MIDISKIP.H')):
            with self.subTest(label=label):
                changed = dict(files)
                build = json.loads(changed['TWXT2/BUILD.JSN'])
                del build['native_builds'][label]['sources'][name]
                changed['TWXT2/BUILD.JSN'] = dump(build)
                changed['TWXT2/EVIDENCE/'+label.upper()+'.JSN'] = dump(build['native_builds'][label])
                self.refused(lambda: self.readback(changed))

    def test_missing_native_source_file_refused(self):
        (self.stages['mci']/'MIDISKIP.H').unlink()
        self.refused(self.build)

    def test_packet_refuses_unfinished_process_with_consistent_identity(self):
        files = self.build()
        build = json.loads(files['TWXT2/BUILD.JSN'])
        process = dump({'return': 0, 'owned_child_stopped': True,
                        'lease_released': True, 'watchdog': True})
        for label in pack.CLOCK_SOURCES:
            files['TWXT2/EVIDENCE/'+label.upper()+'PROC.JSN'] = process
            build['native_builds'][label]['common_process_identity'] = pack.identity(process)
            files['TWXT2/EVIDENCE/'+label.upper()+'.JSN'] = dump(build['native_builds'][label])
        files['TWXT2/BUILD.JSN'] = dump(build)
        self.refused(lambda: self.readback(files))

    def test_packet_refuses_unpinned_transitive_include(self):
        files = self.build()
        build = json.loads(files['TWXT2/BUILD.JSN'])
        data = b'#include "EXTRA.H"\r\n'
        files['TWXT2/SOURCE/MIDI/MIDICLK.C'] = data
        files['TWXT2/DEPS/MIDICLK.C'] = data
        build['native_builds']['midi']['sources']['MIDICLK.C'] = pack.identity(data)
        files['TWXT2/BUILD.JSN'] = dump(build)
        files['TWXT2/EVIDENCE/MIDI.JSN'] = dump(build['native_builds']['midi'])
        self.refused(lambda: self.readback(files))


if __name__ == '__main__':
    unittest.main()
