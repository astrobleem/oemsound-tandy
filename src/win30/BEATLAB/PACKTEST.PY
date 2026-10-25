"""Packaging regression tests use temporary synthetic NE headers, never native execution."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
from importlib.util import module_from_spec, spec_from_loader
import json
import copy
import os
import subprocess
import sys
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


def unit_kit(kit):
    """Use authenticated current bytes as a temporary preservation fixture.

    Once public assets are schema2, strip only their declared additions in
    memory; these synthetic baseline fixtures never leave TemporaryDirectory.
    Production read_kit still accepts only pinned assets/explicit bank baselines.
    """
    raw, files = pack.read_kit(pack.ROOT/'artifacts/win30'/(kit+'.ZIP'), kit)
    if 'EVIDENCE/BEFORE.JSN' in files:
        record = json.loads(files['BUILD.JSN'])
        for name in record['changes']:
            if name not in pack.replacement_paths(kit):
                files.pop(name, None)
        files['MANIFEST.JSN'] = pack.json_bytes({n:pack.pin(b) for n,b in files.items() if n != 'MANIFEST.JSN'})
    return raw, files


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
        cls.raw, cls.base = unit_kit('MML3AUTH')

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
        with patch.object(pack,'read_kit',return_value=(self.raw,self.base)):
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
        with patch.object(pack, 'native_inputs', return_value=(self.runtimes, captured, b'{"test_fixture":true}')), \
             patch.object(pack, 'read_kit', return_value=(self.raw,self.base)):
            pack.build(pack.ROOT/'artifacts/win30/MML3AUTH.ZIP', self.root/'native-capture', out)
        with zipfile.ZipFile(out/'MML3AUTH.ZIP') as archive:
            starter = archive.read('MML3AUTH/BEATS/STARTER.BTS')
            packaged_source = archive.read('MML3AUTH/'+member)
        self.assertEqual(int(starter.split()[1]), 130)
        self.assertEqual(packaged_source, captured[member])
        self.assertEqual((out/'DEMSRC/BEATPAT.C').read_bytes(), captured[member])
        self.assertNotIn(new, (HERE/'BEATPAT.C').read_bytes())

    def test_finished_input_is_refused_before_native_or_compiler_work(self):
        out = self.root/'no-reapply-output'
        finished = dict(self.files)
        with patch.object(pack,'read_kit',return_value=(self.raw,finished)), \
             patch.object(pack,'native_inputs',side_effect=AssertionError('No native receipt work for finished input')), \
             patch.object(pack,'make_demos',side_effect=AssertionError('No compiler for finished input')):
            with self.assertRaisesRegex(ValueError,'preceding bank baseline'):
                pack.build('finished-kit.zip','native-build',out)
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
            if stem == 'FIVE':
                self.assertEqual(values[-8:], [33,33,36,33,33,36,38,33])
            self.assertTrue(mml.startswith(b'MML3' if stem=='FIVE' else b'MML2'))
            self.assertEqual(b'[P]' in mml, stem=='FIVE')


class LayoutAndPublishedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scratch = tempfile.TemporaryDirectory(prefix='beats-two-kit-test-')
        cls.root = Path(cls.scratch.name)
        cls.bases = {kit:unit_kit(kit) for kit in pack.KITS}
        cls.demos = pack.make_demos(cls.root/'real-demo-helper')
        cls.sources = {'SOURCE/'+relative:pack.crlf((pack.ROOT/relative).read_bytes())
                       for relative in pack.SOURCE_PATHS.values()}
        cls.sources['SOURCE/src/win30/FIDDLE/MIDIAPI.H'] = cls.sources[pack.SOURCE_PREFIX+'MIDIAPI.H']
        ne = bytearray(128)
        ne[:2] = b'MZ'
        struct.pack_into('<I',ne,0x3c,64)
        ne[64:66] = b'NE'
        struct.pack_into('<H',ne,64+0x3e,0x0300)
        # Temporary structural fixtures only; never executed or distributed.
        cls.runtimes = {n:bytes(ne)+n.encode('ascii') for n in pack.RUNTIME_PATHS}
        cls.native = {'target':'8086 Windows 3.0 real mode; MSC6/LINK4',
                      'status':'native compile/link only; execution separate',
                      'source_paths':dict(pack.SOURCE_PATHS),
                      'sources':{n:pack.pin(cls.sources['SOURCE/'+rel]) for n,rel in pack.SOURCE_PATHS.items()},
                      'source_originals':{n:pack.pin((pack.ROOT/rel).read_bytes()) for n,rel in pack.SOURCE_PATHS.items()},
                      'runtimes':{n:pack.pin(data) for n,data in cls.runtimes.items()},
                      'runtime':pack.pin(cls.runtimes['BEATS.EXE'])}
        cls.release = copy.deepcopy(pack.release_record())
        cls.release['beatlab_baselines'] = {kit+'.ZIP':pack.pin(raw) for kit,(raw,base) in cls.bases.items()}
        cls.files = {}
        for kit,(raw,base) in cls.bases.items():
            cls.files[kit] = pack.assemble(raw,base,cls.runtimes,cls.sources,
                pack.json_bytes(cls.native),cls.demos,kit=kit,regression=cls.bases['MML3AUTH'])

    @classmethod
    def tearDownClass(cls):
        cls.scratch.cleanup()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='beats-published-test-')
        self.out = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, kit, files):
        path = self.out/(kit+'.ZIP')
        pack.write_packet(path,files,kit)
        return path

    def reindex(self, files, record=None):
        if record is not None:
            files['BUILD.JSN'] = pack.json_bytes(record)
        files['MANIFEST.JSN'] = pack.json_bytes({n:pack.pin(data) for n,data in files.items() if n != 'MANIFEST.JSN'})

    def test_two_layouts_have_identical_app_mapper_and_demos(self):
        author,flat = self.files['MML3AUTH'],self.files['PITEXPR']
        self.assertEqual(author['BEATS/BEATS.EXE'],flat['BEATS.EXE'])
        self.assertEqual(author['BEATS/MIDIMAP.DRV'],flat['MIDIMAP.DRV'])
        self.assertEqual(author['FIDDLE/MIDIMAP.DRV'],flat['MIDIMAP.DRV'])
        self.assertNotIn('BEATS/BEATS.EXE',flat)
        for stem in pack.STEMS:
            for ext in ('.BTS','.MML'):
                self.assertEqual(author['BEATS/'+stem+ext],flat[stem+ext])
        for kit,files in self.files.items():
            base = self.bases[kit][1]
            record = json.loads(files['BUILD.JSN'])
            self.assertEqual(set(record['preserved']),set(base)-pack.replacement_paths(kit))
            for name in record['preserved']:
                self.assertEqual(files[name],base[name],kit+':'+name)
            for name in ('TWSETUP.EXE','TWXT2.ZIP')+pack.KITS[kit]['siblings']:
                self.assertEqual(files[name],base[name])
            self.assertTrue(files[pack.KITS[kit]['guide']].startswith(b'Tandy Beats Lab'))

    def test_finished_verifier_needs_no_base_archive_or_compiler(self):
        for kit,files in self.files.items():
            with self.subTest(kit=kit):
                path = self.write(kit,files)
                with patch.object(pack,'read_kit',side_effect=AssertionError('No baseline ZIP reads allowed')), \
                     patch.object(pack,'make_demos',side_effect=AssertionError('No compiler allowed')):
                    self.assertEqual(pack.verify_release_packet(path,kit,release=self.release,expected=pack.pin(path.read_bytes())),
                                     pack.pin(path.read_bytes()))

    def test_flat_guides_and_current_source_closure(self):
        flat = self.files['PITEXPR']
        self.assertIn(b'C:\\PITEXPR\\BEATS.EXE',flat['INSTALL.TXT'])
        self.assertIn(b'BEATS.TXT',flat['INSTALL.TXT'])
        self.assertNotIn(b'C:\\MML3AUTH\\BEATS',flat['INSTALL.TXT'])
        for name in pack.FROZEN_ALIASES:
            self.assertEqual(flat[name],self.bases['MML3AUTH'][1][name])
        for name in self.sources:
            self.assertEqual(flat[name],self.sources[name])
        self.assertIn('SOURCE/src/win30/DRIVER/PITCORE.H',flat)
        self.assertIn(pack.SOURCE_PREFIX+'HOSTTEST.PY',flat)
        self.assertIn(pack.SOURCE_PREFIX+'TESTS/legacy_parser.c',flat)

    def test_current_checkout_drift_refuses_stale_packaged_runtime(self):
        files = self.files['PITEXPR']
        path = self.write('PITEXPR',files)
        checkout = self.out/'checkout'
        for name,data in files.items():
            if name.startswith('SOURCE/'):
                target = checkout/name[len('SOURCE/'):]
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(data)
        with patch.object(pack,'ROOT',checkout):
            pack.verify_release_packet(path,'PITEXPR',release=self.release,current=True)
            for relative in ('src/win30/BEATLAB/BEATS.C',
                             'src/win30/FIDDLE/MIDIAPI.H','src/win30/DRIVER/PITCORE.H'):
                target = checkout/relative
                original = target.read_bytes()
                target.write_bytes(original+b'\r\n/* changed after packaging */\r\n')
                with self.subTest(source=relative):
                    with self.assertRaisesRegex(ValueError,'Current'):
                        pack.verify_release_packet(path,'PITEXPR',release=self.release,current=True)
                target.write_bytes(original)

    def test_explicit_bank_baseline_remains_accepted_after_asset_promotion(self):
        release = copy.deepcopy(self.release)
        for kit in pack.KITS:
            release['assets'][kit+'.ZIP'] = pack.pin(b'synthetic finished release fixture '+kit.encode('ascii'))
        with patch.object(pack,'release_record',return_value=release):
            for kit in pack.KITS:
                raw,files = pack.read_kit(pack.ROOT/'artifacts/win30'/(kit+'.ZIP'),kit)
                self.assertEqual(pack.pin(raw),release['beatlab_baselines'][kit+'.ZIP'])
                self.assertIn('TWSETUP.EXE',files)

    def test_read_kit_refuses_stale_shared_payload_even_with_approved_outer_pin(self):
        for kit in pack.KITS:
            with self.subTest(kit=kit):
                files = dict(self.bases[kit][1])
                files['TWXT2.ZIP'] = b'synthetic downlevel shared payload'
                self.reindex(files)
                path = self.write(kit,files)
                release = copy.deepcopy(self.release)
                release['beatlab_baselines'][kit+'.ZIP'] = pack.pin(path.read_bytes())
                with patch.object(pack,'release_record',return_value=release):
                    with self.assertRaisesRegex(ValueError,'shared payload mixing'):
                        pack.read_kit(path,kit)

    def test_published_shared_mixing_refused_with_self_consistent_internal_pins(self):
        files = dict(self.files['PITEXPR'])
        record = json.loads(files['BUILD.JSN'])
        files['TWXT2.ZIP'] = b'synthetic downlevel shared runtime'
        record['preserved']['TWXT2.ZIP'] = pack.pin(files['TWXT2.ZIP'])
        self.reindex(files,record)
        with self.assertRaisesRegex(ValueError,'shared payload mixing'):
            pack.verify_release_packet(self.write('PITEXPR',files),'PITEXPR',release=self.release)

    def test_published_compiled_header_pin_and_alias_are_enforced(self):
        for member,error in ((pack.SOURCE_PREFIX+'MIDIAPI.H','compiled source pin'),
                             ('SOURCE/src/win30/FIDDLE/MIDIAPI.H','MIDI source headers')):
            with self.subTest(member=member):
                files = dict(self.files['PITEXPR'])
                record = json.loads(files['BUILD.JSN'])
                files[member] = files[member][:-1]
                record['changes'][member] = pack.pin(files[member])
                self.reindex(files,record)
                with self.assertRaisesRegex(ValueError,error):
                    pack.verify_release_packet(self.write('PITEXPR',files),'PITEXPR',release=self.release)

    def test_published_frozen_header_sizes_are_enforced(self):
        files = dict(self.files['PITEXPR'])
        member = 'SOURCE/src/win30/BEATMML/INSTR.H'
        files[member] = files[member][:-1]
        record = json.loads(files['BUILD.JSN'])
        record['changes'][member] = pack.pin(files[member])
        self.reindex(files,record)
        with self.assertRaisesRegex(ValueError,'regression source pin'):
            pack.verify_release_packet(self.write('PITEXPR',files),'PITEXPR',release=self.release)

    def test_published_bad_demo_range_and_mml_mode_are_refused(self):
        for name,data,error in (('FIVE.BTS',b'DWA1\r\n88 1 0 0 0\r\n'+b'0 '*32+b'25 33 36 33 33 36 38 33\r\n','note range'),
                                ('FIVE.MML',self.files['PITEXPR']['FIVE.MML'].replace(b'MML3',b'MML2',1),'MML/pattern')):
            with self.subTest(name=name):
                files = dict(self.files['PITEXPR'])
                record = json.loads(files['BUILD.JSN'])
                files[name] = data
                record['changes'][name] = record['demos'][name] = pack.pin(data)
                self.reindex(files,record)
                with self.assertRaisesRegex(ValueError,error):
                    pack.verify_release_packet(self.write('PITEXPR',files),'PITEXPR',release=self.release)

    def test_extracted_flat_source_runs_its_host_regressions(self):
        source = self.out/'extracted'
        for name,data in self.files['PITEXPR'].items():
            if name.startswith('SOURCE/'):
                path = source/name
                path.parent.mkdir(parents=True,exist_ok=True)
                path.write_bytes(data)
        runner = source/pack.SOURCE_PREFIX/'HOSTTEST.PY'
        result = subprocess.run([sys.executable,str(runner),'--out',str(self.out/'host-checks')],
                                capture_output=True,text=True,timeout=60)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        report = json.loads((self.out/'host-checks/RESULT.JSON').read_bytes())
        self.assertEqual(report['status'],'PASS')
        self.assertTrue(report['old_range_trace_identical'])
        self.assertEqual(report['audio']['silent_attacks'],0)
        self.assertEqual(report['audio']['clipped_samples'],0)


if __name__ == '__main__':
    unittest.main()
