#!/usr/bin/env python3
"""Host-only, lyric-free score -> complete-state WZM1 PSG arrangement.

Usage: python ARRANGE.PY [--score SCORE.json] [--beatmap MAP.json]
                        [--output WEEZER.WZM] [--preview-ms 40000]
No audio samples, network calls, emulator dependency, or target-side synthesis.
"""
from __future__ import annotations
import argparse
from collections import defaultdict
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import struct

HEADER = struct.Struct('<4sHHIII')
RECORD = struct.Struct('<I4B4BBB')
CUE = struct.Struct('<4sIII')
ROOT = Path(__file__).resolve().parent.parent

@dataclass(frozen=True)
class BeatNote:
    channel: int
    start: float
    duration: float
    pitch: int
    velocity: int
    role: str

class BeatMap:
    def __init__(self, obj):
        self.obj = obj
        self.segments = obj['segments']
        self.mutes = obj.get('mute_intervals_ms', [])
        self.duration = int(obj['source_video_duration_ms'])
        previous_beat = 0
        previous_time = -1
        for s in self.segments:
            assert s['beat_start'] == previous_beat
            assert s['beat_end'] > s['beat_start']
            assert s['time_start_ms'] >= previous_time
            assert s['time_end_ms'] > s['time_start_ms']
            previous_beat = s['beat_end']
            previous_time = s['time_end_ms']
        assert previous_beat == obj['score_total_beats']
        assert 0 <= previous_time <= self.duration

    @staticmethod
    def in_segment(s, beat):
        return s['time_start_ms'] + (beat-s['beat_start']) * (
            s['time_end_ms']-s['time_start_ms']) / (s['beat_end']-s['beat_start'])

    def spans(self, start, duration):
        """Split in beat space first, then subtract dialogue intervals in time."""
        result = []
        for s in self.segments:
            lo, hi = max(start, s['beat_start']), min(start+duration, s['beat_end'])
            if hi <= lo:
                continue
            parts = [(self.in_segment(s, lo), self.in_segment(s, hi))]
            for ml, mh in self.mutes:
                clean = []
                for a, b in parts:
                    if b <= ml or a >= mh:
                        clean.append((a, b))
                    else:
                        if a < ml: clean.append((a, ml))
                        if b > mh: clean.append((mh, b))
                parts = clean
            result += [(round(a), round(b)) for a, b in parts if round(b) > round(a)]
        return result


def section_at(score, beat):
    return next(s['name'] for s in score['sections'] if s['start_beat'] <= beat < s['end_beat'])


def lead_velocity(section, beat):
    base = 106
    if 'chorus' in section or section == 'outro': base = 110
    if section == 'solo_and_build': base = 114
    if section == 'bridge': base = 106
    return min(120, base + (4 if beat % 4 == 0 else 0) - (3 if beat % 1 else 0))


