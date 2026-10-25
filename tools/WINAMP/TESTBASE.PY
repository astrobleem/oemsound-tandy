"""Current published baseline closure checks; no native tool or header writes."""
from pathlib import Path
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import contextlib
import io
import runpy
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

SCRIPT=Path(__file__).with_name('CLIENTS.PY')
spec=importlib.util.spec_from_loader('current_clients',
    SourceFileLoader('current_clients',str(SCRIPT)))
clients=importlib.util.module_from_spec(spec)
spec.loader.exec_module(clients)



def put(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def record(path, value):
    put(path, json.dumps(value).encode('ascii'))


def producer_build(wrapper, name, out, root):
    tool = root/'toolchain'
    for source in ('BIN/CL.EXE', 'BIN/LINK4.EXE', 'LIB/SLIBCEW.LIB',
                   'LIB/LIBW.LIB', 'DDK/286/INC/WINDOWS.H'):
        put(tool/source, b'host-test toolchain placeholder')
    dosbox, lease = root/'DOSBOX.EXE', root/'LEASE'
    put(dosbox, b'not executed'); put(lease, b'not acquired')
    original = SourceFileLoader.exec_module
    calls = []
    def execute(loader, module):
        if loader.name != 'consumer_rundos':
            return original(loader, module)
        def run(executable, config, shared_lease, timeout):
            calls.append((executable, shared_lease, timeout))
            exe = bytearray(128); exe[:2] = b'MZ'
            struct.pack_into('<I', exe, 0x3c, 64); exe[64:66] = b'NE'
            put(out/(name+'.EXE'), exe); put(out/'BUILD.LOG', b'PASS')
            record(out/'PROCESS.JSON', {'return': 0, 'owned_child_stopped': True,
                                       'lease_released': True})
        module.run = run
    argv = [str(wrapper), '--dosbox', str(dosbox), '--toolchain', str(tool),
            '--lease', str(lease), '--out', str(out)]
    with patch.object(sys, 'argv', argv), patch.object(SourceFileLoader, 'exec_module', execute):
        with contextlib.redirect_stdout(io.StringIO()):
            runpy.run_path(str(wrapper), run_name='__main__')
    assert len(calls) == 1
    return json.loads((out/'BUILD.JSON').read_bytes())


class BaselineTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='current-baseline-')
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.files={name:('published native '+name).encode()
                    for name,_,_ in clients.SPECS}
        self.record={'schema':1,'source_commit':'7'*40,'files':{}}
        for name,data in self.files.items():
            (self.root/name).write_bytes(data)
            self.record['files'][name]={**clients.pin(data),
                                       'crc32':'%08x'%zlib.crc32(data)}
        self.manifest=self.root/'BASE.JSN'
        self.save()

    def save(self):
        self.manifest.write_text(json.dumps(self.record))

    def test_actual_oem_producers_feed_current_client_collector(self):
        repo = SCRIPT.parents[2]
        workspace, native = self.root/'workspace', self.root/'native'
        display = workspace/'unified/oemdisplay-tandy'
        (display/'examples/TSHELL/IDLE').mkdir(parents=True)
        clean = {'return': 0, 'owned_child_stopped': True, 'lease_released': True}
        for name, kind, folder in clients.SPECS:
            if folder == 'unchanged':
                put(display/'tools/XR1PACK/PAYLOAD'/name, self.files[name])
                continue
            build = native/(folder+'9') if folder else self.root/'fixxt'
            if name in ('TCHIME.EXE', 'XPCHIME.EXE', 'TEXIT.EXE'):
                component = {'TCHIME.EXE': 'CHIME', 'XPCHIME.EXE': 'XPCHIME',
                             'TEXIT.EXE': 'TEXIT'}[name]
                produced = producer_build(repo/('src/win30/'+component+'/BUILD.PY'),
                                          Path(name).stem, build, self.root)
                expected = 'unified/oemsound-tandy/src/win30/SHARED/PSGOWN.H'
                self.assertIn(expected, produced['source_pins'])
                self.assertEqual(produced['runtime'][name],
                                 clients.pin((build/name).read_bytes()))
                for label in produced['source_pins']:
                    relative = label.removeprefix('unified/oemsound-tandy/')
                    self.assertNotEqual(relative, label)
                    put(workspace/label, (repo/relative).read_bytes())
                continue
            data = ('host binary '+name).encode('ascii')
            source = ('host source '+name).encode('ascii')
            if folder:
                label = 'unified/oemdisplay-tandy/'+Path(name).stem+'.C'
                put(workspace/label, source); staged = Path(label).name
                value = {'source_pins': {label: clients.pin(source)},
                         'runtime': {name: clients.pin(data)}}
            else:
                staged = 'FIXXT.C'; put(display/'tools/DOSINST/FIXXT.C', source)
                value = {'sources': {staged: clients.pin(source)},
                         'runtime': clients.pin(data)}
            put(build/staged, source); put(build/name, data)
            record(build/'BUILD.JSON', value); record(build/'PROCESS.JSON', clean)
        def collect(output):
            argv = [str(SCRIPT), '--native', str(native), '--fixxt', str(self.root/'fixxt'),
                    '--display', str(display), '--workspace', str(workspace),
                    '--out', str(output), '--header', str(output.with_suffix('.H')),
                    '--client-suffix', '9', '--baseline-root', str(self.root),
                    '--baseline-manifest', str(self.manifest)]
            with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
                clients.main()
        collect(self.root/'CLIENTS')
        pins = json.loads((self.root/'CLIENTS/PINS.JSN').read_bytes())
        self.assertEqual(len(pins), 7)
        for name in ('TCHIME.EXE', 'XPCHIME.EXE', 'TEXIT.EXE'):
            self.assertEqual(pins[name]['runtime'],
                             clients.pin((self.root/'CLIENTS'/name).read_bytes()))
        source = workspace/'unified/oemsound-tandy/src/win30/SHARED/PSGOWN.H'
        data = source.read_bytes(); source.write_bytes(b'stale shared header')
        with self.assertRaisesRegex(ValueError, 'Current client source changed:'):
            collect(self.root/'STALE')
        source.write_bytes(data)
        (native/'chime9/PSGOWN.H').unlink()
        with self.assertRaises(OSError):
            collect(self.root/'MISSING')
        put(native/'chime9/PSGOWN.H', data)
        record(native/'chime9/PROCESS.JSON', {**clean, 'return': 1})
        with self.assertRaisesRegex(ValueError, 'Native build did not complete:'):
            collect(self.root/'FAILED')

    def test_reads_exact_seven_file_current_snapshot(self):
        self.assertEqual(clients.current_baseline(self.root,self.manifest),self.files)

    def test_rejects_same_size_producer_finisher(self):
        data=bytearray(self.files['FIXXT.EXE']);data[len(data)//2]^=1
        (self.root/'FIXXT.EXE').write_bytes(data)
        with self.assertRaisesRegex(ValueError,'baseline differs: FIXXT.EXE'):
            clients.current_baseline(self.root,self.manifest)

    def test_rejects_missing_or_extra_identity(self):
        del self.record['files']['TP.EXE'];self.save()
        with self.assertRaisesRegex(ValueError,'exactly seven'):
            clients.current_baseline(self.root,self.manifest)
        self.record['files']['ALIEN.EXE']=clients.pin(b'other');self.save()
        with self.assertRaisesRegex(ValueError,'exactly seven'):
            clients.current_baseline(self.root,self.manifest)

    def test_bad_baseline_refuses_before_output_or_header(self):
        (self.root/'FIXXT.EXE').write_bytes(b'wrong producer finisher')
        output,header=self.root/'OUT',self.root/'XTCLIENT.H'
        result=subprocess.run([sys.executable,str(SCRIPT),
            '--native',str(self.root/'native'),'--fixxt',str(self.root/'fixxt'),
            '--display',str(self.root/'display'),'--workspace',str(self.root),
            '--out',str(output),'--header',str(header),'--client-suffix','9',
            '--baseline-root',str(self.root),'--baseline-manifest',str(self.manifest)],
            capture_output=True,text=True,timeout=10)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('baseline differs: FIXXT.EXE',result.stderr)
        self.assertFalse(output.exists());self.assertFalse(header.exists())


if __name__=='__main__':unittest.main(verbosity=2)
