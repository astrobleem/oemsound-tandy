"""Packaging regression tests use temporary synthetic NE headers, never native execution."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
import json
import struct
import tempfile
import unittest
from unittest.mock import patch
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
        self.repo = self.root / 'repo'
        self.here = self.repo / 'src/win30/BEATLAB'
        self.build = self.root / 'build'
        self.here.mkdir(parents=True)
        self.build.mkdir()
        for name, relative in pack.SOURCE_PATHS.items():
            raw = (pack.ROOT / relative).read_bytes()
            source = self.repo / relative
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes(raw)
            (self.build / name).write_bytes(pack.crlf(raw))
        (self.repo/'src/win30/FIDDLE/MIDIAPI.H').write_bytes((self.here/'MIDIAPI.H').read_bytes())
        # A structural test fixture only: not executable product output.
        self.exe = bytearray(128)
        self.exe[:2] = b'MZ'
        struct.pack_into('<I', self.exe, 0x3c, 64)
        self.exe[64:66] = b'NE'
        struct.pack_into('<H', self.exe, 64+0x3e, 0x0300)
        self.runtimes = {name: bytes(self.exe) + name.encode('ascii') for name in pack.RUNTIME_PATHS}
        self.exe = bytearray(self.runtimes['BEATS.EXE'])
        for name, data in self.runtimes.items():
            (self.build / name).write_bytes(data)
        self.record = {'target': '8086 Windows 3.0 real mode; MSC6/LINK4',
                       'status': 'native compile/link only; execution separate',
                       'sources': {n: pack.pin((self.build/n).read_bytes()) for n in pack.PRODUCTION},
                       'source_originals': {n: pack.pin((self.repo/relative).read_bytes()) for n, relative in pack.SOURCE_PATHS.items()},
                       'source_paths': dict(pack.SOURCE_PATHS),
                       'runtimes': {n: pack.pin(data) for n, data in self.runtimes.items()},
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
        runtimes, sources, receipt = pack.native_inputs(self.build, self.here)
        self.assertEqual(runtimes, self.runtimes)
        self.assertEqual(len(sources), len(pack.PRODUCTION)+1)
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
        with self.assertRaisesRegex(ValueError, 'Incomplete or incorrect compiled source'):
            pack.native_inputs(self.build, self.here)

    def test_changed_native_runtime_refused(self):
        (self.build/'BEATS.EXE').write_bytes(bytes(self.exe)+b'changed')
        with self.assertRaisesRegex(ValueError, 'runtime differs'):
            pack.native_inputs(self.build, self.here)

    def test_wrong_ne_target_refused_even_with_matching_hash(self):
        struct.pack_into('<H', self.exe, 64+0x3e, 0x030a)
        (self.build/'BEATS.EXE').write_bytes(self.exe)
        self.record['runtime'] = self.record['runtimes']['BEATS.EXE'] = pack.pin(self.exe)
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'Windows 3.0 NE'):
            pack.native_inputs(self.build, self.here)

    def test_fiddle_source_and_header_are_bound(self):
        (self.repo/'src/win30/FIDDLE/MMLCORE.C').write_bytes(b'changed parser')
        with self.assertRaisesRegex(ValueError, 'Current source differs'):
            pack.native_inputs(self.build, self.here)
        (self.repo/'src/win30/FIDDLE/MMLCORE.C').write_bytes((pack.ROOT/'src/win30/FIDDLE/MMLCORE.C').read_bytes())
        (self.repo/'src/win30/FIDDLE/MIDIAPI.H').write_bytes(b'stale copied header')
        with self.assertRaisesRegex(ValueError, 'MIDI headers differ'):
            pack.native_inputs(self.build, self.here)

    def test_source_map_cannot_remap_parser(self):
        self.record['source_paths']['MMLCORE.C'] = 'src/win30/BEATLAB/BEATS.C'
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'incorrect compiled source'):
            pack.native_inputs(self.build, self.here)

    def test_all_three_runtime_pins_and_legacy_alias_are_required(self):
        del self.record['runtimes']['MMLFIDL.EXE']
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'runtime inventory'):
            pack.native_inputs(self.build, self.here)
        self.record['runtimes'] = {n:pack.pin(data) for n,data in self.runtimes.items()}
        self.record['runtime'] = pack.pin(b'wrong legacy alias')
        self.save_record()
        with self.assertRaisesRegex(ValueError, 'runtime inventory'):
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
        self.sources = {'SOURCE/'+relative: pack.crlf((pack.ROOT/relative).read_bytes())
                        for n, relative in pack.SOURCE_PATHS.items()}
        self.sources['SOURCE/src/win30/FIDDLE/MIDIAPI.H'] = self.sources[pack.SOURCE_PREFIX+'MIDIAPI.H']
        self.runtimes = {n: ('synthetic client fixture '+n).encode('ascii') for n in pack.RUNTIME_PATHS}
        self.files = pack.assemble(self.raw, self.base, self.runtimes,
                                   self.sources, b'{"test_fixture":true}', self.demos)
        self.path = self.root/'MML3AUTH.ZIP'
        pack.write_packet(self.path, self.files)

    def tearDown(self):
        self.temp.cleanup()

    def test_all_unrelated_members_and_shared_bytes_are_preserved(self):
        record = json.loads(self.files['BUILD.JSN'])
        replaced = pack.REPLACED
        self.assertEqual(set(record['preserved']), set(self.base)-replaced)
        for name in record['preserved']:
            self.assertEqual(self.files[name], self.base[name], name)
        for name in ('TWSETUP.EXE','TWXT2.ZIP'):
            self.assertEqual(self.files[name], self.base[name])
        self.assertEqual(self.files['BEATS/MIDIMAP.DRV'], self.files['FIDDLE/MIDIMAP.DRV'])
        self.assertEqual(self.files['FIDDLE/MMLFIDL.EXE'], self.runtimes['MMLFIDL.EXE'])
        for name in self.base:
            if name.startswith('SOURCE/src/win30/MMLFIDL/'):
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
        self.assertIn('SOURCE/src/win30/FIDDLE/MMLCORE.C', manifest)
        self.assertIn('SOURCE/src/win30/FIDDLE/MMLFIDL.C', manifest)
        self.assertEqual(self.files['SOURCE/src/win30/FIDDLE/MIDIAPI.H'], self.sources[pack.SOURCE_PREFIX+'MIDIAPI.H'])
        self.assertIn(pack.SOURCE_PREFIX+'HOSTTEST.PY', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/core.c', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/mapper.c', manifest)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/README.md', manifest)
        self.assertFalse(any('__pycache__' in n or n.lower().endswith(('.pyc', '.exe', '.obj'))
                             for n in manifest if n.startswith(pack.SOURCE_PREFIX)))

    def test_tampered_paired_mapper_is_refused_even_as_expected(self):
        broken = dict(self.files)
        broken['FIDDLE/MIDIMAP.DRV'] = b'other mapper'
        pack.write_packet(self.path, broken)
        with self.assertRaisesRegex(ValueError, 'Updated runtime pin differs'):
            pack.verify_packet(self.path, broken, self.base)

    def test_low_speaker_guidance_is_precise(self):
        self.assertIn(b'26-96', self.files['BEATS/README.TXT'])
        self.assertIn(b'55/65/73 Hz', self.files['BEATS/README.TXT'])
        self.assertIn(b'PSG tones remain A2-C7 (MIDI 45-96)', self.files['BEATS/README.TXT'])
        for name in ('README.TXT','INSTALL.TXT','BEATS/README.TXT'):
            self.assertNotIn(b'\n', self.files[name].replace(b'\r\n', b''), name)

    def test_pitcore_is_unchanged_regression_input_not_compiled_source(self):
        member = 'SOURCE/src/win30/DRIVER/PITCORE.H'
        original = (pack.ROOT/'src/win30/DRIVER/PITCORE.H').read_bytes()
        self.assertEqual(self.files[member], original)
        record = json.loads(self.files['BUILD.JSN'])
        self.assertEqual(record['regression_inputs'][member], pack.pin(original))
        self.assertEqual(len(pack.SOURCE_PATHS), 21)
        self.assertNotIn('PITCORE.H', pack.SOURCE_PATHS)
        broken = dict(self.files)
        broken[member] = b'changed regression header'
        pack.write_packet(self.path, broken)
        with self.assertRaisesRegex(ValueError, 'Regression input differs'):
            pack.verify_packet(self.path, broken, self.base)

    def test_missing_demo_refused(self):
        del self.demos['BEATS/FIVE.BTS']
        with self.assertRaisesRegex(ValueError, 'Demo inventory'):
            pack.assemble(self.raw, self.base, self.runtimes, self.sources, b'{}', self.demos)

    def test_missing_native_build_creates_no_candidate(self):
        out = self.root/'must-not-exist'
        with self.assertRaises(FileNotFoundError):
            pack.build(pack.ROOT/'artifacts/win30/MML3AUTH.ZIP', self.root/'absent-native-build', out)
        self.assertFalse(out.exists())

    def test_build_demos_use_validated_captured_source_not_live_source(self):
        captured = dict(self.sources)
        member = pack.SOURCE_PREFIX+'BEATPAT.C'
        old = b'{120,90,92,88,90}'
        new = b'{130,90,92,88,90}'
        self.assertIn(old, captured[member])
        # Model source drift after validation: the validated build snapshot has
        # different demo metadata from live HERE. Only captured bytes may win.
        captured[member] = captured[member].replace(old, new, 1)
        out = self.root/'captured-demo-candidate'
        with patch.object(pack, 'native_inputs', return_value=(self.runtimes, captured, b'{"test_fixture":true}')):
            pack.build(pack.ROOT/'artifacts/win30/MML3AUTH.ZIP', self.root/'native-capture', out)
        with zipfile.ZipFile(out/'MML3AUTH.ZIP') as archive:
            starter = archive.read('MML3AUTH/BEATS/STARTER.BTS')
            packaged_source = archive.read('MML3AUTH/'+member)
        self.assertEqual(int(starter.split()[1]), 130)
        self.assertEqual(packaged_source, captured[member])
        self.assertEqual((out/'DEMSRC/BEATPAT.C').read_bytes(), captured[member])
        self.assertNotIn(new, (HERE/'BEATPAT.C').read_bytes())

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
            if stem == 'FIVE':
                self.assertEqual(values[-8:], [33,33,36,33,33,36,38,33])
            self.assertTrue(mml.startswith(b'MML3' if stem=='FIVE' else b'MML2'))
            self.assertEqual(b'[P]' in mml, stem=='FIVE')


if __name__ == '__main__':
    unittest.main()
