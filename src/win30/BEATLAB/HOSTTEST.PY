"""Test Beats state/files and each demo through the shipped mapper; no emulator."""
from pathlib import Path
import argparse
import array
import csv
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import wave

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
TESTS = HERE / 'TESTS'
REPO_ARCHIVE = ROOT / 'experimental/PITPUBLIC/MML3AUTH/SOURCE/src/win30'
ARCHIVE = REPO_ARCHIVE if all((REPO_ARCHIVE / name).is_file() for name in
    ('BEATMML/MIDIMAP.C', 'MMLFIDL/MMLCORE.C')) else HERE.parent
WINDOWS_GCC = Path('C:/Users/chad/Documents/Codex/toolchains/gcc-ucrt64.py')
RATE = 22050
GAIN = 0.18


def pcm_reference(out):
    """Fixed-gain, deterministic reference mix; never an analog fidelity claim."""
    with (out / 'demos.csv').open(newline='') as stream:
        demos = list(csv.DictReader(stream))
    with (out / 'attacks.csv').open(newline='') as stream:
        attacks = [{key: int(value) for key, value in row.items()}
                   for row in csv.DictReader(stream)]
    report = []
    for demo in demos:
        index = int(demo['demo'])
        with (out / f'demo-{index}.csv').open(newline='') as stream:
            trace = [{key: int(value) for key, value in row.items()}
                     for row in csv.DictReader(stream)]
        count = len(trace) * RATE // 1000
        voices = [array.array('f') for unused in range(5)]
        mixed = array.array('h')
        phases = [0.0] * 4
        noise_phase, noise_state, last_noise = 0.0, 0x4000, None
        peak, clipped = 0.0, 0
        lane_energy = [0.0] * 5
        lane_peak = [0.0] * 5
        for sample in range(count):
            row = trace[min(len(trace) - 1, sample * 1000 // RATE)]
            values = [0.0] * 5
            for lane in range(3):
                period, volume = row[f'p{lane}'], row[f'v{lane}']
                if period and volume < 15:
                    phases[lane] = (phases[lane] + 3579545.0 / (32 * period * RATE)) % 1.0
                    values[lane] = (1.0 if phases[lane] < 0.5 else -1.0) * 10 ** (-volume / 10)
            if row['noise'] != last_noise:
                noise_state, noise_phase = 0x4000, 0.0
                last_noise = row['noise']
            if row['v3'] < 15:
                mode = row['noise'] & 3
                divider = (16 << mode) if mode < 3 else max(1, row['p2'])
                noise_phase += 3579545.0 / (32 * divider * RATE)
                while noise_phase >= 1:
                    bit = ((noise_state ^ (noise_state >> 3)) & 1) if row['noise'] & 4 else noise_state & 1
                    noise_state = (noise_state >> 1) | (bit << 14)
                    noise_phase -= 1
                values[3] = (1.0 if noise_state & 1 else -1.0) * 10 ** (-row['v3'] / 10)
            if row['pit_hz']:
                phases[3] = (phases[3] + row['pit_hz'] / RATE) % 1.0
                values[4] = 1.0 if phases[3] < 0.5 else -1.0
            total = sum(values) * GAIN
            peak = max(peak, abs(total))
            clipped += abs(total) >= 1.0
            mixed.append(max(-32768, min(32767, round(total * 32767))))
            for lane, value in enumerate(values):
                voices[lane].append(value)
                lane_energy[lane] += value * value
                lane_peak[lane] = max(lane_peak[lane], abs(value))
        wav = out / f'demo-{index}.wav'
        if sys.byteorder != 'little':
            mixed.byteswap()
        with wave.open(str(wav), 'wb') as stream:
            stream.setnchannels(1)
            stream.setsampwidth(2)
            stream.setframerate(RATE)
            stream.writeframes(mixed.tobytes())
        note_results = []
        for attack in (a for a in attacks if a['demo'] == index):
            lane = attack['slot']
            start = (attack['start_ms'] * RATE + 999) // 1000
            stop = attack['end_ms'] * RATE // 1000
            signal = voices[lane][start:stop]
            note_peak = max(map(abs, signal), default=0.0)
            energy = sum(value * value for value in signal)
            note_rms = math.sqrt(energy / max(1, len(signal)))
            mean = sum(signal) / max(1, len(signal))
            ac_rms = math.sqrt(max(0.0, energy / max(1, len(signal)) - mean * mean))
            span = max(signal, default=0.0) - min(signal, default=0.0)
            if not signal or note_peak < 1e-5 or note_rms < 1e-5 or ac_rms < 1e-5 or span < 1e-5:
                raise RuntimeError('Unintended silent attack: ' + json.dumps(attack))
            after = voices[lane][(attack['end_ms'] * RATE + 999) // 1000:
                                  min(len(voices[lane]), ((attack['end_ms'] + 8) * RATE) // 1000)]
            if any(abs(value) > 1e-7 for value in after):
                raise RuntimeError('Voice did not release at gate: ' + json.dumps(attack))
            note_results.append(dict(attack, peak=note_peak, rms=note_rms,
                                     ac_rms=ac_rms, peak_to_peak=span,
                                     audible_samples=sum(abs(value) > 1e-7 for value in signal)))
        if clipped:
            raise RuntimeError(f'Demo {index} clipped {clipped} samples at fixed gain')
        if index == 3:
            answer_notes = [n for n in note_results if n['lane'] == 4 and n['step'] & 1]
            if len(answer_notes) != 8:
                raise RuntimeError('Speaker-answer demo lacks eight isolated PIT answers')
            for note in answer_notes:
                begin, end = note['start_ms'], note['end_ms']
                if any(any(row[f'v{i}'] < 15 for i in range(4)) for row in trace[begin:end]):
                    raise RuntimeError('Speaker answer overlaps a PSG voice')
        row = dict(demo, sample_rate=RATE, mix_gain=GAIN, peak=peak,
                   raw_mix_peak=peak / GAIN,
                   clipped_samples=clipped, intended_attacks=len(note_results),
                   lane_rms=[math.sqrt(value / max(1, count)) for value in lane_energy],
                   lane_peak=lane_peak, notes=note_results,
                   wav=wav.name, wav_sha256=hashlib.sha256(wav.read_bytes()).hexdigest())
        report.append(row)
    result = {
        'scope': 'Actual shipped mapper state with mocked SOUND/ports; fixed-gain software reference audio, not native timing or analog/hardware fidelity.',
        'sample_rate': RATE, 'mix_gain': GAIN, 'demos': report,
        'intended_attacks': sum(r['intended_attacks'] for r in report),
        'silent_attacks': 0, 'clipped_samples': 0,
    }
    (out / 'AUDIO.JSON').write_text(json.dumps(result, indent=2) + '\n')
    print(f"PASS audio demos={len(report)} attacks={result['intended_attacks']} silent=0 clipped=0 peak={max(r['peak'] for r in report):.4f}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True, help='New private evidence directory')
    parser.add_argument('--cc', help='POSIX compiler, or the established Windows launcher')
    args = parser.parse_args()
    for required in ('BEATMML/MIDIMAP.C', 'BEATMML/INSTR.C', 'MMLFIDL/MMLCORE.C'):
        if not (ARCHIVE / required).is_file():
            parser.error('Required shipped regression source is missing: ' + str(ARCHIVE / required))
    out = args.out.resolve()
    if out.exists():
        parser.error('--out must be new')
    if os.name == 'nt':
        if args.cc and Path(args.cc).resolve() != WINDOWS_GCC:
            parser.error('Windows --cc must name the established gcc-ucrt64.py launcher')
        if not WINDOWS_GCC.is_file():
            parser.error('Required GCC launcher is missing: ' + str(WINDOWS_GCC))
        compiler = [sys.executable, str(WINDOWS_GCC), '--']
    else:
        compiler = [args.cc or shutil.which('cc') or 'gcc']
    out.mkdir(parents=True)
    records = []
    runtime = dict(os.environ)
    if os.name == 'nt':
        runtime['PATH'] = 'C:/msys64/ucrt64/bin' + os.pathsep + runtime.get('PATH', '')

    def run(argv, label):
        result = subprocess.run(list(map(str, argv)), cwd=out, env=runtime,
                                capture_output=True, text=True, timeout=35)
        record = dict(label=label, command=list(map(str, argv)), exit=result.returncode,
                      stdout=result.stdout, stderr=result.stderr)
        records.append(record)
        (out / 'BUILD.JSON').write_text(json.dumps(records, indent=2) + '\n')
        if result.returncode:
            raise RuntimeError(label + ': ' + result.stdout + result.stderr)
        if label.endswith('run'):
            print(result.stdout.strip())

    common = ['-x', 'c', '-Wall', '-Wextra', '-Werror',
              '-Wno-misleading-indentation', '-Wno-unused-function',
              '-I', HERE, '-I', ARCHIVE / 'MMLFIDL']
    suffix = '.exe' if os.name == 'nt' else ''
    cases = [
        ('core', ['-std=c89', '-pedantic-errors'],
         [TESTS / 'core.c', HERE / 'BEATPAT.C', HERE / 'BEATEXP.C', ARCHIVE / 'MMLFIDL/MMLCORE.C']),
        ('file_faults', ['-std=c89', '-pedantic-errors'],
         [TESTS / 'file_faults.c', HERE / 'BEATEXP.C']),
        ('mapper', ['-std=c99', '-I', TESTS,
                    '-DBEATS_MAPPER_SOURCE="' + (ARCHIVE / 'BEATMML/MIDIMAP.C').as_posix() + '"',
                    '-Wno-int-to-pointer-cast',
                    '-Wno-pointer-to-int-cast', '-Wno-unused-parameter', '-Wno-sign-compare'],
         [TESTS / 'mapper.c', HERE / 'BEATPAT.C', HERE / 'BEATEXP.C', ARCHIVE / 'BEATMML/INSTR.C']),
    ]
    for name, flags, sources in cases:
        exe = out / (name + suffix)
        run(compiler + common + flags + sources + ['-o', exe], name + ' compile')
        run([exe], name + ' run')
    audio = pcm_reference(out)
    (out / 'RESULT.JSON').write_text(json.dumps({
        'status': 'PASS', 'cases': [c[0] for c in cases],
        'audio': {'demos': len(audio['demos']), 'attacks': audio['intended_attacks'],
                  'silent_attacks': 0, 'clipped_samples': 0},
        'scope': 'Host-only source/files/mapper checks and software reference audio; no emulator/native claim.'
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