def make_notes(score, transpose=-1, bass_lift=12, harmony=True):
    out = []
    repeated_gaps = 0
    lead = score['lead_sustained_events']
    for i, e in enumerate(lead):
        if e['midi'] is None: continue
        dur = e['duration']
        # Actual gaps articulate repeated pitches on an envelope-less PSG. A tied
        # source event is merged, never interrupted or attacked at its tie join.
        if i+1 < len(lead):
            nxt = lead[i+1]
            if (e.get('tied_segments', 1) == 1 and nxt['midi'] == e['midi'] and
                    e['absolute_beat']+dur == nxt['absolute_beat'] and dur >= .25):
                dur -= .125
                repeated_gaps += 1
        out.append(BeatNote(0, e['absolute_beat'], dur, e['midi']+transpose,
                            lead_velocity(e['section'], e['absolute_beat']), 'lead'))

    harmony_bars = [b for b in score['expanded_bars']
                    if harmony and b['section'] == 'outro' and b.get('optional_harmony')]
    for channel, key in ((1, 'bass_events'), (2, 'fifth_events')):
        for e in score[key]:
            if e['midi'] is None: continue
            at = e['absolute_beat']
            if channel == 2 and any(b['absolute_beat'] <= at < b['absolute_beat']+4
                                     for b in harmony_bars):
                continue
            section = section_at(score, at)
            vel = 69 if channel == 1 else 54
            if 'chorus' in section or section == 'outro': vel += 5
            if section == 'bridge': vel += 3
            if at % 4 == 0: vel += 4
            elif at % 1: vel -= 4
            # One sixteenth rest between repeated eighth-note guitar pulses;
            # quarter-note bass holds keep three sixteenths of their duration.
            dur = max(.125, e['duration']-.25)
            out.append(BeatNote(channel, at, dur, e['midi']+bass_lift+transpose,
                                vel, 'bass' if channel == 1 else 'fifth'))
    for b in harmony_bars:
        for e in b['optional_harmony']:
            if e['midi'] is not None:
                out.append(BeatNote(2, b['absolute_beat']+e['beat'], e['duration'],
                                    e['midi']+transpose, 65, 'written outro harmony'))

    # One hardware noise channel: choose a kick, snare, or hat, never stack them.
    # Kick on 1/3, snare on 2/4, restrained hats on the offbeats. Gate at the next
    # sixteenth; no fast rolls or high-rate timer requirement.
    for b in score['expanded_bars']:
        base = b['absolute_beat']
        for j in range(8):
            local = j*.5
            if b['number'] == 44:  # Expose the signature turnaround.
                if j != 0: continue
                pitch, vel = 36, 48
            elif b['number'] == 58:  # Follow the notated final accented cadence.
                if local not in (0, 1, 2, 2.5, 3): continue
                pitch, vel = (38, 50) if local == 2.5 else (36, 60)
            elif j in (0, 4):
                pitch, vel = 36, 60 if j == 0 else 55
            elif j in (2, 6):
                pitch, vel = 38, 56
            else:
                pitch, vel = 42, 32 if j in (1, 5) else 29
            if b['section'].startswith('verse'): vel -= 3
            out.append(BeatNote(3, base+local, .25, pitch, vel, 'noise groove'))
    return out, {'repeated_lead_articulation_gaps': repeated_gaps,
                 'outro_harmony_bars': [b['number'] for b in harmony_bars]}


def reject_channel_overlaps(notes):
    """Reject ambiguous monophonic input before scheduling any state changes.

    Exact end/start adjacency is valid. Same-pitch duplicates and overlaps are
    not implicit ties: custom scores must supply already-merged sustained ties.
    """
    by_channel = defaultdict(list)
    for n in notes:
        if (not math.isfinite(n.start) or not math.isfinite(n.duration) or
                n.start < 0 or n.duration <= 0):
            raise ValueError(f'Invalid note interval on channel {n.channel}: {n}')
        by_channel[n.channel].append(n)
    for channel, events in by_channel.items():
        previous = None
        for current in sorted(events, key=lambda n: (n.start, n.duration)):
            if previous is not None and current.start < previous.start+previous.duration:
                raise ValueError(
                    f'Overlapping notes on channel {channel}: '
                    f'{previous.role} [{previous.start}, '
                    f'{previous.start+previous.duration}) and '
                    f'{current.role} [{current.start}, '
                    f'{current.start+current.duration}). '
                    'Merge explicit ties or allocate another channel.')
            previous = current


