#!/usr/bin/env python3
"""Deterministic arranger/format tests; no emulator or external dependencies."""
import importlib.machinery
import importlib.util
import json
import copy
from pathlib import Path
import struct
import sys
import unittest
P=Path(__file__).resolve().parent
loader=importlib.machinery.SourceFileLoader('arrange',str(P/'ARRANGE.PY'))
spec=importlib.util.spec_from_loader(loader.name,loader)
a=importlib.util.module_from_spec(spec);sys.modules[spec.name]=a;loader.exec_module(a)
ROOT=P.parent
SCORE=json.loads((ROOT/'score/buddy_holly_score.json').read_text())
MAP=json.loads((ROOT/'sync/buddy_holly_beatmap.json').read_text())

class ArrangerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bm=a.BeatMap(MAP)
        cls.notes,cls.choices=a.make_notes(SCORE)
        cls.records,cls.intervals=a.compile_records(cls.notes,cls.bm)
        cls.packed=a.encode(cls.records,cls.bm.duration)

    def test_format_layout_and_roundtrip(self):
        self.assertEqual(a.HEADER.size,20);self.assertEqual(a.RECORD.size,14)
        duration,records=a.decode(self.packed)
        self.assertEqual(duration,241400);self.assertEqual(records,self.records)
        self.assertEqual(len(self.packed),20+14*len(records))

    def test_gates_spacing_ranges_and_termination(self):
        result=a.validate(self.records,241400,self.bm.mutes)
        self.assertGreaterEqual(result['minimum_record_spacing_ms'],55)
        self.assertEqual(self.records[-1][1],(0,0,0,0))
        self.assertEqual(result['noise_notes'],[36,38,42])

    def test_full_stream_matches_reproducible_output(self):
        self.assertEqual(self.packed,(ROOT/'media/WEEZER.WZM').read_bytes())

    def test_no_voice_overlap(self):
        for channel in range(4):
            spans=sorted((n['start_ms'],n['end_ms']) for n in self.intervals if n['channel']==channel)
            for (_,end),(start,_) in zip(spans,spans[1:]):
                self.assertGreaterEqual(start,end,channel)

    def test_overlapping_custom_score_is_rejected_before_compilation(self):
        bad = copy.deepcopy(SCORE)
        # Extend C#5 over the next D5. The old end must not silently truncate D5.
        bad['lead_sustained_events'][0]['duration'] = 1
        notes, _ = a.make_notes(bad)
        with self.assertRaisesRegex(ValueError, 'Overlapping notes on channel 0'):
            a.compile_records(notes, self.bm)

    def test_duplicate_and_unsorted_overlap_rejected_but_adjacency_allowed(self):
        note = lambda start, duration, pitch=60, channel=0: a.BeatNote(
            channel, start, duration, pitch, 90, 'test note')
        for malformed in ([note(0,1), note(0,1)],
                          [note(1,2,62), note(0,2)],
                          [note(0,4), note(1,.25,64)]):
            with self.assertRaisesRegex(ValueError, 'Overlapping notes on channel 0'):
                a.compile_records(malformed, self.bm)
        # Same-channel adjacency and simultaneous independent channels are valid.
        valid = [note(1,1,62), note(0,1), note(0,2,67,1)]
        records, _ = a.compile_records(valid, self.bm)
        self.assertTrue(any(r[1][0] == 62 for r in records))

    def test_first_chord_transposition_and_uniform_lift(self):
        first=next(r for r in self.records if any(r[1]))
        self.assertEqual(first[0],35652)
        self.assertEqual(first[1],(72,53,60,36))

    def test_source_tie_is_continuous_and_not_retriggered(self):
        for e in SCORE['lead_sustained_events']:
            if e['midi'] is None or e['tied_segments']<2:continue
            for start,stop in self.bm.spans(e['absolute_beat'],e['duration']):
                for t,notes,vel,mask in self.records:
                    if start<t<stop:
                        self.assertEqual(notes[0],e['midi']-1,(e,t))
                        self.assertFalse(mask&1,(e,t))

    def test_repeated_lead_articulation(self):
        self.assertEqual(self.choices['repeated_lead_articulation_gaps'],61)
        bridge=[n for n in self.notes if n.channel==0 and 200<=n.start<232]
        self.assertTrue(any(n.duration==.125 for n in bridge))
        for r in self.records:
            if r[3]&1:self.assertNotEqual(r[1][0],0)

    def test_bridge_riff_and_final_cadence(self):
        bass=[n.pitch for n in self.notes if n.channel==1 and 200<=n.start<204]
        self.assertEqual(bass,[56,56,56,51,53,51,53])
        final=[n for n in self.notes if n.channel==1 and n.start>=316]
        self.assertEqual([n.pitch for n in final],[56,56,56,51,53,51,53])
        self.assertEqual(final[-1].duration,.75)

    def test_outro_harmony_displaces_fifth(self):
        self.assertEqual(self.choices['outro_harmony_bars'],[53,55,57])
        for start in [296,304,312]:
            events=[n for n in self.notes if n.channel==2 and start<=n.start<start+4]
            self.assertEqual(len(events),5)
            self.assertTrue(all(n.role=='written outro harmony' for n in events))

    def test_segment_boundary_is_not_interpolated_across_dialogue(self):
        spans=self.bm.spans(199.5,1)
        self.assertEqual(len(spans),2)
        self.assertEqual(spans[0][1],134580)
        self.assertEqual(spans[1][0],140954)
        self.assertEqual(self.bm.spans(200,.25)[0][0],140954)

    def test_invalid_binary_is_rejected(self):
        for invalid in [self.packed[:19],self.packed[:-1],self.packed+b'x']:
            with self.assertRaises(AssertionError):a.decode(invalid)
        bad=bytearray(self.packed);bad[0]=0
        with self.assertRaises(AssertionError):a.decode(bad)
        bad=bytearray(self.packed);bad[20+13]=1
        with self.assertRaises(AssertionError):a.decode(bad)
        bad=bytearray(self.packed);bad[20+14:20+18]=struct.pack('<I',0)
        with self.assertRaises(AssertionError):a.decode(bad)

    def test_preview_and_cue(self):
        duration,records=a.decode((ROOT/'media/PREVIEW.WZM').read_bytes())
        self.assertEqual(duration,40000);a.validate(records,duration)
        self.assertEqual(a.CUE.unpack((ROOT/'media/WEEZER.CUE').read_bytes()),
                         (b'WZC1',241400,35652,200365))

if __name__=='__main__':unittest.main(verbosity=2)
