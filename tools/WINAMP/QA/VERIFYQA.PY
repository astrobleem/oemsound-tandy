"""Check complete native observer logs against exact build/guest bytes.

This reads completed native evidence; it never launches an emulator.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
SOURCES = ('UNIFYQA.C', 'UNINTR.C', 'QACOMMON.H', 'PITAPI.H',
           'PITLINK.H', 'PSGLINK.H', 'PITREAD.ASM', 'UNIFYQA.DEF',
           'UNINTR.DEF', 'UNIFYQA.RSP', 'UNINTR.RSP', 'BUILDQA.BAT')


def identity(path):
    data = path.read_bytes()
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(yes, message):
    if not yes:
        raise ValueError(message)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, required=True)
    p.add_argument('--guest', type=Path, required=True)
    p.add_argument('--app', default='MMLAB')
    p.add_argument('--windows', default='WINDOWS')
    p.add_argument('--sound-sha256', required=True)
    p.add_argument('--tshell-sha256', required=True)
    p.add_argument('--winxt', default='WINXT')
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    require(not a.out.exists(), 'Receipt output must be new')
    buildlog = (a.build / 'BUILD.LOG').read_text(errors='replace')
    require('PASS' in buildlog and 'FAIL' not in buildlog,
            'Native build did not finish PASS')
    sources = {}
    for name in SOURCES:
        pin = identity(a.build / name)
        require(pin == identity(HERE / name),
                'Current observer differs from native build: ' + name)
        sources[name] = pin
    runtime = {}
    for name in ('RAWGUI.EXE', 'UNINTR.EXE'):
        pin = identity(a.build / name)
        require(pin == identity(a.guest / a.app / name),
                'Guest observer differs from native build: ' + name)
        runtime[name] = pin
    sound = identity(a.guest / a.windows / 'SYSTEM' / 'SOUND.DRV')
    require(sound['sha256'] == a.sound_sha256.lower(),
            'Guest sound differs from accepted driver')
    logs = {}
    pattern = re.compile(r'^([^\r\n]+) got=(-?\d+) want=(-?\d+)$')
    for name in ('UNIFYQA.LOG', 'UNBUSY.LOG', 'UNFREE.LOG'):
        path = a.guest / name
        lines = path.read_text().splitlines()
        checks = []
        for line in lines:
            match = pattern.fullmatch(line)
            if match:
                label, actual, expected = match.groups()
                require(actual == expected, name + ' failed: ' + line)
                checks.append(label)
        require(lines and lines[-1] == 'TOTAL_FAILURES got=0 want=0',
                name + ' did not complete')
        require(len(checks) > 5, name + ' is incomplete')
        expected = ('loaded_sound=C:\\' + a.windows +
                    '\\SYSTEM\\SOUND.DRV').upper()
        require(any(line.upper() == expected for line in lines),
                name + ' selected a different loaded SOUND path')
        logs[name] = {**identity(path), 'checks': checks}
    require('AllPass' in logs['UNIFYQA.LOG']['checks'],
            'Observer graphics/playback completion missing')
    require('PITEmulatedControlBits' in logs['UNIFYQA.LOG']['checks'],
            'Current emulated port61 assertion missing')
    clients = {name: identity(a.guest / a.app / name) for name in
               ('TCHIME.EXE', 'XPCHIME.EXE', 'TEXIT.EXE', 'TP.EXE')}
    clients['WINDOWS/TSHELL.EXE'] = identity(a.guest / a.windows / 'TSHELL.EXE')
    require(clients['WINDOWS/TSHELL.EXE']['sha256'] ==
            a.tshell_sha256.lower(), 'Installed shell differs from accepted TSHELL')
    for name in ('TCHIME.EXE', 'XPCHIME.EXE', 'TEXIT.EXE'):
        pin = identity(a.guest / a.winxt / name)
        require(pin == clients[name], 'Windows XT chime differs: ' + name)
        clients[a.winxt + '/' + name] = pin
    required_shell = ('TSHELLStartedChime', 'TSHELLChimeOwnedSound',
                      'TSHELLChimeReleasedSound', 'TSHELLUnloaded',
                      'BusyTSHELLNoChime', 'BusyTSHELLPreservedOwner',
                      'BusyTSHELLUnloaded', 'TSHELLFinalSoundAvailable')
    require(all(name in logs['UNIFYQA.LOG']['checks'] for name in required_shell),
            'Actual TSHELL startup/cleanup qualification missing')
    for name, busy in (('TPGOOD.LOG', False), ('TPBUSY.LOG', True)):
        path = a.guest / name
        entries = dict(line.split('=', 1) for line in
                       path.read_text().splitlines() if '=' in line)
        wanted = {'COMPLETE': '1', 'FAIL_COUNT': '0',
                  'FLUSH_TRANSITIONS': '1', 'SOUND_SUPPORTED': '1',
                  'SOUND_STARTED': '0' if busy else '1',
                  'SOUND_COMPLETED': '0' if busy else '1',
                  'SOUND_BUSY_SKIPS': '1' if busy else '0'}
        require(all(entries.get(key) == value for key, value in wanted.items()),
                name + ' did not meet native TP acceptance')
        logs[name] = {**identity(path), 'entries': entries}
    for name, busy in (('TEXGOOD.LOG', False), ('TEXBUSY.LOG', True)):
        path = a.guest / name
        text = path.read_text()
        if busy:
            require('BUSY: NO PSG OR EXIT' in text and 'START' not in text,
                    name + ' did not refuse busy exit')
        else:
            require(all(value in text for value in
                        ('START', 'MUTED WINDOW GONE', 'NO EXIT REQUEST'))
                    and 'LATE ABORT' not in text and 'CALL ExitWindows' not in text,
                    name + ' did not complete preview cleanup')
        logs[name] = identity(path)
    receipt = {'target': '8086 / Windows 3.0 real mode / normal 640KB',
               'driver': sound, 'sources': sources,
               'native_binaries': runtime, 'client_binaries': clients, 'logs': logs,
               'runtime_claim': 'completed production-driver native observer',
               'assertions': sum(len(record.get('checks', []))
                                 for record in logs.values()),
               'port61_readback': 'DOSBox-X stored lower-nibble controls, mask0x0c; gate bits checked separately',
               'physical_hardware': 'not tested'}
    a.out.write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
