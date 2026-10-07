#!/usr/bin/env python3
"""Generate original MIDI fixtures; test the C core against a Fraction oracle.

Python 3 standard library only. Run from any directory: python3 test_smf.py.
The C timing oracle is not reused: expected timing is Python rational arithmetic.
"""
from fractions import Fraction
from pathlib import Path
import random
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
FIXTURES = HERE / "fixtures"


def vlq(n):
    assert 0 <= n <= 0x0FFFFFFF
    out = [n & 127]
    n >>= 7
    while n:
        out.append((n & 127) | 128)
        n >>= 7
    return bytes(reversed(out))


def chunk(name, data):
    return name + struct.pack(">I", len(data)) + data


def midi(tracks, ppqn=96, fmt=None, extra_header=b"", prefix=b"", suffix=b""):
    if fmt is None:
        fmt = int(len(tracks) > 1)
    header = struct.pack(">HHH", fmt, len(tracks), ppqn) + extra_header
    return (chunk(b"MThd", header) + prefix +
            b"".join(chunk(b"MTrk", t) for t in tracks) + suffix)


def ev(delta, wire, channel=None, tempo=None):
    return dict(delta=delta, wire=wire, channel=channel, tempo=tempo)


def ch(delta, status, d1, d2=None, running=False):
    data = bytes([d1] if d2 is None else [d1, d2])
    return ev(delta, (b"" if running else bytes([status])) + data,
              channel=(status, d1, 0 if d2 is None else d2,
                       2 if d2 is None else 3))


def tempo(delta, us):
    return ev(delta, b"\xff\x51\x03" + us.to_bytes(3, "big"), tempo=us)


def end(delta=0):
    return ev(delta, b"\xff\x2f\x00")


def encode(events):
    return b"".join(vlq(e["delta"]) + e["wire"] for e in events)


def expected(tracks, ppqn):
    merged = []
    for ti, track in enumerate(tracks):
        tick = 0
        for ei, e in enumerate(track):
            tick += e["delta"]
            merged.append((tick, ti, ei, e))
    merged.sort(key=lambda x: x[:3])
    clock = Fraction(0)
    last = 0
    us = 500000
    lines = []
    for tick, ti, _, event in merged:
        clock += Fraction((tick - last) * us, ppqn)
        last = tick
        if event["tempo"] is not None:
            us = event["tempo"]
        if event["channel"] is not None:
            status, d1, d2, length = event["channel"]
            lines.append(f"{clock // 1000} {tick} {ti} {status:02X} {d1} {d2} {length}")
    assert clock <= 3600000000
    return (f"OK tracks={len(tracks)} ppqn={ppqn} "
            f"duration_ms={clock // 1000} channels={len(lines)}\n" +
            "".join(line + "\n" for line in lines))


def fixtures():
    FIXTURES.mkdir(exist_ok=True)
    # Original eight-note ascending/returning exercise; no borrowed composition.
    melody = [tempo(0, 500000), ch(0, 0xC0, 0), ch(0, 0xB0, 7, 100)]
    for index, note in enumerate([60, 64, 67, 72, 71, 67, 64, 60]):
        if index == 4:
            melody.append(tempo(0, 400000))
        melody.extend([ch(0, 0x90, note, 90), ch(480, 0x90, note, 0, True)])
    melody.extend([ch(0, 0xB0, 123, 0), end(0)])
    conductor = [ev(0, b"\xff\x01\x0eOriginal study"), tempo(0, 500000),
                 tempo(960, 750000), end(960)]
    voice = [ch(0, 0xC0, 0), ch(0, 0x90, 60, 84), ch(480, 0x80, 60, 64),
             ch(0, 0x90, 67, 84), ch(480, 0x90, 67, 0, True),
             ch(0, 0x90, 64, 84, True), ch(480, 0x80, 64, 64),
             ch(0, 0x90, 60, 84), ch(480, 0x90, 60, 0, True),
             ch(0, 0xB0, 123, 0), end()]
    drums = []
    for i in range(8):
        drums.extend([ch(0 if i == 0 else 210, 0x99, 35 if i % 2 == 0 else 38, 72),
                      ch(30, 0x89, 35 if i % 2 == 0 else 38, 0)])
    drums.extend([ch(210, 0xB9, 120, 0), end()])
    cases = [("TYPE0.MID", [melody], 480),
             ("TYPE1.MID", [conductor, voice, drums], 480)]
    for name, tracks, ppqn in cases:
        (FIXTURES / name).write_bytes(midi([encode(t) for t in tracks], ppqn))
        (FIXTURES / (name + ".log")).write_text(expected(tracks, ppqn))
    return cases