def compile_records(notes, beatmap):
    notes = list(notes)
    reject_channel_overlaps(notes)
    changes = defaultdict(list)
    sounding = []
    for n in notes:
        assert 0 <= n.channel <= 3
        assert (35 <= n.pitch <= 81) if n.channel == 3 else (45 <= n.pitch <= 96)
        assert 1 <= n.velocity <= 127
        for start, stop in beatmap.spans(n.start, n.duration):
            velocity = n.velocity
            # Optional edit fade, applied only at already scheduled attacks. This
            # creates no extra sub-timer events and then the hard dialogue gate wins.
            for fade in beatmap.obj.get('optional_fades_ms', []):
                if fade['start'] <= start < fade['end']:
                    frac = (fade['end']-start)/(fade['end']-fade['start'])
                    velocity = max(1, round(velocity*(.25+.75*frac)))
            changes[start].append((1, n.channel, n.pitch, velocity))
            changes[stop].append((0, n.channel, 0, 0))
            sounding.append({'channel': n.channel, 'start_ms': start, 'end_ms': stop,
                             'note': n.pitch, 'velocity': velocity, 'role': n.role,
                             'source_beat': n.start})
    state = [0]*8
    records = [(0, (0,0,0,0), (0,0,0,0), 0)]
    for stamp in sorted(changes):
        previous = state.copy()
        trigger = 0
        # End old note before beginning replacement on the same channel.
        for priority, ch, pitch, velocity in sorted(changes[stamp]):
            state[ch], state[ch+4] = pitch, velocity
            if priority: trigger |= 1 << ch
        if state != previous or trigger:
            rec = (stamp, tuple(state[:4]), tuple(state[4:]), trigger)
            if stamp == records[-1][0]: records[-1] = rec
            else: records.append(rec)
    stop = (beatmap.duration, (0,0,0,0), (0,0,0,0), 0)
    if records[-1][0] == beatmap.duration: records[-1] = stop
    else: records.append(stop)
    return records, sounding


def encode(records, duration):
    header = HEADER.pack(b'WZM1', 20, 14, len(records), duration, 0)
    return header + b''.join(RECORD.pack(t, *n, *v, mask, 0) for t,n,v,mask in records)


def decode(data):
    assert len(data) >= HEADER.size, 'truncated header'
    magic, hs, rs, count, duration, reserved = HEADER.unpack_from(data)
    assert (magic, hs, rs, reserved) == (b'WZM1', 20, 14, 0), 'invalid header'
    assert 1 <= count <= 10000 and 0 < duration <= 600000
    assert len(data) == hs+count*rs, 'incorrect file length'
    records = []
    for i in range(count):
        row = RECORD.unpack_from(data, hs+i*rs)
        t, notes, velocities, mask, unused = row[0], row[1:5], row[5:9], row[9], row[10]
        assert unused == 0 and mask & ~15 == 0
        assert 0 <= t <= duration
        assert not records or records[-1][0] < t, 'non-increasing timestamp'
        for ch, (note, velocity) in enumerate(zip(notes, velocities)):
            if not note: assert velocity == 0
            else:
                assert (35 <= note <= 81) if ch == 3 else (45 <= note <= 96)
                assert 1 <= velocity <= 127
        records.append((t, notes, velocities, mask))
    return duration, records


def validate(records, duration, mutes=(), min_spacing=55):
    assert records[0][0] == 0
    assert not any(records[0][1]) and not any(records[-1][1])
    assert records[-1][0] == duration
    deltas = [b[0]-a[0] for a,b in zip(records, records[1:])]
    assert min(deltas) >= min_spacing, ('too-close record', min(deltas))
    for lo, hi in mutes:
        lo, hi = round(lo), round(hi)
        for i, row in enumerate(records[:-1]):
            end = records[i+1][0]
            if max(row[0], lo) < min(end, hi):
                assert not any(row[1]), ('sound in dialogue', row, lo, hi)
    return {'event_count': len(records), 'minimum_record_spacing_ms': min(deltas),
            'maximum_record_spacing_ms': max(deltas),
            'retrigger_records': sum(bool(r[3]) for r in records),
            'tone_note_range': [min(n for r in records for n in r[1][:3] if n),
                                max(n for r in records for n in r[1][:3] if n)],
            'noise_notes': sorted({r[1][3] for r in records if r[1][3]}),
            'first_sound_ms': next(r[0] for r in records if any(r[1])),
            'last_sound_end_ms': max(records[i+1][0] for i,r in enumerate(records[:-1]) if any(r[1])),
            'all_dialogue_intervals_silent': True,
            'strictly_increasing_timestamps': True, 'header_roundtrip': True}


