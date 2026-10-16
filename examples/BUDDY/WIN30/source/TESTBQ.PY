"""Compile/run C89 client logic on Windows; no emulator or media needed."""
import argparse
import json
import os
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--cc', type=Path, required=True)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=False)
source = Path(__file__).resolve().parent
env = os.environ.copy()
env['PATH'] = str(args.cc.resolve().parent) + os.pathsep + env['PATH']
native = (source / 'TINYVID.C').read_text()
bodies = []
for name in ('fetchEvent', 'applyState', 'musicAt', 'movieNow', 'updateMusic'):
    match = re.search(r'^static (?:int|DWORD) ' + name +
                      r'\([^\n]*\)\n\{\n.*?^\}\n', native, re.M | re.S)
    if not match:
        raise RuntimeError('Missing production function: ' + name)
    bodies.append(match.group())
(args.out / 'BQPROD.H').write_text('\n'.join(bodies))
results = []
for name in ('BQTEST', 'BQINTEG'):
    exe = args.out.resolve() / (name + '.EXE')
    build = subprocess.run([str(args.cc.resolve()), '-x', 'c', '-std=c89',
                            '-Wall', '-Wextra', '-Werror', '-pedantic',
                            '-I', str(args.out.resolve()),
                            str(source / (name + '.C')), '-o', str(exe)],
                           capture_output=True, text=True, env=env)
    (args.out / (name + '-BUILD.LOG')).write_text(build.stdout + build.stderr)
    build.check_returncode()
    test = subprocess.run([str(exe)], capture_output=True, text=True, env=env)
    (args.out / (name + '-TEST.LOG')).write_text(test.stdout + test.stderr)
    print(test.stdout, end='')
    test.check_returncode()
    results.append(test.stdout.strip())
(args.out / 'RESULT.JSON').write_text(json.dumps({
    'pass': True, 'results': results,
    'scope': 'C89 scheduling logic with a synthetic backend; not Win16 ABI, '
             'native timing, hardware, or listening qualification'
}, indent=2) + '\n')
