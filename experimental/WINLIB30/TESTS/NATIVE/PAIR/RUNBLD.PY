"""Run only after explicit 180-second DOSBox handoff; never launches 86Box."""
from pathlib import Path
import hashlib, json, os, struct, subprocess, sys, time
W = Path(__file__).resolve().parent
sys.path.insert(0, str(W.parent / 'edge-work'))
from lease import lease, hidden, DOSBOX

ui = sys.argv[1:] == ['--ui-granted-60-seconds']
if not ui and sys.argv[1:] != ['--granted-180-seconds']:
    raise SystemExit('Explicit slot flag required')
tool = W.parent / 'windows-build-view'
out = W / ('UIFIX30' if ui else 'READY30')
jobs = json.loads((out / 'PREP.JSON').read_text())
results = []
hidden()
with lease(out / 'LEASE.JSON'):
    begin = time.monotonic()
    deadline = begin + (50 if ui else 165)
    try:
        # Candidate audio first; baseline second; then the two UI builds.
        for name in (('SHELLNEW', 'SHELLOLD') if ui else
                     ('AUDNEW', 'AUDOLD', 'SHELLNEW', 'SHELLOLD')):
            row = next(x for x in jobs if x['job'] == name)
            directory = out / name
            audio = name.startswith('AUD')
            work = directory / 'MMLAB' if audio else directory
            if time.monotonic() > deadline - 15:
                results.append({'job': name, 'status': 'deferred by slot budget'})
                continue
            config = ('[sdl]\nfullscreen=false\noutput=surface\n'
                '[dosbox]\nmachine=tandy\nmemsize=16\n[cpu]\n'
                'core=normal\ncputype=8086_prefetch\ncycles=fixed 200000\n'
                '[mixer]\nnosound=true\n[midi]\nmididevice=none\n'
                '[autoexec]\n@echo off\nmount C "' + str(tool) +
                '"\nmount D "' + str(directory) + '"\nD:\n' +
                ('CD \\MMLAB\nBUILD30.BAT\n' if audio else 'BUILD.BAT\n'))
            (directory / 'BUILD.CNF').write_text(config)
            env = dict(os.environ, SDL_VIDEODRIVER='dummy',
                       SDL_AUDIODRIVER='dummy')
            start = time.monotonic()
            with (directory / 'HOST.LOG').open('wb') as log:
                p = subprocess.Popen([str(DOSBOX), '-nopromptfolder',
                    '-conf', str(directory / 'BUILD.CNF')], stdout=log,
                    stderr=subprocess.STDOUT, env=env)
                try:
                    p.wait(timeout=max(1, deadline - time.monotonic()))
                finally:
                    if p.poll() is None:
                        p.terminate()
                        try:
                            p.wait(timeout=3)
                        except subprocess.TimeoutExpired:
                            p.kill()
                            p.wait(timeout=3)
            text = (work / 'BUILD.LOG').read_text(errors='replace')
            passed = 'PASS' in text and 'FAIL' not in text
            products = []
            if passed:
                for filename in row['products']:
                    f = work / filename
                    data = bytearray(f.read_bytes())
                    if filename != 'TBRIDGE.EXE':
                        ne = struct.unpack_from('<I', data, 0x3c)[0]
                        assert data[ne:ne+2] == b'NE'
                        struct.pack_into('<H', data, ne+0x3e, 0x0300)
                        f.write_bytes(data)
                    products.append({'name': filename, 'bytes': len(data),
                        'sha256': hashlib.sha256(data).hexdigest()})
            record = {'job': name, 'status': 'PASS' if passed else 'FAIL',
                      'seconds': time.monotonic() - start,
                      'products': products}
            results.append(record)
            print(json.dumps(record), flush=True)
            if not passed:
                break
    finally:
        (out / 'RESULT.JSON').write_text(json.dumps({
            'scope': 'fresh MSC6 /G0 compile/link; native execution pending',
            'elapsed_seconds': time.monotonic() - begin,
            'jobs': results}, indent=2))
