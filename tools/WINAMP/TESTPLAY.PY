"""Synthetic native-player packaging guard tests; never native acceptance."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
import json
import unittest

HERE=Path(__file__).resolve().parent
fixture=SourceFileLoader('player_pack_fixture',str(HERE/'TESTMIDI.PY')).load_module()
pack=fixture.pack


class PlayerTests(unittest.TestCase):
    def setUp(self):
        self.f=fixture.MidiPackage('test_rebuilt_dlls_and_source_replace_baseline')
        self.f.setUp();self.addCleanup(self.f.doCleanups)
        self.stage=self.f.stages['player']

    def change(self,name,value):
        (self.stage/name).write_bytes(value)

    def source_change(self,name,value):
        source=json.loads((self.stage/'SOURCES.JSN').read_bytes())
        source['files'][name]=pack.identity(value)
        (self.stage/'SOURCES.JSN').write_bytes(fixture.dump(source))
        result=json.loads((self.stage/'RESULT.JSN').read_bytes());result['source']=source
        (self.stage/'RESULT.JSN').write_bytes(fixture.dump(result))
        self.change(name,value)
        (self.f.repo/'src/win30/PLAYER'/name).write_bytes(value)

    def test_player_copies_current_sources_and_preserves_frozen_evidence(self):
        current=self.f.repo/'experimental/WINLIB30/SOURCE'
        (current/'MINIPLAY.C').write_bytes(b'STALE UNCOMPILED COPY')
        files=self.f.build()
        self.assertNotIn('TWXT2/SOURCE/RAW/MINIPLAY.C',files)
        self.assertNotIn('TWXT2/SOURCE/MIDI/MINIPLAY.C',files)
        self.assertNotIn('TWXT2/SOURCE/MCI/MINIPLAY.C',files)
        self.assertNotIn('TWXT2/SRC/MINIPLAY.C',files)
        for name in pack.PLAYER_SOURCES:
            self.assertEqual(files['TWXT2/SRC/SOURCE/'+name],files['TWXT2/SOURCE/PLAYER/'+name])
            self.assertEqual(files['TWXT2/DEPS/'+name],files['TWXT2/SOURCE/PLAYER/'+name])
        files['TWXT2/SRC/EVIDENCE/FROZEN/MINIPLAY.C']=b'FROZEN HISTORICAL SOURCE'
        self.f.readback(files)
        for name,wanted in pack.MIDI_BASE['runtime'].items():
            self.assertEqual(pack.identity(files['TWXT2/'+name]),wanted)

    def test_old_player_or_stale_current_source_refused(self):
        original=(self.stage/'MINIPLAY.EXE').read_bytes()
        self.change('MINIPLAY.EXE',original[:-5]);self.f.refused(self.f.build)
        self.change('MINIPLAY.EXE',original)
        (self.f.repo/'src/win30/PLAYER/FMTAPP.C').write_bytes(b'NEW FINAL SOURCE')
        self.f.refused(self.f.build)

    def test_missing_header_or_compiled_unit_refused(self):
        for name in ('FMTAPP.H','FMTCORE.C','PLLIST.C','BLDPLAY.BAT'):
            with self.subTest(name=name):
                source=json.loads((self.stage/'SOURCES.JSN').read_bytes());saved=fixture.dump(source)
                result=json.loads((self.stage/'RESULT.JSN').read_bytes());savedresult=fixture.dump(result)
                del source['files'][name];result['source']=source
                self.change('SOURCES.JSN',fixture.dump(source));self.change('RESULT.JSN',fixture.dump(result))
                self.f.refused(self.f.build)
                self.change('SOURCES.JSN',saved);self.change('RESULT.JSN',savedresult)

    def test_unpinned_include_or_link_unit_refused(self):
        self.source_change('MINIPLAY.C',b'#include "EXTRA.H"\r\n')
        self.f.refused(self.f.build)
        self.source_change('MINIPLAY.C',b'UNIT SOURCE')
        self.source_change('MINIPLAY.RSP',b'UNPINNED.OBJ\r\n')
        self.f.refused(self.f.build)

    def test_unfinished_process_or_result_refused(self):
        process=json.loads((self.stage/'PROCESS.JSON').read_bytes())
        result=json.loads((self.stage/'RESULT.JSN').read_bytes())
        for key,value in (('return',1),('owned_child_stopped',False),('lease_released',False),('watchdog',True)):
            with self.subTest(key=key):
                changed=dict(process,**{key:value});r=dict(result,process=changed)
                self.change('PROCESS.JSON',fixture.dump(changed));self.change('RESULT.JSN',fixture.dump(r))
                self.f.refused(self.f.build)
        self.change('PROCESS.JSON',fixture.dump(process))
        self.change('RESULT.JSN',fixture.dump(dict(result,status='STAGED')))
        self.f.refused(self.f.build)

    def test_map_and_native_ne_guard_refused(self):
        original=(self.stage/'MINIPLAY.MAP').read_bytes()
        self.change('MINIPLAY.MAP',b'OTHER MAP');self.f.refused(self.f.build)
        self.change('MINIPLAY.MAP',original)
        binary=bytearray((self.stage/'MINIPLAY.EXE').read_bytes())
        binary[:2]=b'PE';self.change('MINIPLAY.EXE',bytes(binary));self.f.refused(self.f.build)

    def test_acceptance_must_name_this_player(self):
        tests=json.loads(self.f.tests.read_bytes());tests['player']=pack.identity(b'OLD PLAYER')
        self.f.tests.write_bytes(fixture.dump(tests));self.f.refused(self.f.build)

    def test_current_duplicate_and_core_runtime_tampering_refused(self):
        files=self.f.build()
        for name in ('DEPS/OLD/MINIPLAY.C','SOURCE/RAW/MINIPLAY.C','SRC/SOURCE/FMTAPP.C','RESERVE.COM'):
            with self.subTest(name=name):
                changed=dict(files);changed['TWXT2/'+name]=b'STALE OR CHANGED'
                self.f.refused(lambda:self.f.readback(changed))


if __name__=='__main__':unittest.main()