def compile_host(name, *options):
    output = HERE / name
    subprocess.run(["gcc", "-x", "c", "-DSMF_HOST", "-std=c89", "-pedantic-errors",
                    "-Wall", "-Wextra", "-Wconversion", "-Werror", "-O2", *options,
                    str(HERE / "SMF.C"), str(HERE / "HOSTTEST.C"), "-o", str(output)],
                   check=True)
    return output


def main():
    host = compile_host("hosttest")
    cases = fixtures()
    checked = 0
    with tempfile.TemporaryDirectory(prefix="smf-test-") as tmp:
        path = Path(tmp) / "case.mid"

        def check(name, data, want=None, error=None, binary=host):
            nonlocal checked
            path.write_bytes(data)
            result = subprocess.run([str(binary), str(path)], capture_output=True,
                                    text=True, timeout=5)
            assert result.returncode in (0, 1), (name, result.returncode, result.stderr)
            if want is not None:
                assert result.returncode == 0, (name, result.stdout)
                assert result.stdout == want, (name, result.stdout, want)
            elif error is not None:
                assert result.returncode == 1, (name, result.stdout)
                assert result.stdout.startswith("ERROR "), (name, result.stdout)
                assert error in result.stdout, (name, result.stdout, error)
            checked += 1

        def valid(name, tracks, ppqn=96, **kwargs):
            check(name, midi([encode(t) for t in tracks], ppqn, **kwargs),
                  expected(tracks, ppqn))

        for name, tracks, ppqn in cases:
            check(name, (FIXTURES / name).read_bytes(), expected(tracks, ppqn))
        valid("empty musical sequence", [[end()]])
        valid("header extension and unknown chunks", [[end()]],
              extra_header=b"extension", prefix=chunk(b"JUNK", b"\xff" * 23),
              suffix=chunk(b"abcd", b""))
        valid("16 tracks maximum PPQN", [[end(i)] for i in range(16)], ppqn=32767)
        all_channels = [ch(0, 0x80, 0, 127), ch(1, 0x9F, 127, 0),
                        ch(1, 0xA0, 1, 2), ch(1, 0xB0, 120, 0),
                        ch(1, 0xC0, 127), ch(1, 0xC0, 0, running=True),
                        ch(1, 0xD0, 100), ch(1, 0xD0, 101, running=True),
                        ch(1, 0xE0, 0, 64), ch(1, 0xE0, 127, 127, True), end()]
        valid("all seven channel message kinds", [all_channels])
        valid("skipped binary meta and sysex", [[
            ch(0, 0x90, 60, 1), ev(0, b"\xff\x7f\x03\xff\x00\x80"),
            ch(0, 0x90, 60, 0), ev(0, b"\xf0\x04\x7e\xff\x00\xf7"),
            ev(0, b"\xf7\x03\xf1\xff\xf7"), ch(0, 0x90, 61, 0), end()]])
        valid("lowest index ties and independent running status", [
            [tempo(0, 1001), ch(0, 0x90, 60, 1), ch(0, 0x90, 61, 2, True),
             tempo(7, 2003), ch(0, 0x90, 62, 3), end(21)],
            [tempo(0, 3001), ch(0, 0x91, 65, 4), ch(7, 0x91, 66, 5, True),
             ch(7, 0x91, 67, 6, True), end(14)]], ppqn=7)
        valid("tempo remainder survives many changes", [[
            item for i in range(1, 501)
            for item in (tempo(1, 1000 + i % 17), ch(1, 0x90, i % 128, 0))
        ] + [end(1)]], ppqn=7)
        valid("maximum 28-bit delta at tiny tempo", [[tempo(0, 1), end(0x0FFFFFFF)]],
              ppqn=32767)
        valid("near 32-bit absolute tick", [[tempo(0, 1)] +
              [ev(0x0FFFFFFF, b"\xff\x01\x00") for _ in range(15)] +
              [end(0x0FFFFFFF)]], ppqn=32767)
        valid("maximum fraction multiplication", [[tempo(0, 32766), end(32766)]],
              ppqn=32767)
        valid("exactly one hour", [[end(7200)]], ppqn=1)
        base = midi([encode([end()])])
        pad = 32768 - len(base) - 8
        check("exact maximum file", base + chunk(b"JUNK", b"x" * pad),
              expected([[end()]], 96))
        check("one byte over maximum file", base + chunk(b"JUNK", b"x" * (pad + 1)),
              error="exceeds 32768")

        # Deterministically exercise rational clock arithmetic, all channels,
        # repeated tempo changes and up to 16-way merges against exact Fraction.
        rng = random.Random(0x8088)
        for sample in range(180):
            ppqn = rng.choice([1, 3, 7, 96, 480, 960, 32767])
            tracks = []
            for ti in range(rng.randint(1, 16)):
                track = []
                running = None
                for _ in range(rng.randint(1, 45)):
                    delta = rng.randrange(20)
                    if rng.randrange(5) == 0:
                        track.append(tempo(delta, rng.randint(1, min(0xFFFFFF, ppqn * 4000000))))
                        running = None
                    else:
                        status = rng.choice(range(0x80, 0xF0))
                        if running is not None and rng.randrange(2):
                            status = running
                        use_running = status == running
                        d2 = None if status & 0xF0 in (0xC0, 0xD0) else rng.randrange(128)
                        track.append(ch(delta, status, rng.randrange(128), d2, use_running))
                        running = status
                track.append(end(rng.randrange(20)))
                tracks.append(track)
            valid(f"rational randomized {sample}", tracks, ppqn=ppqn)

        # Header / file bounds: every prefix of this minimal valid file fails.
        for length in range(len(base)):
            check(f"truncated file {length}", base[:length], error="")
        corrupt = bytearray(base)
        corrupt[0] = 0
        check("bad magic", corrupt, error="MThd")
        for value in (0, 5, 0xFFFFFFFF):
            check("invalid header length", base[:4] + struct.pack(">I", value) + base[8:],
                  error="header length")
        check("format 2", midi([encode([end()])], fmt=2), error="format 0")
        check("zero PPQN", midi([encode([end()])], ppqn=0), error="PPQN")
        check("SMPTE", midi([encode([end()])], ppqn=0xE728), error="PPQN")
        check("zero tracks", midi([]), error="track count")
        check("17 tracks", midi([encode([end()])] * 17), error="track count")
        check("type zero two tracks", midi([encode([end()])] * 2, fmt=0), error="track count")
        check("missing track", base[:14], error="Fewer tracks")
        check("extra track", base + base[14:], error="More tracks")
        check("partial chunk header", base + b"1234567", error="chunk header")
        check("huge chunk", base + b"JUNK\xff\xff\xff\xff", error="file boundary")

        invalid_tracks = [
            ("empty track", b"", "end-of-track"),
            ("missing EOT", b"\x00\x90\x3c\x40", "end-of-track"),
            ("five-byte delta", b"\x81\x80\x80\x80\x00\xff\x2f\x00", "VLQ"),
            ("five-byte meta size", b"\x00\xff\x01\x81\x80\x80\x80\x00", "VLQ"),
            ("five-byte sysex size", b"\x00\xf0\x81\x80\x80\x80\x00", "VLQ"),
            ("running without prior", b"\x00\x3c\x40\x00\xff\x2f\x00", "Running status"),
            ("meta cancels status", b"\x00\x90\x3c\x40\x00\xff\x01\x00\x00\x3c\x00", "Running status"),
            ("F0 cancels status", b"\x00\x90\x3c\x40\x00\xf0\x00\x00\x3c\x00", "Running status"),
            ("F7 cancels status", b"\x00\x90\x3c\x40\x00\xf7\x00\x00\x3c\x00", "Running status"),
            ("data1 status", b"\x00\x90\x80\x40", "MIDI data"),
            ("data2 status", b"\x00\x90\x3c\x90", "MIDI data"),
            ("truncated note", b"\x00\x90\x3c", "Truncated"),
            ("truncated program", b"\x00\xc0", "Truncated"),
            ("bad meta type", b"\x00\xff\x80\x00", "meta-event type"),
            ("EOT length", b"\x00\xff\x2f\x01\x00", "length is not zero"),
            ("EOT trailing bytes", b"\x00\xff\x2f\x00\x00", "Bytes follow"),
            ("short tempo", b"\x00\xff\x51\x02\x01\x01", "length is not three"),
            ("long tempo", b"\x00\xff\x51\x04\x01\x01\x01\x01", "length is not three"),
            ("zero tempo", b"\x00\xff\x51\x03\x00\x00\x00", "nonzero"),
            ("meta boundary", b"\x00\xff\x01\x04\x00", "Meta event exceeds"),
            ("F0 boundary", b"\x00\xf0\x04\x00", "SysEx event exceeds"),
            ("F7 boundary", b"\x00\xf7\x04\x00", "SysEx event exceeds"),
        ]
        invalid_tracks.extend((f"system status {s:02x}", bytes([0, s, 0, 0]),
                               "Unsupported system status")
                              for s in range(0xF1, 0xFF) if s != 0xF7)
        for name, data, err in invalid_tracks:
            check(name, midi([data]), error=err)
        check("track running status isolation", midi([
            encode([ch(0, 0x90, 60, 1), end()]), b"\x00\x3d\x01\x00\xff\x2f\x00"]),
              error="Running status")
        check("bad late event detected in open", midi([
            encode([ch(0, 0x90, 60, 1), end(96)]),
            encode([ch(0, 0x91, 60, 1)]) + b"\x60\x91\x40\xff"]), error="MIDI data")
        check("duration over one hour", midi([encode([end(7201)])], ppqn=1),
              error="duration budget")
        check("duration fractional over one hour", midi([
            encode([tempo(21600, 1), end(1)])], ppqn=3), error="duration budget")
        check("largest tempo times largest delta", midi([
            encode([tempo(0, 0xFFFFFF), end(0x0FFFFFFF)])], ppqn=1),
              error="duration budget")
        check("32-bit absolute tick overflow", midi([encode(
            [tempo(0, 1)] + [ev(0x0FFFFFFF, b"\xff\x01\x00") for _ in range(16)] +
            [end(16)])], ppqn=32767), error="tick overflow")
        low_budget = compile_host("hosttest_budget", "-DSMF_MAX_EVENTS=4")
        check("budget includes skipped meta and EOT", midi([encode([
            ev(0, b"\xff\x01\x00") for _ in range(4)] + [end()])]),
              error="event budget", binary=low_budget)

        # Mutation smoke corpus: acceptance/rejection may vary, crashes may not.
        original = (FIXTURES / "TYPE1.MID").read_bytes()
        for sample in range(400):
            data = bytearray(original)
            for _ in range(rng.randint(1, 8)):
                data[rng.randrange(len(data))] = rng.randrange(256)
            if sample % 3 == 0:
                data = data[:rng.randrange(len(data) + 1)]
            check(f"mutation {sample}", data)
    print(f"PASS: {checked} deterministic cases; exact Fraction timing oracle; rewind/EOF checks")


if __name__ == "__main__":
    main()
