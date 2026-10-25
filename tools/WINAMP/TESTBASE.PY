"""Current published baseline closure checks; no native tool or header writes."""
from pathlib import Path
import importlib.util
from importlib.machinery import SourceFileLoader
import json
import subprocess
import sys
import tempfile
import unittest
import zlib

SCRIPT=Path(__file__).with_name('CLIENTS.PY')
spec=importlib.util.spec_from_loader('current_clients',
    SourceFileLoader('current_clients',str(SCRIPT)))
clients=importlib.util.module_from_spec(spec)
spec.loader.exec_module(clients)


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
