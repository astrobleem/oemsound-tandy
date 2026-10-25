"""Guard one installed DOSBox-X child with a shared Windows emulator lease."""
from pathlib import Path
import datetime
import hashlib
import json
import msvcrt
import os
import subprocess


def run(exe, config, lease_path, timeout=60):
    exe, config = Path(exe).resolve(), Path(config).resolve()
    out = config.parent
    result_path = out / 'PROCESS.JSON'
    if result_path.exists():
        raise ValueError('A fresh run directory is required')
    if not 1 <= timeout <= 180:
        raise ValueError('Timeout must be 1..180 seconds')
    result = {'started': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'emulator': str(exe), 'config': str(config),
              'emulator_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
              'timeout': timeout}
    child = None
    try:
        with Path(lease_path).open('rb') as lease:
            if not lease.read(1):
                raise ValueError('Shared lease is empty; do not initialize it here')
            lease.seek(0)
            msvcrt.locking(lease.fileno(), msvcrt.LK_NBLCK, 1)
            result['lease_acquired'] = True
            try:
                inventory = subprocess.run(
                    ['powershell.exe', '-NoProfile', '-NonInteractive', '-Command',
                     "Get-Process -ErrorAction Stop | Where-Object { "
                     "$_.ProcessName -match '^(dosbox-x|dosbox|86Box|pcem|"
                     "tandybox|FIXBOX|TRACEIO|Mesen|Mesen2)$' } | "
                     "Select-Object ProcessName,Id"],
                    capture_output=True, text=True, timeout=10)
                result['inventory'] = inventory.stdout
                if inventory.returncode != 0 or inventory.stdout.strip():
                    raise RuntimeError('Existing emulator or unavailable inventory')
                env = os.environ.copy()
                env.update(SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
                startup = subprocess.STARTUPINFO()
                startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                startup.wShowWindow = 0
                with (out / 'HOST.LOG').open('wb') as log:
                    child = subprocess.Popen(
                        [str(exe), '-nopromptfolder', '-conf', str(config)],
                        cwd=out, stdout=log, stderr=subprocess.STDOUT, env=env,
                        startupinfo=startup, creationflags=subprocess.CREATE_NO_WINDOW)
                    result['pid'] = child.pid
                    print('RUNNING', child.pid, flush=True)
                    result['return'] = child.wait(timeout=timeout)
            finally:
                if child is not None and child.poll() is None:
                    child.kill()
                    child.wait(timeout=5)
                    result['watchdog'] = True
                result['owned_child_stopped'] = child is None or child.poll() is not None
                lease.seek(0)
                msvcrt.locking(lease.fileno(), msvcrt.LK_UNLCK, 1)
                result['lease_released'] = True
    except Exception as error:
        result['error'] = str(error)
    finally:
        result_path.write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result), flush=True)
    if result.get('return') != 0 or 'error' in result:
        raise RuntimeError('Emulator did not complete normally: ' + str(result_path))
    return result


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dosbox', type=Path, required=True)
    p.add_argument('--config', type=Path, required=True)
    p.add_argument('--lease', type=Path, required=True)
    p.add_argument('--timeout', type=int, default=60)
    a = p.parse_args()
    run(a.dosbox, a.config, a.lease, a.timeout)
