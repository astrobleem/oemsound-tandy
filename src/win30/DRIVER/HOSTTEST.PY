"""Run production driver state tests with mocked Win16 and hardware calls."""
from pathlib import Path
import argparse
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    exe = a.out.resolve() / ('HOSTQA.EXE' if os.name == 'nt' else 'hostqa')
    negative = None
    if os.name == 'nt':
        launcher = (Path.home() /
                    'Documents/Codex/toolchains/gcc-ucrt64.py')
        if not launcher.is_file():
            p.error('Required GCC launcher missing: ' + str(launcher))
        compiler = [sys.executable, str(launcher), '--']
        # Missing dependencies must refuse before any compiler launch.
        invalid = subprocess.run([sys.executable, str(launcher), '--root',
                                  str(a.out / 'missing'), '--', '-x', 'c',
                                  '-c', str(HERE / 'HOSTQA.C')],
                                 capture_output=True, text=True)
        negative = (invalid.returncode != 0 and
                    'refusing to start' in invalid.stderr)
        if not negative:
            raise RuntimeError('GCC dependency preflight did not refuse')
    else:
        compiler = ['cc']
    subprocess.run([*compiler, '-x', 'c', '-std=c89', '-Wall', '-Wextra',
                    '-Wno-unused-parameter', '-Werror', '-I', str(HERE / 'HOST'),
                    '-o', str(exe), str(HERE / 'HOSTQA.C')], check=True)
    child_env = os.environ.copy()
    if os.name == 'nt':
        child_env['PATH'] = (r'C:\msys64\ucrt64\bin' + os.pathsep +
                             child_env.get('PATH', ''))
    result = subprocess.run([str(exe)], check=True, env=child_env,
                            capture_output=True, text=True)
    record = {'output': result.stdout.strip(),
              'windows_gcc_launcher': str(launcher) if os.name == 'nt' else None,
              'missing_toolchain_refused_before_launch': negative,
              'scope': 'real driver state machine; mocked Win16 and hardware'}
    (a.out / 'RESULT.JSON').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
