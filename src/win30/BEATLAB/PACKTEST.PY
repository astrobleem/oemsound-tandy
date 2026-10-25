"""Packaging regression tests use temporary synthetic NE headers, never native execution."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
import json
import struct
import tempfile
import unittest
import zipfile

HERE = Path(__file__).resolve().parent
loader = SourceFileLoader('beats_pack_tested', str(HERE / 'PACK.PY'))
spec = spec_from_loader(loader.name, loader)
pack = module_from_spec(spec)
loader.exec_module(pack)


class NativeReceiptTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='beats-pack-test-')
        self.root = Path(self.temp.name)
        self.here = self.root / 'source'
        self.build = self.root / 'build'
        self.here.mkdir()
        self.build.mkdir()
        for name in pack.PRODUCTION:
            raw = (HERE / name).read_bytes()
            (self.here / name).write_bytes(raw)
            (self.build / name).write_bytes(pack.crlf(raw))
        # A structural test fixture only: not executable product output.
        self.exe = bytearray(128)
        self.exe[:2] = b'MZ'
        struct.pack_into('<I', self.exe, 0x3c, 64)
        self.exe[64:66] = b'NE'
        struct.pack_into('<H', self.exe, 64+0x3e, 0x0300)
        (self.build / 'BEATS.EXE').write_bytes(self.exe)
        self.record = {'target': '8086 Windows 3.0 real mode; MSC6/LINK4',
                       'status': 'native compile/link only; execution separate',
                       'sources': {n: pack.pin((self.build/n).read_bytes()) for n in pack.PRODUCTION},
                       'source_originals': {n: pack.pin((self.here/n).read_bytes()) for n in pack.PRODUCTION},
                       'runtime': pack.pin(self.exe)}
        self.save_record()
        self.process = {'lease_acquired': True, 'lease_released': True,
                        'owned_child_stopped': True, 'return': 0}
        self.save_process()

    def tearDown(self):
        self.temp.cleanup()

    def save_record(self):
        (self.build/'BUILD.JSON').write_bytes(pack.json_bytes(self.record))

    def save_process(self):
        (self.build/'PROCESS.JSON').write_bytes(pack.json_bytes(self.process))

    def test_current_and_staged_inputs_are_bound(self):
        exe, sources, receipt = pack.native_inputs(self.build, self.here)
        self.assertEqual(exe, self.exe)
        self.assertEqual(len(sources), len(pack.PRODUCTION))
        self.assertEqual(json.loads(receipt), self.record)

    def test_stale_current_source_refused(self):
        (self.here/'BEATS.C').write_bytes(b'changed after native build')
        with self.assertRaisesRegex(ValueError, 'Current source differs'):
            pack.native_inputs(self.build, self.here)

    def test_staged_bytes_and_normalization_are_both_checked(self):
        changed = b'other staged input\r\n'
        (self.build/'BEATS.C').write_bytes(changed)
        self.record['sources']['BEATS.C'] = pack.pin(changed)
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'Compiled source differs'):
            pack.native_inputs(self.build, self.here)

    def test_missing_source_inventory_refused(self):
        del self.record['source_originals']['MIDIAPI.H']
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'Incomplete compiled source'):
            pack.native_inputs(self.build, self.here)

    def test_changed_native_runtime_refused(self):
        (self.build/'BEATS.EXE').write_bytes(bytes(self.exe)+b'changed')
        with self.assertRaisesRegex(ValueError, 'BEATS.EXE differs'):
            pack.native_inputs(self.build, self.here)

    def test_wrong_ne_target_refused_even_with_matching_hash(self):
        struct.pack_into('<H', self.exe, 64+0x3e, 0x030a)
        (self.build/'BEATS.EXE').write_bytes(self.exe)
        self.record['runtime'] = pack.pin(self.exe)
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'Windows 3.0 NE'):
            pack.native_inputs(self.build, self.here)

    def test_missing_or_failed_guard_refused(self):
        for key, value in (('lease_acquired', False), ('lease_released', False),
                           ('owned_child_stopped', False), ('return', 1),
                           ('error', 'blocked'), ('watchdog', True)):
            with self.subTest(key=key):
                self.process = {'lease_acquired': True, 'lease_released': True,
                                'owned_child_stopped': True, 'return': 0}
                self.process[key] = value
                self.save_process()
                with self.assertRaisesRegex(ValueError, 'shared lease'):
                    pack.native_inputs(self.build, self.here)


class PacketTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.base = pack.read_kit(pack.ROOT/'artifacts/win30/MML3AUTH.ZIP')

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='beats-packet-test-')
        self.root = Path(self.temp.name)
        self.demos = {'BEATS/'+stem+ext: b'synthetic format fixture\r\n'
                      for stem in pack.STEMS for ext in ('.BTS', '.MML')}
        self.sources = {pack.SOURCE_PREFIX+n: pack.crlf((HERE/n).read_bytes())
                        for n in pack.PRODUCTION}
        self.files = pack.assemble(self.raw, self.base, b'synthetic client fixture',
                                   self.sources, b'{"test_fixture":true}', self.demos)
        self.path = self.root/'MML3AUTH.ZIP'
        pack.write_packet(self.path, self.files)

    def tearDown(self):
        self.temp.cleanup()

    def test_all_unrelated_members_and_shared_bytes_are_preserved(self):
        record = json.loads(self.files['BUILD.JSN'])
        replaced = {'BEATS/BEATS.EXE','README.TXT','INSTALL.TXT','BUILD.JSN','MANIFEST.JSN'}
        self.assertEqual(set(record['preserved']), set(self.base)-replaced)
        for name in record['preserved']:
            self.assertEqual(self.files[name], self.base[name], name)
        for name in ('TWSETUP.EXE','TWXT2.ZIP','BEATS/MIDIMAP.DRV',
                     'FIDDLE/MIDIMAP.DRV','FIDDLE/MMLFIDL.EXE'):
            self.assertEqual(self.files[name], self.base[name])
        self.assertFalse(record['qualification']['historical_unchanged_client_gate'])
        pack.verify_packet(self.path, self.files, self.base)

    def test_corrupt_shared_or_fiddle_or_mapper_is_refused(self):
        for name in ('TWSETUP.EXE','TWXT2.ZIP','FIDDLE/MMLFIDL.EXE','BEATS/MIDIMAP.DRV'):
            with self.subTest(name=name):
                broken = dict(self.files)
                broken[name] = b'wrong shared bytes'
                pack.write_packet(self.path, broken)
                with self.assertRaisesRegex(ValueError, 'content differs'):
                    pack.verify_packet(self.path, self.files, self.base)

    def test_original_receipt_and_new_manifest_are_complete(self):
        self.assertEqual(self.files['EVIDENCE/BEFORE.JSN'], self.base['BUILD.JSN'])
        manifest = json.loads(self.files['MANIFEST.JSN'])
        self.assertEqual(set(manifest), set(self.files)-{'MANIFEST.JSN'})
        self.assertIn('BEATS/FIVE.BTS', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'BEATS.C', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'HOSTTEST.PY', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/core.c', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/mapper.c', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/README.md', manifest)
        self.assertFalse(any('__pycache__' in n or n.lower().endswith(('.pyc', '.exe', '.obj'))
                             for n in manifest if n.startswith(pack.SOURCE_PREFIX)))

    def test_missing_demo_refused(self):
        del self.demos['BEATS/FIVE.BTS']
        with self.assertRaisesRegex(ValueError, 'Demo inventory'):
            pack.assemble(self.raw, self.base, b'fixture', self.sources, b'{}', self.demos)

    def test_missing_native_build_creates_no_candidate(self):
        out = self.root/'must-not-exist'
        with self.assertRaises(FileNotFoundError):
            pack.build(pack.ROOT/'artifacts/win30/MML3AUTH.ZIP', self.root/'absent-native-build', out)
        self.assertFalse(out.exists())

    def test_real_host_generated_demos_are_ready_formats(self):
        files = pack.make_demos(self.root/'host-demos')
        self.assertEqual(set(files), set(self.demos))
        for stem in pack.STEMS:
            bts, mml = files['BEATS/'+stem+'.BTS'], files['BEATS/'+stem+'.MML']
            self.assertTrue(bts.startswith(b'DWA1\r\n'))
            self.assertLess(len(bts), 512)
            values = [int(x) for x in bts.split()[1:]]
            self.assertEqual(len(values), 45)
            self.assertEqual(values[1], int(stem=='FIVE'))
            self.assertTrue(mml.startswith(b'MML3' if stem=='FIVE' else b'MML2'))
            self.assertEqual(b'[P]' in mml, stem=='FIVE')


if __name__ == '__main__':
    unittest.main()
