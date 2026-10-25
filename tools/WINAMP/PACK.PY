"""Package native Setup and one canonical SOUND driver from qualified builds."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct
import zipfile
import zlib

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
BASE_SHA = '055f872e4af09d5f2a8f8bc961dfadbcf274d56380e827006d5e3d91a3d3b061'
RUNTIME = ('CHIME.SND', 'LWFILE.DLL',
           'PLUCK.TND', 'RESERVE.COM', 'SHORT.VGM',
           'STREAM.MID', 'TBRIDGE.EXE')
# These are the current three-DLL native link inputs, not SDK/toolchain files.
CLOCK_SOURCES = {
    'raw': set(('BLDMIDI.BAT CLOCK.ASM PITLINK.H PSGOWN.H TCBURST.H '
                'TCXBASS.H TIMECLK.C TIMECLK.DEF TIMECLK.H TIMECLK.RSP '
                'WINHOOK.ASM').split()),
    'midi': set(('BLDMIDI.BAT ENTRY.ASM INSTR.C INSTR.H LTMATH.H LTPBASE.H '
                 'MDAPI.H MIDICLK.C MIDICLK.DEF MIDICLK.RSP MIDIMSG.H '
                 'MIDISYN.C MIDISYN.H PERIODS.H PITLINK.H PITNOTES.H '
                 'TCBURST.H TCXBASS.H TIMECLK.H').split()),
    'mci': set(('BLDMIDI.BAT ENTRY.ASM LTPBASE.H MCIAPI.H MCIPSG.C MCIPSG.H '
                'MCITNDY.C MCITNDY.DEF MCITNDY.RSP MDAPI.H MIDIMSG.H '
                'MIDISKIP.H SMF.C SMF.H').split()),
}
CLOCK_FILES = {'raw': 'RAWCLK.DLL', 'midi': 'MIDICLK.DLL', 'mci': 'MCITNDY.DLL'}


MIDI_BASE = json.loads((HERE/'MIDIBASE.JSN').read_bytes())
PLAYER_SOURCES = set(('BLDPLAY.BAT CDLG.H FMTAPP.C FMTAPP.H FMTCORE.C FMTCORE.H '
    'GUNZIP.C GUNZIP.H LTWUI.C LTWUI.H LWAUDIO.H MCIAPI.H MINIPLAY.C '
    'MINIPLAY.DEF MINIPLAY.RSP MODPLAY.C MODPLAY.H PLLIST.C PLLIST.H '
    'RAWAPP.C RAWAPP.H RAWFILE.C RAWFILE.H TCBURST.H TIMECLK.H '
    'VGM.C VGM.H VGMGRP.C VGMGRP.H').split())


def player_sources(record, sources):
    names = set(record['files'])
    require(record.get('flags') == '/G0 /Gw /Gs /AS /W3 /Od',
            'Player compile flags differ from the native 8086/small-model build')
    require(PLAYER_SOURCES <= names, 'Missing compiled player source/header')
    require(re.fullmatch('[0-9a-f]{40}', record.get('commit','')),
            'Missing committed player source identity')
    for name, wanted in record['files'].items():
        require(re.fullmatch(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}', name),
                'Unsafe player source name: '+name)
        data = sources[name]
        require(identity(data) == wanted, 'Player source identity differs: '+name)
        for header in re.findall(rb'^\s*#\s*include\s*"([^"\r\n]+)"',data,re.M):
            require(header.decode('ascii').upper() in names,
                    'Unpinned player include: '+header.decode('ascii'))
    for obj in re.findall(rb'\b([A-Z0-9_]+)\.OBJ\b',sources['MINIPLAY.RSP'].upper()):
        require(obj.decode()+'.C' in names or obj.decode()+'.ASM' in names,
                'Unpinned player link unit: '+obj.decode())


def player_receipt(files):
    source = json.loads(files['EVIDENCE/PLAYERSR.JSN'])
    result = json.loads(files['EVIDENCE/PLAYER.JSN'])
    process_bytes = files['EVIDENCE/PLAYERPR.JSN']
    process = json.loads(process_bytes)
    require(result.get('status') == 'COMPILED_RUNTIME_ACCEPTANCE_PENDING',
            'Player native build is unfinished')
    require(result.get('source') == source and result.get('process') == process,
            'Player build/source/process receipts differ')
    require(process.get('return') == 0 and process.get('owned_child_stopped') is True and
            process.get('lease_released') is True and not process.get('watchdog') and
            'error' not in process, 'Player native process did not finish normally')
    log = files['EVIDENCE/PLAYER.LOG']
    require(b'PASS NATIVE PLAYER BUILD' in log and
            not re.search(rb'^\s*FAIL\b',log,re.M), 'Player native build has not passed')
    binary = files['MINIPLAY.EXE']
    require(len(binary)>=64 and binary[:2]==b'MZ', 'Player is not a native executable')
    at = struct.unpack_from('<I',binary,60)[0]
    require(at+64<=len(binary) and binary[at:at+2]==b'NE' and
            struct.unpack_from('<H',binary,at+62)[0]==0x300,
            'Player native NE/version differs')
    runtime = identity(binary)
    require(result['runtime'] == runtime and result['NE']['identity'] == runtime,
            'Player native binary differs')
    ne = result['NE']
    require(ne['expected_windows_version'] == 0x300 and ne['CODE_minimum_bytes'] <= 65536 and
            ne['DGROUP_minimum_bytes']+ne['heap_bytes']+ne['stack_bytes'] <= 65536,
            'Player is outside native Windows3.0/small-model limits')
    require(identity(files['EVIDENCE/PLAYER.MAP']) == result['map'], 'Player native MAP differs')
    sources = {n:files['SOURCE/PLAYER/'+n] for n in source['files']}
    player_sources(source,sources)
    return {'sources':source['files'], 'source_commit':source['commit'],
            'flags':source['flags'], 'runtime':runtime,
            'source_receipt':identity(files['EVIDENCE/PLAYERSR.JSN']),
            'result_receipt':identity(files['EVIDENCE/PLAYER.JSN']),
            'process_receipt':identity(process_bytes), 'map':result['map']}


def native_player(directory, source):
    source_record = json.loads((directory/'SOURCES.JSN').read_bytes())
    for name, wanted in source_record['files'].items():
        require(identity((source/name).read_bytes()) == wanted,
                'Current player source differs: '+name)
        require(identity((directory/name).read_bytes()) == wanted,
                'Staged player source differs: '+name)
    files = {'MINIPLAY.EXE':(directory/'MINIPLAY.EXE').read_bytes()}
    for output,original in (('PLAYERSR.JSN','SOURCES.JSN'),('PLAYER.JSN','RESULT.JSN'),
                            ('PLAYERPR.JSN','PROCESS.JSON'),('PLAYER.LOG','BUILD.LOG'),
                            ('PLAYER.MAP','MINIPLAY.MAP')):
        files['EVIDENCE/'+output] = (directory/original).read_bytes()
    for name in source_record['files']:
        files['SOURCE/PLAYER/'+name] = (directory/name).read_bytes()
    return files,player_receipt(files)


def frozen_runtime(files):
    for name,wanted in MIDI_BASE['runtime'].items():
        require(identity(files[name]) == wanted, 'Fixed current runtime changed: '+name)


def current_player_copy(path):
    parts = path.upper().split('/')
    if any(p in ('EVIDENCE','FROZEN','HISTORICAL','HISTORY','TESTS') for p in parts):
        return False
    return (path.startswith(('DEPS/','SRC/SOURCE/','SOURCE/PLAYER/')) or
            (len(parts)==2 and parts[0]=='SRC') or
            path.startswith(('SOURCE/RAW/','SOURCE/MIDI/','SOURCE/MCI/')))


def install_player_sources(files, record, records):
    sources = {n:files['SOURCE/PLAYER/'+n] for n in record['sources']}
    for path in list(files):
        name = path.rsplit('/',1)[-1]
        if name not in sources or not current_player_copy(path):
            continue
        parts = path.split('/')
        if len(parts)==3 and parts[0]=='SOURCE' and parts[1].lower() in CLOCK_SOURCES:
            core = records[parts[1].lower()]['sources']
            if name in core:
                require(files[path] == sources[name], 'Player changed shared compiled header: '+name)
                continue
        del files[path]
    for name,data in sources.items():
        for prefix in ('SOURCE/PLAYER/','SRC/SOURCE/','DEPS/'):
            files[prefix+name] = data



def clock_sources(label, record, sources):
    names = set(record['sources'])
    require(CLOCK_SOURCES[label] <= names,
            'Missing pinned '+label+' source: '+', '.join(sorted(CLOCK_SOURCES[label]-names)))
    for name, pin in record['sources'].items():
        require(re.fullmatch(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}', name),
                'Unsafe native source name: '+name)
        data = sources[name]
        require(identity(data) == pin, 'Unlinked '+label+' source: '+name)
        for header in re.findall(rb'^\s*#\s*include\s*"([^"\r\n]+)"', data, re.M):
            require(header.decode('ascii').upper() in names,
                    'Unpinned '+label+' include: '+header.decode('ascii'))


def clock_process(record, process_bytes, log):
    process = json.loads(process_bytes)
    require(process.get('return') == 0 and process.get('owned_child_stopped') is True and
            process.get('lease_released') is True and 'error' not in process and
            not process.get('watchdog'), 'Unfinished three-DLL native build')
    require(record.get('common_process_identity') == identity(process_bytes),
            'Unlinked three-DLL native process')
    require(b'PASS MIDI THREE DLL BUILD' in log and
            not re.search(rb'^\s*FAIL\b', log, re.M),
            'Three-DLL native build has not passed')


def clock_receipts(files, records):
    process = files['EVIDENCE/RAWPROC.JSN']
    batch = files['SOURCE/RAW/BLDMIDI.BAT']
    for label in CLOCK_SOURCES:
        require(files['EVIDENCE/'+label.upper()+'PROC.JSN'] == process,
                'Clock DLLs came from different native processes')
        require(files['SOURCE/'+label.upper()+'/BLDMIDI.BAT'] == batch,
                'Clock DLLs came from different native batches')
        record = records[label]
        source = {n: files['SOURCE/'+label.upper()+'/'+n] for n in record['sources']}
        clock_sources(label, record, source)
        clock_process(record, process, files['EVIDENCE/'+label.upper()+'.LOG'])


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def native(directory, source, filename, remap=None):
    record = json.loads((directory / 'BUILD.JSON').read_bytes())
    for name, pin in record['sources'].items():
        path = (remap or {}).get(name, source / name)
        require(identity(path.read_bytes()) == pin, 'Stale native source: ' + str(path))
        require(identity((directory / name).read_bytes()) == pin,
                'Staged native source changed: ' + name)
    binary = (directory / filename).read_bytes()
    require(identity(binary) == record['runtime'], 'Native binary changed: ' + filename)
    process = json.loads((directory / 'PROCESS.JSON').read_bytes())
    require(process.get('return') == 0 and process.get('owned_child_stopped') and
            process.get('lease_released') and 'error' not in process and
            not process.get('watchdog'),
            'Native build did not stop/release normally: ' + str(directory))
    return binary, record


def build(a):
    require(not a.out.exists(), 'Output directory must be new')
    base = a.baseline.read_bytes()
    require(identity(base)['sha256'] == BASE_SHA, 'Expected the published XT1 baseline')
    with zipfile.ZipFile(a.baseline) as z:
        previous = {n[6:]: z.read(n) for n in z.namelist()
                    if n.startswith('TWXT1/') and not n.endswith('/')}
    files = {n: previous[n] for n in RUNTIME}
    records = {}
    specs = (
        ('driver', a.driver, REPO/'src/win30/DRIVER', 'SOUND.DRV', {}),
        ('raw', a.raw, REPO/'experimental/WINLIB30/SOURCE', 'RAWCLK.DLL', {}),
        ('midi', a.midi, REPO/'experimental/WINLIB30/SOURCE', 'MIDICLK.DLL', {}),
        ('mci', a.mci, REPO/'experimental/WINLIB30/SOURCE', 'MCITNDY.DLL', {}),
        ('launch', a.launch, REPO/'src/win30/LAUNCH', 'TPLAY.EXE', {}),
        ('setup', a.setup, REPO/'src/win30/SETUP', 'SETUP.EXE', {
            'SOUNDID.H': REPO/'src/win30/DRIVER/SOUNDID.H',
            'TPLAYPOL.H': REPO/'src/win30/LAUNCH/TPLAYPOL.H'}),
    )
    for label, directory, source, name, remap in specs:
        files[name], records[label] = native(directory, source, name, remap)
        for member in records[label]['sources']:
            files['SOURCE/'+label.upper()+'/'+member] = (directory/member).read_bytes()
        for extra in source.rglob('*'):
            if extra.is_file() and '__pycache__' not in extra.parts and extra.suffix.upper() in (
                    '.C','.H','.ASM','.DEF','.RSP','.BAT','.PY','.MD','.TXT','.JSON'):
                rel=extra.relative_to(source).as_posix()
                files.setdefault('SOURCE/'+label.upper()+'/'+rel,extra.read_bytes())
        files['EVIDENCE/'+label.upper()+'.JSN'] = (directory/'BUILD.JSON').read_bytes()
        log = directory/'BUILD.LOG'
        if label == 'raw' and not log.exists():
            log = directory/'RAWBUILD.LOG'
        files['EVIDENCE/'+label.upper()+'.LOG'] = log.read_bytes()
        if label in CLOCK_SOURCES:
            files['EVIDENCE/'+label.upper()+'PROC.JSN'] = (directory/'PROCESS.JSON').read_bytes()
    clock_receipts(files, records)
    player_files,records['player'] = native_player(a.player, REPO/'src/win30/PLAYER')
    files.update(player_files)
    clients = json.loads((a.clients/'PINS.JSN').read_bytes())
    require(len(clients) == 7, 'Expected the current Windows XT clients and paired input dependency')
    for name,record in clients.items():
        data = (a.clients/name).read_bytes()
        require(identity(data) == record['runtime'], 'Changed native client: '+name)
        require('%08x'%zlib.crc32(data) == record['crc32'], 'Changed native client CRC')
        for source,pin in record['sources'].items():
            require(identity((a.workspace/source).read_bytes()) == pin,
                    'Current client source changed: '+source)
        files[name] = data
    for source in (a.clients/'SOURCE').rglob('*'):
        if source.is_file():
            files['SOURCE/CLIENTS/'+source.relative_to(a.clients/'SOURCE').as_posix()] = source.read_bytes()
    files['EVIDENCE/CLIENTS.JSN'] = (a.clients/'PINS.JSN').read_bytes()
    sound = json.loads((REPO/'src/win30/DRIVER/SOUNDID.JSON').read_bytes())
    require(identity(files['SOUND.DRV']) == {k: sound[k] for k in ('bytes','sha256')},
            'Canonical generated sound pin differs')
    require('%08x' % zlib.crc32(files['SOUND.DRV']) == sound['crc32'], 'Sound CRC differs')
    for label in ('driver','launch','setup'):
        data = files.get('SOURCE/'+label.upper()+'/SOUNDID.H')
        if data is not None:
            require(data == (REPO/'src/win30/DRIVER/SOUNDID.H').read_bytes(),
                    'Consumers use different canonical driver expectations')
    for name in ('INSTALL.TXT','README.TXT'):
        files[name] = (HERE/name).read_bytes()
    files['TESTS.JSN'] = a.tests.read_bytes()
    tests = json.loads(files['TESTS.JSN'])
    require(tests.get('status') == 'PASS', 'Native acceptance has not passed')
    require(tests.get('release_readiness', {}).get('status') == 'READY',
            'Current published baseline reconciliation is incomplete')
    require(tests.get('sound') == identity(files['SOUND.DRV']), 'Tests cover another driver')
    for key, name in (('rawclk','RAWCLK.DLL'),('midiclk','MIDICLK.DLL'),
                      ('mcitndy','MCITNDY.DLL')):
        require(tests.get(key) == identity(files[name]), 'Tests cover another '+name)
    require(tests.get('setup') == identity(files['SETUP.EXE']), 'Tests cover another Setup')
    require(tests.get('launcher') == identity(files['TPLAY.EXE']), 'Tests cover another launcher')
    require(tests.get('player') == identity(files['MINIPLAY.EXE']), 'Tests cover another player')
    frozen_runtime(files)
    require(tests.get('clients') == {n:r['runtime'] for n,r in clients.items()},
            'Tests cover different shared client files')
    # Native Setup installs only these flat runtime files. The generated manifest
    # includes SETUP itself; neither its text nor the driver embeds the manifest.
    rows = ['TWSETUP1']
    for name in sorted(n for n in files if '/' not in n and n not in ('TESTS.JSN',)):
        require(re.fullmatch(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}', name), 'Non-DOS runtime name')
        data = files[name]
        kind = clients[name]['kind'] if name in clients else ('S' if name == 'SOUND.DRV' else 'A')
        rows.append('%s|%s|%s|%d|%08x' %
                    (kind, name, name,
                     len(data), zlib.crc32(data)))
    files['PAYLOAD.LST'] = ('\r\n'.join(rows)+'\r\n').encode('ascii')
    # Preserve notices/frozen evidence; replace current rebuilt player/clock inputs.
    rebuilt = set(records['launch']['sources'])
    clock_inputs = {}
    for label in CLOCK_SOURCES:
        for name in records[label]['sources']:
            data = files['SOURCE/'+label.upper()+'/'+name]
            require(name not in clock_inputs or clock_inputs[name] == data,
                    'Clock DLLs use different source: '+name)
            clock_inputs[name] = data
    rebuilt.update(clock_inputs)
    for name, data in previous.items():
        if name.startswith('SRC/'):
            if name in ('SRC/INSTALL.TXT','SRC/README.md'):
                continue
            files[name] = data
        elif name.startswith('DEPS/') and not name.startswith('DEPS/TSOUND/'):
            if name.rsplit('/',1)[-1] in (
                    'README.TXT','POLICY.MD','BUILD.BAT','BLDTPLAY.BAT'):
                continue
            if name.rsplit('/',1)[-1] not in rebuilt:
                files[name] = data
    for name, data in clock_inputs.items():
        files['DEPS/'+name] = data
    install_player_sources(files, records['player'], records)
    files['EVIDENCE/MIDIBASE.JSN'] = (HERE/'MIDIBASE.JSN').read_bytes()
    files['DEPS/README.TXT'] = (
        'Current MIDI/MCI and raw-clock link inputs are included here.\r\n'
        'The unchanged timer-bridge source is retained. Native source snapshots\r\n'
        'are in SOURCE/DRIVER, RAW, MIDI, MCI, PLAYER, LAUNCH and SETUP.\r\n'
        'Current player source is in SOURCE/PLAYER and SRC/SOURCE. Native receipts\r\n'
        'bind the current runtime bytes to those source snapshots.\r\n').encode('ascii')
    files['SRC/README.TXT'] = (
        'Current player source/builder is in SOURCE/PLAYER and SRC/SOURCE.\r\n'
        'Unchanged file browser source and notices remain here.\r\n'
        'Run the native installer described by the packet-root INSTALL.TXT.\r\n'
        'Current shared driver, clocks, MIDI/MCI, launcher and Setup are in SOURCE.\r\n'
        'Historical test records retain their original binary scope.\r\n').encode('ascii')
    files['SOURCE/DRIVER/LICENSE'] = (REPO/'LICENSE').read_bytes()
    files['BUILD.JSN'] = (json.dumps({'baseline_xt1': BASE_SHA,
        'source_commit': a.source_commit, 'canonical_sound': sound,
        'native_builds': records, 'clients': clients, 'runtime_base': MIDI_BASE}, indent=2)+'\n').encode()
    files['MANIFEST.JSN'] = (json.dumps({n: identity(data) for n,data in
        sorted(files.items())}, indent=2)+'\n').encode()
    a.out.mkdir(parents=True)
    with zipfile.ZipFile(a.out/'TWXT2.ZIP','w',compression=zipfile.ZIP_STORED) as z:
        for name,data in sorted(files.items()):
            require(not name.startswith('/') and '..' not in name.split('/'), 'Unsafe ZIP path')
            info = zipfile.ZipInfo('TWXT2/'+name, (1980,1,1,0,0,0))
            info.external_attr = 0o100644 << 16
            z.writestr(info,data)
            target = a.out/'TWXT2'/name
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(data)
    result = identity((a.out/'TWXT2.ZIP').read_bytes())
    # Same native Setup body, with its validated flat payload overlay appended.
    from importlib.machinery import SourceFileLoader
    from importlib.util import spec_from_loader, module_from_spec
    loader=SourceFileLoader('native_sfx',str(HERE/'SFX.PY'))
    spec=spec_from_loader(loader.name,loader);sfx=module_from_spec(spec);loader.exec_module(sfx)
    exe=sfx.make(a.out/'TWXT2/SETUP.EXE',a.out/'TWXT2')
    require(tests.get('installer') == identity(exe),
            'Tests cover another self-contained installer payload')
    (a.out/'TWSETUP.EXE').write_bytes(exe)
    (a.out/'PACKET.JSN').write_text(json.dumps(
        {**result, 'installer': identity(exe)},indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for n in ('baseline','driver','raw','midi','mci','launch','setup','player','clients','workspace','tests','out'):
        p.add_argument('--'+n,type=Path,required=True)
    p.add_argument('--source-commit',required=True)
    build(p.parse_args())
