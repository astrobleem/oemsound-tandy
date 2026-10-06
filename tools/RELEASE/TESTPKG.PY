#!/usr/bin/env python3
"""Package regression tests: provenance, corruption, exclusions and determinism."""
import importlib.machinery
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import textwrap
import unittest
import zipfile

loader = importlib.machinery.SourceFileLoader('pack', str(Path(__file__).with_name('PACK.PY')))
spec = importlib.util.spec_from_loader(loader.name, loader)
pack = importlib.util.module_from_spec(spec)
loader.exec_module(pack)
SHA = '7b994c5d0072a69840767f1898aecbaea675dc7f'


class Packages(unittest.TestCase):
    def test_original_and_deterministic_packages(self):
        with tempfile.TemporaryDirectory() as temp:
            a, b = Path(temp) / 'a', Path(temp) / 'b'
            pack.build(pack.ROOT, a, SHA)
            pack.build(pack.ROOT, b, SHA)
            self.assertEqual({p.name: p.read_bytes() for p in a.iterdir()},
                             {p.name: p.read_bytes() for p in b.iterdir()})
            pack.verify(a, SHA)
            report = json.loads((a / 'BUILD.JSON').read_bytes())
            self.assertFalse(report['native_compilation'])
            self.assertFalse(report['fresh_native_qa'])
            for name, original in json.loads(pack.INPUTS.read_bytes())['archives'].items():
                self.assertEqual((a / name).read_bytes(),
                                 (pack.ROOT / original['path']).read_bytes())
            for kit in ('SOUNDP01', 'SOUNDP02'):
                with zipfile.ZipFile(a / (kit + '.ZIP')) as archive:
                    rows = json.loads(archive.read('FILES.JSON'))
                    self.assertEqual(set(archive.namelist()), set(rows) | {'FILES.JSON'})
                    for name, metadata in rows.items():
                        self.assertEqual(pack.identity(archive.read(name)), metadata)
                    self.assertIn(b'runtime binaries', archive.read('SCOPE.TXT'))
                    self.assertFalse(any(n.lower().endswith(('.exe', '.drv'))
                                         for n in archive.namelist()))
            with self.assertRaises(ValueError):
                pack.verify(a, '0' * 40)
            (a / 'WININST12.ZIP').write_bytes(b'corrupt')
            with self.assertRaises(ValueError):
                pack.verify(a, SHA)

    def test_corrupt_input_and_existing_output(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'root'
            first = next(iter(json.loads(pack.INPUTS.read_bytes())['archives'].values()))
            target = root / first['path']
            target.parent.mkdir(parents=True)
            target.write_bytes(b'corrupt')
            with self.assertRaisesRegex(ValueError, 'Pinned archive changed'):
                pack.build(root, Path(temp) / 'out', SHA)
            with self.assertRaisesRegex(ValueError, 'Output must be new'):
                pack.build(pack.ROOT, root, SHA)

    def test_exclusions_traversal_duplicates_and_unqualified_runtime(self):
        for name in ('../CL.EXE', '/absolute', 'x\\evil', 'x/../CL.EXE', 'C:/DOS',
                     'x/BIN/CL.EXE', 'DDK/WINDOWS.H', 'WINDOWS/SYSTEM.INI',
                     'x/LIBW.LIB', 'disk.IMG', 'sdk.ZIP'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                pack.safe_name(name)
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'archive.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('NEW.EXE', b'MZunknown')
            with self.assertRaisesRegex(ValueError, 'Unqualified runtime'):
                pack.inspect_zip(path, {})
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('README.TXT', b'one')
                archive.writestr('readme.txt', b'two')
            with self.assertRaisesRegex(ValueError, 'Duplicate'):
                pack.inspect_zip(path, {})
            with zipfile.ZipFile(path, 'w') as archive:
                item = zipfile.ZipInfo('LINK.TXT')
                item.create_system = 3
                item.external_attr = 0o120777 << 16
                archive.writestr(item, b'outside')
            with self.assertRaisesRegex(ValueError, 'symlink'):
                pack.inspect_zip(path, {})

    def test_unexpected_output(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'dist'
            pack.build(pack.ROOT, out, SHA)
            (out / 'CL.EXE').write_bytes(b'not-allowed')
            with self.assertRaisesRegex(ValueError, 'asset set'):
                pack.verify(out, SHA)

    def test_independent_release_pins_reject_resigned_archive(self):
        workflow = (pack.ROOT / '.github/workflows/sound-packages.yml').read_text()
        code = textwrap.dedent(workflow.split("python3 - <<'PY'\n", 1)[1]
                               .split('\n          PY', 1)[0])
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'dist'
            pack.build(pack.ROOT, out, SHA)
            previous = Path.cwd()
            old_sha = os.environ.get('SOURCE_SHA')
            try:
                os.chdir(temp)
                os.environ['SOURCE_SHA'] = SHA
                exec(compile(code, 'independent-release-check', 'exec'), {})
                (out / 'SOUNDP01.ZIP').write_bytes(b'unqualified replacement')
                (out / 'SHA256SUMS').write_text(pack.checksums(out))
                with self.assertRaises(AssertionError):
                    exec(compile(code, 'independent-release-check', 'exec'), {})
            finally:
                os.chdir(previous)
                if old_sha is None:
                    os.environ.pop('SOURCE_SHA', None)
                else:
                    os.environ['SOURCE_SHA'] = old_sha


if __name__ == '__main__':
    unittest.main()