def preview_records(records, start, duration):
    end = start+duration
    current = next((r for r in reversed(records) if r[0] <= start), records[0])
    out = [(0, (0,0,0,0), (0,0,0,0), 0)]
    # A 125ms lead-in keeps a guaranteed initial silent record for every reader.
    for t,n,v,m in records:
        if start <= t < end-125:
            out.append((t-start+125, n, v, m))
    if any(current[1]) and not any(r[0] == 125 for r in out):
        out.insert(1, (125, current[1], current[2], 15))
    while out and duration-out[-1][0] < 55: out.pop()
    out.append((duration, (0,0,0,0), (0,0,0,0), 0))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--score', type=Path, default=ROOT/'score/buddy_holly_score.json')
    ap.add_argument('--beatmap', type=Path, default=ROOT/'sync/buddy_holly_beatmap.json')
    ap.add_argument('--output', type=Path, default=ROOT/'media/WEEZER.WZM')
    ap.add_argument('--preview-ms', type=int, default=40000)
    ap.add_argument('--no-outro-harmony', action='store_true')
    args = ap.parse_args(argv)
    score = json.loads(args.score.read_text())
    mapping = json.loads(args.beatmap.read_text())
    bm = BeatMap(mapping)
    assert bm.duration == 241400, 'expected the complete user-provided 241.4-second video'
    assert score['total_beats'] == mapping['score_total_beats'] == 320
    transpose = mapping['recording_transposition_semitones']
    notes, choices = make_notes(score, transpose, 12, not args.no_outro_harmony)
    records, intervals = compile_records(notes, bm)
    packed = encode(records, bm.duration)
    duration, readback = decode(packed)
    assert readback == records
    qa = validate(readback, duration, bm.mutes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(packed)
    cue_start = round(mapping['segments'][0]['time_start_ms'])
    cue_end = round(mapping['segments'][-1]['time_end_ms'])
    cue = args.output.with_suffix('.CUE')
    cue.write_bytes(CUE.pack(b'WZC1', bm.duration, cue_start, cue_end))
    assert CUE.unpack(cue.read_bytes()) == (b'WZC1', bm.duration, cue_start, cue_end)
    report = {'format':'WZM1', 'duration_ms':duration, 'bytes':len(packed),
              'sha256':hashlib.sha256(packed).hexdigest(), 'source_score':str(args.score),
              'source_beatmap':str(args.beatmap), 'source_score_sha256':hashlib.sha256(args.score.read_bytes()).hexdigest(),
              'source_beatmap_sha256':hashlib.sha256(args.beatmap.read_bytes()).hexdigest(),
              'transpose_semitones':transpose,'bass_and_fifth_octave_lift':12,
              'cue':{'file':str(cue), 'full_duration_ms':bm.duration,'song_start_ms':cue_start,'song_end_ms':cue_end},
              'choices':choices, 'validation':qa}
    if args.preview_ms:
        assert 30000 <= args.preview_ms <= 45000
        preview = preview_records(records, cue_start, args.preview_ms)
        pp = args.output.with_name('PREVIEW.WZM')
        pp.write_bytes(encode(preview, args.preview_ms))
        pd, pr = decode(pp.read_bytes())
        report['preview'] = {'file':str(pp), 'duration_ms':pd, 'validation':validate(pr,pd)}
        pp.with_suffix('.CUE').write_bytes(CUE.pack(b'WZC1',pd,0,pd))
        bridge_start = round(next(seg['time_start_ms'] for seg in mapping['segments'] if seg['beat_start'] == 200))
        bridge_preview = preview_records(records, bridge_start, args.preview_ms)
        bp = args.output.with_name('BRIDGE.WZM')
        bp.write_bytes(encode(bridge_preview, args.preview_ms))
        bd, br = decode(bp.read_bytes())
        report['bridge_preview'] = {'file':str(bp), 'duration_ms':bd, 'validation':validate(br,bd)}
        bp.with_suffix('.CUE').write_bytes(CUE.pack(b'WZC1',bd,0,bd))
    args.output.with_suffix('.WZM.json').write_text(json.dumps(report,indent=2)+'\n')
    (ROOT/'score/arrangement_intervals.json').write_text(json.dumps(intervals,indent=2)+'\n')
    (ROOT/'score/arrangement_notes.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__': main()
