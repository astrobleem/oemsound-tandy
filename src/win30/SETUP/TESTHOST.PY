"""Compile and run installer core tests with the mandatory Windows GCC route."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import tempfile
import uuid

HERE = Path(__file__).resolve().parent
GCC = Path(r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc-launcher', type=Path, default=GCC)
    args = parser.parse_args()
    if args.gcc_launcher.resolve() != GCC.resolve():
        parser.error('Windows GCC must use the shared dependency-preflight launcher')
    if os.name != 'nt':
        parser.error('This filesystem harness targets Windows host semantics')
    root = Path(tempfile.gettempdir()) / ('TS' + uuid.uuid4().hex[:6])
    root.mkdir(exist_ok=False)
    exe = root / 'TESTSET.EXE'
    command = [os.sys.executable, str(args.gcc_launcher), '--', '-x', 'c',
               '-std=c89', '-Wall', '-Wextra', '-Werror',
               '-Wno-unused-function', '-I', str(HERE.parent / 'DRIVER'),
               '-I', str(HERE.parent / 'LAUNCH'), '-o', str(exe),
               str(HERE / 'TESTSET.C')]
    try:
        subprocess.run(command, check=True, timeout=60)
        env = os.environ.copy()
        env['PATH'] = r'C:\msys64\ucrt64\bin' + os.pathsep + env.get('PATH', '')
        result = subprocess.run([str(exe), str(root)], env=env,
                                capture_output=True, text=True,
                                timeout=30, check=True)
        print(result.stdout, end='')
        print(json.dumps({'gcc_route': str(args.gcc_launcher),
                          'result': 'pass', 'native_emulator': False}))
    finally:
        # Only the exact newly-created, owned temporary test tree is removed.
        if (root.resolve().parent != Path(tempfile.gettempdir()).resolve()
                or len(root.name) != 8 or not root.name.startswith('TS')):
            raise RuntimeError('Refusing cleanup outside temporary root')
        shutil.rmtree(root)


if __name__ == '__main__':
    main()
