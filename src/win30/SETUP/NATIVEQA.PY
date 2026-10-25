"""Prepare/verify bounded DOS installer acceptance; never launches an emulator."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import zlib

CLIENTS = (('W', 'TSHELL.EXE'), ('X', 'TCHIME.EXE'), ('X', 'XPCHIME.EXE'),
           ('X', 'TEXIT.EXE'), ('X', 'TP.EXE'), ('X', 'FIXXT.EXE'),
           ('W', 'TSINPUT.DLL'))

CASES = ('01', '02', '03', '04A', '04B', '05', '06A',
         '06B', '06C', '07', '08', '09')


def pin(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def rows(package):
    result = []
    text = (package / 'PAYLOAD.LST').read_text()
    if text.splitlines()[0] != 'TWSETUP1':
        raise ValueError('Expected native installer payload index')
    for line in text.splitlines()[1:]:
        if not line:
            continue
        kind, source, dest, size, crc = line.split('|')
        if '/' in source or '\\' in source or source in ('.', '..'):
            raise ValueError('Unsafe payload source')
        data = (package / source).read_bytes()
        if len(data) != int(size) or zlib.crc32(data) != int(crc, 16):
            raise ValueError('Package pin mismatch: ' + source)
        result.append((kind, source, dest))
    return result


def index(directory, entries):
    text = ['TWSETUP1']
    for kind, source, dest in sorted(entries):
        data = (directory / source).read_bytes()
        text.append(f'{kind}|{source}|{dest}|{len(data)}|{zlib.crc32(data):08x}')
    (directory / 'PAYLOAD.LST').write_bytes(('\r\n'.join(text) + '\r\n').encode())


def prepare(args):
    out = args.out.resolve()
    if out.exists():
        raise ValueError('Use a fresh output directory')
    out.mkdir(parents=True)
    croot = out / 'CROOT'
    croot.mkdir()
    shutil.copytree(args.windows, croot / 'WINDOWS')
    windows = croot / 'WINDOWS'
    original_ini = (windows / 'SYSTEM.INI').read_bytes()
    if b'display.drv=XRSYNC.DRV' not in original_ini:
        raise ValueError('Use the known XRSYNC Windows fixture for preflight failure test')
    if (windows / 'SOUND.DRV').exists():
        raise ValueError('Use a fixture without a root SOUND.DRV shadow')
    original_driver = (windows / 'SYSTEM/SOUND.DRV').read_bytes()
    p0 = croot / 'P0'
    p0.mkdir()
    entries = rows(args.package)
    for _, source, _ in entries:
        shutil.copyfile(args.package / source, p0 / source)
    shutil.copyfile(args.setup, p0 / 'SETUP.EXE')
    if args.driver:
        shutil.copyfile(args.driver, p0 / 'SOUND.DRV')
    if args.launcher:
        shutil.copyfile(args.launcher, p0 / 'TPLAY.EXE')
    if args.rawclk:
        shutil.copyfile(args.rawclk, p0 / 'RAWCLK.DLL')
    if args.clients:
        for kind, name in CLIENTS:
            source = args.clients / name
            if not source.exists():
                continue
            shutil.copyfile(source, p0 / name)
            entries = [entry for entry in entries if entry[2] != name]
            entries.append((kind, name, name))
    shared, absent = {}, []
    for kind, source, name in entries:
        if kind not in ('W', 'X'):
            continue
        paths = [('WINDOWS/' if kind == 'W' else 'WINXT/') + name,
                 'WINXT/FILES/' + name]
        for relative in paths:
            target = croot / relative
            if args.xt_before and relative == 'WINDOWS/TSINPUT.DLL':
                # Current TSHELL requires this paired DLL even when missing.
                if target.exists():
                    target.unlink()
                shared[relative] = pin((p0 / source).read_bytes())
                continue
            if args.xt_before:
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(args.xt_before / name, target)
            if target.exists():
                shared[relative] = pin((p0 / source).read_bytes())
            else:
                absent.append(relative)
    if args.xt_before:
        shared['WINXT/FILES/SOUND.DRV'] = pin((p0 / 'SOUND.DRV').read_bytes())
    else:
        absent.append('WINXT/FILES/SOUND.DRV')
    # One synthetic text file exercises repair; musical data stays exact.
    (p0 / 'BASE.TXT').write_bytes(b'version one\r\n')
    entries.append(('A', 'BASE.TXT', 'BASE.TXT'))
    index(p0, entries)
    pfail = croot / 'PFAIL'
    shutil.copytree(p0, pfail)
    shutil.copyfile(args.setup_test, pfail / 'SETUP.EXE')
    index(pfail, entries)
    for name in ('PMISS', 'PBAD', 'PSOUND', 'PREFUSE'):
        shutil.copytree(p0, croot / name)
    (croot / 'PMISS/LWFILE.DLL').unlink()
    bad = croot / 'PBAD/RAWCLK.DLL'
    data = bytearray(bad.read_bytes()); data[-1] ^= 1; bad.write_bytes(data)
    bad = croot / 'PSOUND/SOUND.DRV'
    data = bytearray(bad.read_bytes()); data[-1] ^= 1; bad.write_bytes(data)
    index(croot / 'PSOUND', entries)
    bad = croot / 'PREFUSE/RESERVE.COM'
    data = bytearray(bad.read_bytes()); data[-1] ^= 1; bad.write_bytes(data)
    index(croot / 'PREFUSE', entries)
    fixtures = croot / 'FIX'
    fixtures.mkdir()
    (fixtures / 'FOREIGN.TXT').write_bytes(b'unrelated foreign file\r\n')
    (fixtures / 'CHANGED.TXT').write_bytes(b'user changed this text\r\n')
    append = b'\r\n[InstallerQA]\r\nKeepThisSetting=YES\r\n'
    (fixtures / 'APPEND.INI').write_bytes(append)
    (fixtures / 'OWNER.DAT').write_bytes(b'TWSTAGE1\r\nC:\\wamp\r\n')
    (fixtures / 'TXN.DAT').write_bytes(
        b'TWTRANS1\r\nC:\\WINDOWS\r\n1\r\n1|C:\\WAMP\\BASE.TXT\r\n')
    (fixtures / 'R000.BAK').write_bytes(b'version one\r\n')
    if args.xt_before:
        shutil.copyfile(args.xt_before / 'TP.EXE', fixtures / 'TPOLD.EXE')
    (croot / 'LOGS').mkdir()
    snapshots = croot / 'SNAPS'
    snapshots.mkdir()
    commands = ['@ECHO OFF', 'SET WINDIR=', 'SET PATH=C:\\DOS',
                'ECHO START > C:\\RESULTS.TXT']
    cases = list(CASES)
    unchanged = ['04A', '04B', '05', '06A', '06B', '06C', '07']
    appnames = sorted({entry[2] for entry in entries if entry[0] != 'S'} |
                      {'PAYLOAD.LST', 'GO.BAT', 'TWSTATE.DAT', 'USER.TXT'})

    def snap(label):
        target = snapshots / label
        target.mkdir(exist_ok=True)
        commands.append(f'COPY C:\\WINDOWS\\SYSTEM.INI C:\\SNAPS\\{label}\\INI.TXT > NUL')
        commands.append(f'COPY C:\\WINDOWS\\SYSTEM\\SOUND.DRV C:\\SNAPS\\{label}\\DRIVER.BIN > NUL')
        for name in appnames:
            commands.append(f'IF EXIST C:\\WAMP\\{name} COPY C:\\WAMP\\{name} C:\\SNAPS\\{label}\\{name} > NUL')

    def action(label, program, options, failure=False):
        commands.append(f'{program} {options} > C:\\LOGS\\{label}.TXT')
        if failure:
            commands.extend([f'IF ERRORLEVEL 1 GOTO OK{label}', f'GOTO BAD{label}',
                             f':OK{label}'])
        else:
            commands.append(f'IF ERRORLEVEL 1 GOTO BAD{label}')
        commands.append(f'ECHO {label} PASS >> C:\\RESULTS.TXT')

    def run(label, package, failure=False, extra=''):
        action(label, f'C:\\{package}\\SETUP.EXE',
               '/REPAIR /APP:C:\\WAMP /WIN:C:\\WINDOWS /QUIET ' + extra,
               failure)

    if args.xt_before:
        cases = ['00', '00D', '00M'] + cases
        unchanged += ['00', '00D', '00M']
        commands.append('COPY C:\\FIX\\FOREIGN.TXT C:\\WINXT\\TP.EXE > NUL')
        snap('B00')
        action('00', 'C:\\P0\\SETUP.EXE',
               '/INSTALL /APP:C:\\WAMP /WIN:C:\\WINDOWS /QUIET', True)
        snap('A00')
        commands += ['COPY C:\\WINXT\\TP.EXE C:\\FIX\\CLIENT0.OUT > NUL',
                     'COPY C:\\FIX\\TPOLD.EXE C:\\WINXT\\TP.EXE > NUL']
        commands.append('COPY C:\\FIX\\FOREIGN.TXT C:\\WINDOWS\\TSINPUT.DLL > NUL')
        snap('B00D')
        action('00D', 'C:\\P0\\SETUP.EXE',
               '/INSTALL /APP:C:\\WAMP /WIN:C:\\WINDOWS /QUIET', True)
        snap('A00D')
        commands += ['COPY C:\\WINDOWS\\TSINPUT.DLL C:\\FIX\\INPUT0.OUT > NUL',
                     'DEL C:\\WINDOWS\\TSINPUT.DLL']
        commands.append('COPY C:\\FIX\\FOREIGN.TXT C:\\WINXT\\FILES\\SOUND.DRV > NUL')
        snap('B00M')
        action('00M', 'C:\\P0\\SETUP.EXE',
               '/INSTALL /APP:C:\\WAMP /WIN:C:\\WINDOWS /QUIET', True)
        snap('A00M')
        commands += ['COPY C:\\WINXT\\FILES\\SOUND.DRV C:\\FIX\\MIRROR0.OUT > NUL',
                     'DEL C:\\WINXT\\FILES\\SOUND.DRV']
    action('01', 'C:\\P0\\SETUP.EXE', '/INSTALL /APP:C:\\WAMP /QUIET')
    snap('R01')
    action('02', 'C:\\WAMP\\SETUP.EXE', '/REPAIR /QUIET')
    snap('R02')
    commands.append('DEL C:\\WAMP\\BASE.TXT')
    run('03', 'P0')
    snap('R03')
    if args.xt_before:
        cases.append('03C')
        commands += ['COPY C:\\FIX\\FOREIGN.TXT C:\\WINXT\\TP.EXE > NUL',
                     'DEL C:\\WINXT\\FILES\\TP.EXE',
                     'DEL C:\\WINDOWS\\TSINPUT.DLL',
                     'COPY C:\\FIX\\FOREIGN.TXT C:\\WINXT\\FILES\\SOUND.DRV > NUL']
        run('03C', 'P0')
        snap('R03C')
        cases.append('0F')
        unchanged.append('0F')
        snap('B0F')
        commands.append('REN C:\\WINXT\\FILES SAVFILES')
        action('0F', 'C:\\PREFUSE\\SETUP.EXE',
               '/INSTALL /APP:C:\\BADWAMP /WIN:C:\\WINDOWS /QUIET', True)
        commands += ['IF EXIST C:\\WINXT\\FILES GOTO BAD0F',
                     'REN C:\\WINXT\\SAVFILES FILES']
        snap('A0F')
    commands += ['MD C:\\COLLIDE', 'MD C:\\COLLIDE\\TWSTAGE',
                 'COPY C:\\FIX\\FOREIGN.TXT C:\\COLLIDE\\TWSTAGE\\NOTES.TXT > NUL',
                 'COPY C:\\FIX\\FOREIGN.TXT C:\\COLLIDE\\MINIPLAY.EXE > NUL']
    snap('B04A')
    action('04A', 'C:\\P0\\SETUP.EXE',
           '/INSTALL /APP:C:\\COLLIDE /WIN:C:\\WINDOWS /QUIET', True)
    snap('A04A')
    commands += ['COPY C:\\COLLIDE\\TWSTAGE\\NOTES.TXT C:\\FIX\\STAGE.OUT > NUL',
                 'DEL C:\\COLLIDE\\TWSTAGE\\NOTES.TXT',
                 'RD C:\\COLLIDE\\TWSTAGE']
    snap('B04B')
    action('04B', 'C:\\P0\\SETUP.EXE',
           '/INSTALL /APP:C:\\COLLIDE /WIN:C:\\WINDOWS /QUIET', True)
    snap('A04B')
    commands += ['COPY C:\\COLLIDE\\MINIPLAY.EXE C:\\FIX\\COLLIDE.OUT > NUL',
                 'COPY C:\\FIX\\FOREIGN.TXT C:\\WINDOWS\\SOUND.DRV > NUL']
    # New root shadows must be preserved by current-install repair.
    commands.append('C:\\P0\\SETUP.EXE /REPAIR /APP:C:\\WAMP /WIN:C:\\WINDOWS /QUIET > C:\\LOGS\\SHADOW.TXT')
    commands += ['IF NOT ERRORLEVEL 1 GOTO BAD05',
                 'COPY C:\\WINDOWS\\SOUND.DRV C:\\FIX\\SHADOW.OUT > NUL',
                 'DEL C:\\WINDOWS\\SOUND.DRV']
    snap('B05')
    run('05', 'PFAIL', True, '/FAIL:4')
    snap('A05')
    for label, package in (('06A', 'PMISS'), ('06B', 'PBAD'), ('06C', 'PSOUND')):
        snap('B' + label)
        run(label, package, True)
        snap('A' + label)
    snap('B07')
    run('07', 'PREFUSE', True)
    snap('A07')
    commands += ['MD C:\\WAMP\\TWSTAGE',
                 'COPY C:\\FIX\\OWNER.DAT C:\\WAMP\\TWSTAGE\\OWNER.DAT > NUL',
                 'COPY C:\\FIX\\TXN.DAT C:\\WAMP\\TWSTAGE\\TXN.DAT > NUL',
                 'COPY C:\\FIX\\R000.BAK C:\\WAMP\\TWSTAGE\\R000.BAK > NUL',
                 'COPY C:\\FIX\\FOREIGN.TXT C:\\WAMP\\BASE.TXT > NUL']
    action('08', 'C:\\WAMP\\SETUP.EXE', '/REPAIR /QUIET')
    snap('R08')
    commands += ['TYPE C:\\FIX\\APPEND.INI >> C:\\WINDOWS\\SYSTEM.INI',
                 'COPY C:\\FIX\\FOREIGN.TXT C:\\WAMP\\USER.TXT > NUL',
                 'COPY C:\\FIX\\CHANGED.TXT C:\\WAMP\\BASE.TXT > NUL']
    action('09', 'C:\\WAMP\\SETUP.EXE', '/UNINSTALL /QUIET')
    snap('R09')
    if args.failures_only:
        commands = ['@ECHO OFF', 'SET WINDIR=', 'SET PATH=C:\\DOS',
                    'ECHO START > C:\\RESULTS.TXT']
        cases, unchanged = ['01', '05', '07'], ['05', '07']
        action('01', 'C:\\P0\\SETUP.EXE', '/INSTALL /APP:C:\\WAMP /QUIET')
        snap('R01')
        snap('B05'); run('05', 'PFAIL', True, '/FAIL:4'); snap('A05')
        snap('B07'); run('07', 'PREFUSE', True); snap('A07')
        if args.xt_before:
            cases.append('0F'); unchanged.append('0F')
            snap('B0F')
            commands.append('REN C:\\WINXT\\FILES SAVFILES')
            action('0F', 'C:\\PREFUSE\\SETUP.EXE',
                   '/INSTALL /APP:C:\\BADWAMP /WIN:C:\\WINDOWS /QUIET', True)
            commands += ['IF EXIST C:\\WINXT\\FILES GOTO BAD0F',
                         'REN C:\\WINXT\\SAVFILES FILES']
            snap('A0F')
    commands += ['ECHO ALL PASS >> C:\\RESULTS.TXT', 'GOTO END']
    for label in cases:
        commands += [f':BAD{label}', f'ECHO {label} FAIL >> C:\\RESULTS.TXT',
                     'GOTO END']
    commands += [':END', 'EXIT']
    (croot / 'TEST.BAT').write_bytes(('\r\n'.join(commands) + '\r\n').encode())
    config = f'''[sdl]
fullscreen=false
output=surface
[dosbox]
machine=tandy
memsize=1
allow more than 640kb base memory=false
working directory option=force
working directory default={out}
quit warning=false
[cpu]
core=normal
cputype=8086_prefetch
cycles=fixed 200000
fpu=false
[dos]
xms=false
ems=false
umb=false
[mixer]
nosound=true
[midi]
mididevice=none
[autoexec]
@echo off
mount C "{croot}"
C:
TEST.BAT
exit
'''
    (out / 'RUN.CNF').write_text(config)
    expectation = {'cases': cases, 'failures_only': args.failures_only, 'inputs': {
        'setup': pin(args.setup.read_bytes()),
        'setup_test': pin(args.setup_test.read_bytes()),
        'sound': pin((p0 / 'SOUND.DRV').read_bytes()),
        'launcher': pin((p0 / 'TPLAY.EXE').read_bytes()),
        'original_sound': pin(original_driver),
        'original_ini': pin(original_ini)},
        'unchanged_pairs': unchanged,
        'shared_clients': shared, 'absent_optional_clients': absent,
        'appended_ini': append.decode(),
        'final_files': {'BASE.TXT': pin((fixtures / 'CHANGED.TXT').read_bytes()),
                        'USER.TXT': pin((fixtures / 'FOREIGN.TXT').read_bytes())}}
    (out / 'EXPECT.JSON').write_text(json.dumps(expectation, indent=2) + '\n')
    print(json.dumps({'prepared': str(out), 'native_cases': len(cases),
                      'emulator_launched': False}, indent=2))


def snapshot(path):
    return {file.name: pin(file.read_bytes()) for file in sorted(path.iterdir())
            if file.is_file()}


def prooflogs(croot, cases):
    injected = (croot / 'LOGS/05.TXT').read_text()
    if ('SETUP TEST: injected commit failure after 4 writes.' not in injected or
            'previous installation was restored' not in injected):
        raise AssertionError('Injected failure with completed rollback was not exercised')
    for label in ('07', '0F'):
        if label not in cases:
            continue
        text = (croot / ('LOGS/' + label + '.TXT')).read_text()
        if ('TPLAY REFUSED' not in text or
                'Keep the qualified RESERVE.COM' not in text or
                'previous installation was restored' not in text):
            raise AssertionError('Actual launch preflight rollback was not exercised: ' + label)


def verify(args):
    out = args.out.resolve()
    croot = out / 'CROOT'
    expected = json.loads((out / 'EXPECT.JSON').read_text())
    results = (croot / 'RESULTS.TXT').read_text().splitlines()
    for label in expected['cases']:
        if f'{label} PASS' not in results or f'{label} FAIL' in results:
            raise AssertionError('Native case failed or did not complete: ' + label)
    if 'ALL PASS' not in results:
        raise AssertionError('Native batch did not finish')
    snapshots = croot / 'SNAPS'
    for label in expected['unchanged_pairs']:
        if snapshot(snapshots / ('B' + label)) != snapshot(snapshots / ('A' + label)):
            raise AssertionError('Refusal/rollback changed files: ' + label)
    prooflogs(croot, expected['cases'])
    if expected.get('failures_only'):
        if (croot / 'BADWAMP').exists() or (croot / 'WAMP/TWSTAGE').exists():
            raise AssertionError('Failed operation left app/private staging behind')
        if pin((croot / 'WINDOWS/SYSTEM/SOUND.DRV').read_bytes()) != expected['inputs']['sound']:
            raise AssertionError('Failed operations changed canonical sound')
        for relative, identity in expected['shared_clients'].items():
            if pin((croot / relative).read_bytes()) != identity:
                raise AssertionError('Failed operations changed shared client: ' + relative)
        record = {'result': 'pass', 'failure_regression_only': True,
                  'cases': expected['cases'], 'exact_nochange_pairs': expected['unchanged_pairs'],
                  'inputs': expected['inputs'], 'strong_failure_log_evidence': True}
        (out / 'RESULT.JSON').write_text(json.dumps(record, indent=2) + '\n')
        print(json.dumps(record, indent=2))
        return
    for label in ('R01', 'R02', 'R03', 'R08', 'R09'):
        if pin((snapshots / label / 'DRIVER.BIN').read_bytes()) != expected['inputs']['sound']:
            raise AssertionError('Shared driver changed across install/repair/uninstall: ' + label)
    for label in ('R01', 'R02', 'R03', 'R08'):
        if (snapshots / label / 'BASE.TXT').read_bytes() != b'version one\r\n':
            raise AssertionError('Current install/repair/recovery payload mismatch: ' + label)
    for name in ('SHADOW.OUT', 'STAGE.OUT', 'COLLIDE.OUT'):
        if (croot / 'FIX' / name).read_bytes() != (croot / 'FIX/FOREIGN.TXT').read_bytes():
            raise AssertionError('Foreign file changed: ' + name)
    if '00' in expected['cases']:
        if (croot / 'FIX/CLIENT0.OUT').read_bytes() != (croot / 'FIX/FOREIGN.TXT').read_bytes():
            raise AssertionError('Unrecognized existing client file was changed')
    if '00D' in expected['cases']:
        if (croot / 'FIX/INPUT0.OUT').read_bytes() != (croot / 'FIX/FOREIGN.TXT').read_bytes():
            raise AssertionError('Unrecognized TSHELL input dependency was changed')
    if '00M' in expected['cases']:
        if (croot / 'FIX/MIRROR0.OUT').read_bytes() != (croot / 'FIX/FOREIGN.TXT').read_bytes():
            raise AssertionError('Unrecognized Windows XT sound mirror was changed')
    if '0F' in expected['cases'] and (croot / 'BADWAMP').exists():
        raise AssertionError('Failed install left its new app/private directories behind')
    for relative, identity in expected['shared_clients'].items():
        if pin((croot / relative).read_bytes()) != identity:
            raise AssertionError('Shared client was not installed/repaired/retained: ' + relative)
    for relative in expected['absent_optional_clients']:
        if (croot / relative).exists():
            raise AssertionError('Absent optional client was installed: ' + relative)
    final = snapshot(croot / 'WAMP')
    if final != expected['final_files']:
        raise AssertionError('Removal did not leave exactly the changed/unrelated files: ' + str(final))
    if any(file.is_dir() for file in (croot / 'WAMP').iterdir()):
        raise AssertionError('Private installation directories remain after normal removal')
    if (croot / 'WINDOWS/SYSTEM.INI').read_bytes() != ((snapshots / 'R01/INI.TXT').read_bytes() + expected['appended_ini'].encode()):
        raise AssertionError('App uninstall altered shared sound/unrelated settings')
    if pin((croot / 'WINDOWS/SYSTEM/SOUND.DRV').read_bytes()) != expected['inputs']['sound']:
        raise AssertionError('App uninstall changed the shared canonical driver')
    if (croot / 'WINDOWS/SOUND.DRV').exists():
        raise AssertionError('Unexpected root driver remains')
    record = {'result': 'pass', 'native_cases': len(expected['cases']),
              'cases': expected['cases'], 'exact_nochange_pairs': expected['unchanged_pairs'],
              'same_installed_driver': expected['inputs']['sound'],
              'production_setup': expected['inputs']['setup'],
              'test_only_failure_injector': expected['inputs']['setup_test'],
              'shared_driver_and_unrelated_ini_retained_after_app_uninstall': True,
              'shared_clients': expected['shared_clients'],
              'absent_optional_clients': expected['absent_optional_clients'],
              'changed_and_foreign_files_preserved': True,
              'emulator_launched_by_this_script': False}
    (out / 'RESULT.JSON').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='mode', required=True)
    prep = sub.add_parser('prepare')
    prep.add_argument('--package', type=Path, required=True)
    prep.add_argument('--windows', type=Path, required=True)
    prep.add_argument('--setup', type=Path, required=True)
    prep.add_argument('--setup-test', type=Path, required=True)
    prep.add_argument('--driver', type=Path)
    prep.add_argument('--launcher', type=Path)
    prep.add_argument('--rawclk', type=Path)
    prep.add_argument('--clients', type=Path)
    prep.add_argument('--xt-before', type=Path)
    prep.add_argument('--failures-only', action='store_true')
    prep.add_argument('--out', type=Path, required=True)
    sub.add_parser('verify').add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    (prepare if args.mode == 'prepare' else verify)(args)


if __name__ == '__main__':
    main()
