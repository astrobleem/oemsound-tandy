"""Prepare coordinated authoring kits; preserve current shared runtime bytes."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import struct
import subprocess
import sys
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BEATS_INPUTS = ('BEATS.C', 'BEATPAT.C', 'BEATPAT.H', 'BEATEXP.C', 'BEATEXP.H',
                'MIDIAPI.H', 'BEATS.DEF', 'BEATS.RSP', 'BUILD.BAT', 'MIDIMAP.C',
                'MIDIMAP.DEF', 'MIDIMAP.RSP', 'ENTRY.ASM', 'INSTR.C', 'INSTR.H', 'PERIODS.H')
FIDDLE_INPUTS = ('MMLFIDL.C', 'MMLFIDL.DEF', 'MMLFIDL.RSP', 'MMLCORE.C', 'MMLCORE.H')
SOURCE_PATHS = {n: 'src/win30/BEATLAB/' + n for n in BEATS_INPUTS}
SOURCE_PATHS.update({n: 'src/win30/FIDDLE/' + n for n in FIDDLE_INPUTS})
PRODUCTION = tuple(SOURCE_PATHS)
RUNTIME_PATHS = {'BEATS.EXE': ('BEATS/BEATS.EXE',),
                 'MIDIMAP.DRV': ('BEATS/MIDIMAP.DRV', 'FIDDLE/MIDIMAP.DRV'),
                 'MMLFIDL.EXE': ('FIDDLE/MMLFIDL.EXE',)}
REPLACED = {path for paths in RUNTIME_PATHS.values() for path in paths} | {
    'README.TXT', 'INSTALL.TXT', 'BUILD.JSN', 'MANIFEST.JSN'}
KITS = {
    'MML3AUTH': {'runtimes': RUNTIME_PATHS, 'demo_prefix': 'BEATS/', 'guide': 'BEATS/README.TXT',
                 'siblings': ()},
    'PITEXPR': {'runtimes': {'BEATS.EXE': ('BEATS.EXE',), 'MIDIMAP.DRV': ('MIDIMAP.DRV',)},
                'demo_prefix': '', 'guide': 'BEATS.TXT',
                'siblings': ('MINIPIAN.EXE', 'JOYMIDI.EXE', 'MINIMIDI.EXE')},
}
FROZEN_ALIASES = tuple('SOURCE/src/win30/' + n for n in (
    'BEATMML/MIDIMAP.C', 'BEATMML/INSTR.C', 'BEATMML/INSTR.H', 'BEATMML/MIDIAPI.H',
    'BEATMML/PERIODS.H', 'MMLFIDL/MMLCORE.C', 'MMLFIDL/MMLCORE.H'))

STEMS = ('STARTER', 'POCKET', 'LOWRIDER', 'FIVE', 'NIGHT')
WINDOWS_GCC = Path('C:/Users/chad/Documents/Codex/toolchains/gcc-ucrt64.py')
SOURCE_PREFIX = 'SOURCE/src/win30/BEATLAB/'
REGRESSION_INPUTS = {'SOURCE/src/win30/DRIVER/PITCORE.H': 'src/win30/DRIVER/PITCORE.H'}
GUIDE = b'''Tandy Beats Lab and Fiddle authoring kit\r
DWA: DAWs With Attitude\r
Straight Outta LLMompton. Produced by Dr. DRAM.\r
\r
Extract the complete MML3AUTH folder, keeping BEATS and FIDDLE intact.\r
Exit Windows to DOS and run the included TWSETUP.EXE once. It installs\r
the shared SOUND driver and Winamp automatically. Start Windows through\r
your normal display launcher. In Program Manager, File / Run:\r
  C:\\MML3AUTH\\BEATS\\BEATS.EXE\r
  C:\\MML3AUTH\\FIDDLE\\MMLFIDL.EXE\r
\r
Keep the supplied MIDIMAP.DRV files beside their applications. They are\r
app-local MIDI support, not alternative system SOUND installations. The\r
same installed SOUND driver serves these tools and Winamp. No driver\r
swapping, file gathering, system MIDI registration or INI editing is needed.\r
\r
Beats has three tone rows, noise percussion and a PC-speaker row. Demo\r
cycles five editable examples. The speaker demo loads PIT on; pattern\r
Save/Load preserves that setting. See BEATS\\README.TXT for controls.\r
Stop other sound or speech producers before playing.\r
\r
TWSETUP.EXE already includes its installation payload. TWXT2.ZIP is the\r
matching shared runtime/source companion, not a preparation step. Shared\r
driver, installer and Winamp bytes are preserved. Beats, Fiddle and their\r
paired app-local mapper are updated together; current build and demo\r
identities are in BUILD.JSN. Older evidence retains its original scope.\r
'''
BEATS_GUIDE = b'''Tandy Beats Lab - DAWs With Attitude
Straight Outta LLMompton. Produced by Dr. DRAM.

Click Demo to cycle Starter, Pocket drums, Lowrider lead, Speaker bass
and Late-night keys. These original G-funk-style educational patterns
contain no copied recordings, lyrics or recognizable song melodies.

Click a grid cell to toggle note/rest. Arrows select; Space/Enter toggles;
Pitch-/Pitch+ or +/- changes pitch; Delete rests.

Click tone labels for shipped presets.
PCspkr is row 5. Its visible PIT switch controls playback and export;
the Speaker bass demo enables it automatically. No extra activation step.
Tempo +/- or Page Down/Up changes BPM. Play starts at step 1. Stop/Escape
releases sound; P/S works with grid focus, and F5 toggles playback.
Tab/Shift+Tab navigates controls. About/F1 shows key controls.

Enter NAME.BTS beside Load/Save. Save preserves all 40 cells, tempo,
presets and PIT state; Load reopens them. Ctrl+S saves, Ctrl+O loads.
Replacement asks first and keeps the previous file as .BAK. Bad input
leaves the current edit intact. Ready editable examples are:
  STARTER.BTS  POCKET.BTS  LOWRIDER.BTS  FIVE.BTS  NIGHT.BTS

MML shows the generated score. Copy copies it; Export writes NAME.MML
beside the app without replacing an existing .MML. Same-named demo .MML
files are supplied for Fiddle. The grid loads .BTS, not arbitrary MML.

Timing is straight eighths with 75 percent gate: no swing, glide, ties
or per-cell velocity. PSG tones remain A2-C7 (MIDI 45-96). The updated
app-local mapper and Fiddle [P] part accept PC speaker D1-C7 (26-96);
Speaker bass uses A1/C2/D2 (33/36/38), about 55/65/73 Hz, without folding.
The speaker has fixed level and no PSG preset envelope.

Run the complete kit's TWSETUP.EXE from DOS for shared sound setup. Keep
paired MIDIMAP.DRV beside Beats; do not install or swap it system-wide.
Stop one sound producer before starting another.
'''.replace(b'\n', b'\r\n')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def pin(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode('utf-8')


def crlf(data):
    return data.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')


def release_record():
    return json.loads((ROOT / 'tools/WINAMP/RELEASE.JSN').read_bytes())


def kit_layout(kit):
    require(kit in KITS, 'Unsupported kit: ' + str(kit))
    return KITS[kit]


def replacement_paths(kit):
    return {p for paths in kit_layout(kit)['runtimes'].values() for p in paths} | {
        'README.TXT', 'INSTALL.TXT', 'BUILD.JSN', 'MANIFEST.JSN'}


def consumer_guide(kit):
    if kit == 'MML3AUTH':
        return GUIDE
    kit_layout(kit)
    text = """Tandy Beats Lab and Tandy instruments
DWA: DAWs With Attitude
Straight Outta LLMompton. Produced by Dr. DRAM.

Extract the complete PITEXPR folder with all supplied files intact.
Exit Windows to DOS and run its included TWSETUP.EXE once. It installs
the shared SOUND driver and Winamp automatically. Start Windows through
your normal display launcher. In Program Manager, File / Run:
  C:\\PITEXPR\\BEATS.EXE
  C:\\PITEXPR\\MINIPIAN.EXE
  C:\\PITEXPR\\JOYMIDI.EXE
  C:\\PITEXPR\\MINIMIDI.EXE

This is the same current Tandy Beats Lab supplied with Fiddle in the
MML3AUTH authoring kit: five editable lanes, named demos, pattern Save/Load
and MML export. See BEATS.TXT for controls. Demo loads the required speaker
setting automatically; Save/Load preserves it. The same .BTS and .MML
examples are supplied in both kits. Fiddle itself is in MML3AUTH.

Keep the supplied MIDIMAP.DRV beside these applications. It is app-local
MIDI support, not a second system SOUND installation. All use the same
installed SOUND driver. No driver swapping, file gathering, system MIDI
registration or INI editing is needed. Stop one sound producer before
starting another. Mini Piano, JoyMIDI and MiniMIDI retain their own controls
and original application bytes.

TWSETUP.EXE already includes its payload. TWXT2.ZIP is the matching shared
runtime/source companion, not a preparation step. Shared driver, installer
and Winamp bytes are preserved. Updated client/source/demo identities are
in BUILD.JSN; older evidence retains its original scope.
"""
    return crlf(text.encode('ascii'))


def read_kit(path, kit='MML3AUTH'):
    layout = kit_layout(kit)
    raw = Path(path).read_bytes()
    release = release_record()
    allowed = [release['assets'][kit + '.ZIP']]
    if kit + '.ZIP' in release.get('beatlab_baselines', {}):
        allowed.append(release['beatlab_baselines'][kit + '.ZIP'])
    require(pin(raw) in allowed, 'Base kit is not a current or explicit bank baseline for ' + kit)
    require(release['package'] == release['assets']['TWXT2.ZIP'] and
            release['installer'] == release['assets']['TWSETUP.EXE'], 'Current shared release pins are inconsistent')
    files, seen = {}, set()
    with zipfile.ZipFile(Path(path)) as archive:
        require(sum(i.file_size for i in archive.infolist()) <= 64 * 1024 * 1024, 'Base kit too large')
        for item in archive.infolist():
            name = item.filename
            parts = PurePosixPath(name).parts
            require(name.startswith(kit + '/') and '\\' not in name and ':' not in name and
                    all(p not in ('', '.', '..') for p in parts) and name == '/'.join(parts), 'Unsafe kit member')
            require(name.upper() not in seen and not stat.S_ISLNK(item.external_attr >> 16), 'Duplicate or linked kit member')
            seen.add(name.upper())
            require(not item.is_dir(), 'Unexpected directory-only member')
            files[name[len(kit) + 1:]] = archive.read(item)
    manifest = json.loads(files['MANIFEST.JSN'])
    require(set(manifest) == set(files) - {'MANIFEST.JSN'}, 'Base kit inventory differs')
    for name, wanted in manifest.items():
        require(pin(files[name]) == wanted, 'Base member differs: ' + name)
    required = {'TWSETUP.EXE', 'TWXT2.ZIP'} | set(layout['siblings']) | {
        member for members in layout['runtimes'].values() for member in members}
    for name in required:
        require(name in files, 'Required base input missing: ' + name)
    for name in ('TWSETUP.EXE', 'TWXT2.ZIP'):
        require(pin(files[name]) == release['assets'][name], 'Base kit has stale/current shared payload mixing: ' + name)
    return raw, files


def frozen_regression_sources(kit, base_raw, base, regression=None):
    if kit == 'MML3AUTH':
        frozen_raw, frozen = base_raw, base
    else:
        frozen_raw, frozen = regression if regression is not None else read_kit(ROOT / 'artifacts/win30/MML3AUTH.ZIP', 'MML3AUTH')
    require(all(n in frozen for n in FROZEN_ALIASES), 'Frozen authoring regression source missing')
    return {n: frozen[n] for n in FROZEN_ALIASES}, pin(frozen_raw)


def native_inputs(build, here=HERE):
    build, here = Path(build), Path(here)
    repo = here.parents[2]
    record_raw = (build / 'BUILD.JSON').read_bytes()
    record = json.loads(record_raw)
    require('BTSTART.EXE' not in record.get('runtimes',{}), 'Effects integration builds require FXPACK.PY')
    require(record.get('target') == '8086 Windows 3.0 real mode; MSC6/LINK4', 'Unexpected native target')
    require(record.get('status') == 'native compile/link only; execution separate', 'Unexpected native build scope')
    require(record.get('source_paths') == SOURCE_PATHS and
            set(record.get('sources', {})) == set(PRODUCTION) and
            set(record.get('source_originals', {})) == set(PRODUCTION), 'Incomplete or incorrect compiled source inventory')
    sources = {}
    for name, relative in SOURCE_PATHS.items():
        original, staged = (repo / relative).read_bytes(), (build / name).read_bytes()
        require(pin(original) == record['source_originals'][name], 'Current source differs from native receipt: ' + name)
        require(pin(staged) == record['sources'][name] and staged == crlf(original), 'Compiled source differs: ' + name)
        sources['SOURCE/' + relative] = staged
    fiddle_header = (repo / 'src/win30/FIDDLE/MIDIAPI.H').read_bytes()
    require(fiddle_header == (here / 'MIDIAPI.H').read_bytes() and
            crlf(fiddle_header) == (build / 'MIDIAPI.H').read_bytes(), 'Fiddle and Beats MIDI headers differ')
    sources['SOURCE/src/win30/FIDDLE/MIDIAPI.H'] = (build / 'MIDIAPI.H').read_bytes()
    require(set(record.get('runtimes', {})) == set(RUNTIME_PATHS) and
            record.get('runtime') == record['runtimes']['BEATS.EXE'], 'Incomplete or inconsistent native runtime inventory')
    runtimes = {}
    for name in RUNTIME_PATHS:
        exe = (build / name).read_bytes()
        require(pin(exe) == record['runtimes'][name], 'Native runtime differs from build receipt: ' + name)
        require(len(exe) >= 64 and exe[:2] == b'MZ', 'Not a native executable: ' + name)
        ne = struct.unpack_from('<I', exe, 0x3c)[0]
        require(ne + 64 <= len(exe) and exe[ne:ne+2] == b'NE' and
                struct.unpack_from('<H', exe, ne+0x3e)[0] == 0x0300, 'Expected Windows 3.0 NE executable: ' + name)
        runtimes[name] = exe
    process = json.loads((build / 'PROCESS.JSON').read_bytes())
    require(all(process.get(k) is True for k in ('lease_acquired', 'lease_released', 'owned_child_stopped')) and
            process.get('return') == 0 and not process.get('error') and not process.get('watchdog'),
            'Native build did not complete under the shared lease')
    return runtimes, sources, record_raw


def make_demos(work, here=HERE):
    work, here = Path(work), Path(here)
    work.mkdir(parents=True, exist_ok=False)
    helper = work / 'DEMOS.C'
    helper.write_text('''#include <stdio.h>
#include <string.h>
#include "BEATPAT.H"
#if BP_DEMOS < 5
#error Update_the_packet_demo_inventory
#endif
int main(void){
 static const char *names[5]={"STARTER","POCKET","LOWRIDER","FIVE","NIGHT"};
 static BPAT p,loaded;static char text[BP_MML_CAP];char path[20];FILE *f;unsigned i,n;
 for(i=0;i<5;++i){
  BPDemo(&p,i);if(!BPValid(&p))return 1;
  sprintf(path,"%s.BTS",names[i]);n=BPEncode(&p,text,sizeof(text));
  if(!n||BPDecode(&loaded,text,n)||loaded.tempo!=p.tempo||loaded.pit!=p.pit||
     memcmp(loaded.program,p.program,sizeof(p.program))||memcmp(loaded.note,p.note,sizeof(p.note)))return 6;
  f=fopen(path,"wb");if(!f||!n)return 2;
  if(fwrite(text,1,n,f)!=n||fclose(f))return 3;
  sprintf(path,"%s.MML",names[i]);n=BPMML(&p,text,sizeof(text));
  f=fopen(path,"wb");if(!f||!n)return 4;
  if(fwrite(text,1,n,f)!=n||fclose(f))return 5;
 }
 return 0;
}
''', encoding='ascii')
    if os.name == 'nt':
        require(WINDOWS_GCC.is_file(), 'Established Windows GCC launcher missing')
        compiler = [sys.executable, str(WINDOWS_GCC), '--']
    else:
        compiler = [shutil.which('cc') or 'gcc']
    exe = work / ('DEMOS.exe' if os.name == 'nt' else 'DEMOS')
    command = compiler + ['-x', 'c', '-std=c89', '-pedantic-errors', '-Wall', '-Wextra', '-Werror',
                          '-Wno-misleading-indentation', '-I', str(here), '-I', str(ROOT/'src/win30/EFFECTS'), str(helper),
                          str(here / 'BEATPAT.C'), str(here / 'BEATEXP.C'), str(ROOT/'src/win30/EFFECTS/FXCORE.C'), '-o', str(exe)]
    env = dict(os.environ)
    if os.name == 'nt':
        env['PATH'] = 'C:/msys64/ucrt64/bin' + os.pathsep + env.get('PATH', '')
    subprocess.run(command, cwd=work, env=env, check=True, timeout=35)
    subprocess.run([str(exe)], cwd=work, env=env, check=True, timeout=10)
    return {'BEATS/' + stem + ext: (work / (stem + ext)).read_bytes()
            for stem in STEMS for ext in ('.BTS', '.MML')}


def assemble(base_raw, base, runtimes, sources, build_raw, demos, here=HERE, kit='MML3AUTH', regression=None):
    layout = kit_layout(kit)
    require(set(runtimes) == set(RUNTIME_PATHS), 'Runtime inventory differs')
    require(set(demos) == {'BEATS/' + stem + ext for stem in STEMS for ext in ('.BTS', '.MML')}, 'Demo inventory differs')
    require(all(data for data in demos.values()), 'Empty demo')
    require('EVIDENCE/BEFORE.JSN' not in base, 'Already updated kit is not a baseline')
    files = dict(base)
    frozen, frozen_pin = frozen_regression_sources(kit, base_raw, base, regression)
    files.update(frozen)
    files.update(sources)
    for name in ('FXCORE.C','FXCORE.H'):
        files['SOURCE/src/win30/EFFECTS/'+name] = (ROOT/'src/win30/EFFECTS'/name).read_bytes()
    for name in ('BUILD.PY', 'PACK.PY', 'PACKTEST.PY', 'HOSTTEST.PY', 'README.MD', 'LOWPIT.MD', 'VALIDATE.MD'):
        path = Path(here) / name
        if path.is_file():
            files[SOURCE_PREFIX + name] = path.read_bytes()
    for directory in (Path(here) / 'TESTS', Path(here).parent / 'FIDDLE'):
        for path in sorted(directory.rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts and path.suffix.lower() in ('.c', '.h', '.py', '.md', '.txt', '.bts', '.mml', '.asm', '.def', '.rsp', '.bat'):
                relative = 'SOURCE/' + path.relative_to(Path(here).parents[2]).as_posix()
                if relative not in sources:
                    files[relative] = path.read_bytes()
    files['SOURCE/src/win30/LAUNCH/RUNDOS.PY'] = (ROOT / 'src/win30/LAUNCH/RUNDOS.PY').read_bytes()
    for member, relative in REGRESSION_INPUTS.items():
        files[member] = (ROOT / relative).read_bytes()
    actual_demos = {layout['demo_prefix'] + name[len('BEATS/'):]: data for name, data in demos.items()}
    files.update(actual_demos)
    for name, paths in layout['runtimes'].items():
        for path in paths:
            files[path] = runtimes[name]
    guide = consumer_guide(kit)
    files.update({layout['guide']: BEATS_GUIDE, 'README.TXT': guide, 'INSTALL.TXT': guide,
                  'EVIDENCE/BEFORE.JSN': base['BUILD.JSN'], 'EVIDENCE/DWABUILD.JSN': build_raw})
    preserved = {name: pin(data) for name, data in base.items() if name not in replacement_paths(kit)}
    for name in preserved:
        require(files[name] == base[name], 'Unexpected baseline change: ' + name)
    files['BUILD.JSN'] = json_bytes({
        'schema': 2, 'kit': kit, 'operation': 'Tandy Beats Lab, low-PIT mapper and matching Fiddle update; shared runtime unchanged',
        'baseline': pin(base_raw), 'baseline_record': 'EVIDENCE/BEFORE.JSN',
        'native_build': pin(build_raw), 'runtimes': {name: pin(data) for name, data in runtimes.items()},
        'regression_inputs': {name: pin(files[name]) for name in REGRESSION_INPUTS},
        'frozen_regression_baseline': frozen_pin,
        'frozen_regression_inputs': {name: pin(data) for name, data in frozen.items()},
        'preserved': preserved,
        'changes': {name: pin(data) for name, data in files.items()
                    if name not in ('BUILD.JSN', 'MANIFEST.JSN') and
                    (name not in base or data != base[name] or name in replacement_paths(kit))},
        'demos': {name: pin(data) for name, data in actual_demos.items()},
        'qualification': {'native_build': 'Compile/link receipt only; execution acceptance is separate',
                          'shared': 'Exact base installer, shared driver and Winamp companion preserved',
                          'paired_mapper': 'Both kit layouts use the same receipt-pinned app-local mapper',
                          'historical_unchanged_client_gate': False,
                          'publication': 'Artifact creation only; no public write performed by packager'}})
    files['MANIFEST.JSN'] = json_bytes({name: pin(data) for name, data in sorted(files.items()) if name != 'MANIFEST.JSN'})
    return files


def write_packet(path, files, kit='MML3AUTH'):
    kit_layout(kit)
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(kit + '/' + name, (1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def verify_packet(path, expected, base, kit='MML3AUTH'):
    layout = kit_layout(kit)
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        require(len(names) == len(set(n.upper() for n in names)), 'Duplicate output member')
        require(set(names) == {kit + '/' + n for n in expected}, 'Output inventory differs')
        files = {name: archive.read(kit + '/' + name) for name in expected}
    require(files == expected, 'Output content differs')
    record = json.loads(files['BUILD.JSN'])
    for name, wanted in record['preserved'].items():
        require(files[name] == base[name] and pin(files[name]) == wanted, 'Preserved member differs: ' + name)
    require(set(record['preserved']) == set(base) - replacement_paths(kit), 'Preservation inventory differs')
    require(record['kit'] == kit, 'Candidate kit metadata differs')
    for name, paths in layout['runtimes'].items():
        for member in paths:
            require(pin(files[member]) == record['runtimes'][name], 'Updated runtime pin differs: ' + member)
    if kit == 'MML3AUTH':
        require(files['BEATS/MIDIMAP.DRV'] == files['FIDDLE/MIDIMAP.DRV'], 'Paired authoring mappers differ')
    for member in layout['siblings']:
        require(files[member] == base[member], 'Sibling application changed: ' + member)
    for member in ('TWSETUP.EXE', 'TWXT2.ZIP'):
        require(files[member] == base[member], 'Shared runtime changed: ' + member)
    require(set(record['regression_inputs']) == set(REGRESSION_INPUTS), 'Regression source inventory differs')
    for member, relative in REGRESSION_INPUTS.items():
        require(files[member] == (ROOT / relative).read_bytes() and
                pin(files[member]) == record['regression_inputs'][member], 'Regression input differs: ' + member)
    require(set(record['frozen_regression_inputs']) == set(FROZEN_ALIASES), 'Frozen regression source inventory differs')
    for member in FROZEN_ALIASES:
        require(pin(files[member]) == record['frozen_regression_inputs'][member], 'Frozen regression input differs: ' + member)
    require(files['EVIDENCE/BEFORE.JSN'] == base['BUILD.JSN'], 'Baseline receipt changed')
    require(pin(files['EVIDENCE/DWABUILD.JSN']) == record['native_build'], 'Native receipt changed')
    manifest = json.loads(files['MANIFEST.JSN'])
    require(manifest == {n: pin(b) for n, b in files.items() if n != 'MANIFEST.JSN'}, 'Output manifest differs')
    return pin(Path(path).read_bytes())


def verify_release_packet(path, kit='MML3AUTH', release=None, expected=None, current=False):
    """Read-only schema2 verification; no baseline ZIP, compiler or emulator."""
    layout = kit_layout(kit)
    raw = Path(path).read_bytes()
    if expected is not None:
        require(pin(raw) == expected, 'Published packet identity differs')
    if release is None:
        release = release_record()
    require(release['package'] == release['assets']['TWXT2.ZIP'] and
            release['installer'] == release['assets']['TWSETUP.EXE'], 'Current shared release pins are inconsistent')
    files, seen = {}, set()
    with zipfile.ZipFile(path) as archive:
        require(sum(i.file_size for i in archive.infolist()) <= 64*1024*1024, 'Published packet too large')
        for item in archive.infolist():
            name = item.filename
            parts = PurePosixPath(name).parts
            require(name.startswith(kit + '/') and '\\' not in name and ':' not in name and
                    all(p not in ('', '.', '..') for p in parts) and name == '/'.join(parts), 'Unsafe published member')
            require(name.upper() not in seen and not item.is_dir() and
                    not stat.S_ISLNK(item.external_attr >> 16), 'Duplicate, directory or linked published member')
            seen.add(name.upper())
            files[name[len(kit)+1:]] = archive.read(item)
    manifest = json.loads(files['MANIFEST.JSN'])
    require(manifest == {n: pin(b) for n,b in files.items() if n != 'MANIFEST.JSN'}, 'Published manifest differs')
    record = json.loads(files['BUILD.JSN'])
    require(record.get('schema') == 2 and record.get('kit') == kit, 'Not this kit schema2 packet')
    baseline_pins = [release['assets'][kit + '.ZIP']]
    if kit + '.ZIP' in release.get('beatlab_baselines', {}):
        baseline_pins.append(release['beatlab_baselines'][kit + '.ZIP'])
    require(record['baseline'] in baseline_pins, 'Unrecognized or downlevel bank baseline')
    require(not (set(record['preserved']) & set(record['changes'])), 'Preserved and changed inventories overlap')
    require(set(record['preserved']) | set(record['changes']) | {'BUILD.JSN','MANIFEST.JSN'} == set(files),
            'Published preservation/change inventory differs')
    for table in ('preserved','changes'):
        for name,wanted in record[table].items():
            require(name in files and pin(files[name]) == wanted, 'Published ' + table + ' pin differs: ' + name)
    require(not (set(record['preserved']) & replacement_paths(kit)), 'Updated runtime incorrectly marked preserved')
    for name in ('TWSETUP.EXE','TWXT2.ZIP') + tuple(layout['siblings']):
        require(name in record['preserved'], 'Protected input missing from preserved inventory: ' + name)
    for name in ('TWSETUP.EXE','TWXT2.ZIP'):
        require(pin(files[name]) == release['assets'][name], 'Published packet has stale/current shared payload mixing: ' + name)
    require(record['baseline_record'] == 'EVIDENCE/BEFORE.JSN' and 'EVIDENCE/BEFORE.JSN' in files,
            'Original baseline receipt missing')
    build_raw = files['EVIDENCE/DWABUILD.JSN']
    require(pin(build_raw) == record['native_build'], 'Published native receipt differs')
    native = json.loads(build_raw)
    require(native.get('target') == '8086 Windows 3.0 real mode; MSC6/LINK4' and
            native.get('status') == 'native compile/link only; execution separate', 'Published native receipt scope differs')
    require(native.get('source_paths') == SOURCE_PATHS and set(native.get('sources',{})) == set(PRODUCTION) and
            set(native.get('source_originals',{})) == set(PRODUCTION), 'Published compiled source inventory differs')
    for name,relative in SOURCE_PATHS.items():
        data = files['SOURCE/' + relative]
        require(data and data == crlf(data) and pin(data) == native['sources'][name], 'Published compiled source pin differs: ' + name)
        if current:
            require(crlf((ROOT/relative).read_bytes()) == data, 'Current compiled source differs: ' + name)
        original = native['source_originals'][name]
        require(isinstance(original.get('bytes'),int) and len(data.replace(b'\r\n',b'\n')) <= original['bytes'] <= len(data) and
                re.fullmatch('[0-9a-f]{64}',original.get('sha256','')), 'Published original source metadata differs: ' + name)
    require(files['SOURCE/src/win30/FIDDLE/MIDIAPI.H'] == files[SOURCE_PREFIX+'MIDIAPI.H'], 'Published MIDI source headers differ')
    if current:
        require(crlf((ROOT/'src/win30/FIDDLE/MIDIAPI.H').read_bytes()) == files['SOURCE/src/win30/FIDDLE/MIDIAPI.H'], 'Current Fiddle MIDI header differs')
        for member,relative in REGRESSION_INPUTS.items():
            require((ROOT/relative).read_bytes() == files[member], 'Current regression input differs: ' + relative)
    require(set(native.get('runtimes',{})) == set(RUNTIME_PATHS) and native.get('runtime') == native['runtimes']['BEATS.EXE'] and
            record['runtimes'] == native['runtimes'], 'Published native runtime inventory differs')
    for name,paths in layout['runtimes'].items():
        for member in paths:
            data = files[member]
            require(pin(data) == native['runtimes'][name], 'Published runtime pin differs: ' + member)
            require(len(data) >= 64 and data[:2] == b'MZ', 'Published native header missing: ' + member)
            ne = struct.unpack_from('<I',data,0x3c)[0]
            require(ne+64 <= len(data) and data[ne:ne+2] == b'NE' and struct.unpack_from('<H',data,ne+0x3e)[0] == 0x0300,
                    'Published Windows 3.0 target differs: ' + member)
    if kit == 'MML3AUTH':
        require(files['BEATS/MIDIMAP.DRV'] == files['FIDDLE/MIDIMAP.DRV'], 'Published paired mappers differ')
    require(set(record['regression_inputs']) == set(REGRESSION_INPUTS) and
            set(record['frozen_regression_inputs']) == set(FROZEN_ALIASES), 'Published regression source inventory differs')
    frozen_pins = [release['assets']['MML3AUTH.ZIP']]
    if 'MML3AUTH.ZIP' in release.get('beatlab_baselines',{}):
        frozen_pins.append(release['beatlab_baselines']['MML3AUTH.ZIP'])
    require(record['frozen_regression_baseline'] in frozen_pins, 'Published frozen regression baseline differs')
    for table in ('regression_inputs','frozen_regression_inputs'):
        for name,wanted in record[table].items():
            require(files[name] and pin(files[name]) == wanted, 'Published regression source pin differs: ' + name)
    demo_names = {layout['demo_prefix']+stem+ext for stem in STEMS for ext in ('.BTS','.MML')}
    require(set(record['demos']) == demo_names, 'Published demo inventory differs')
    preset_names = (b'KEYS',b'ORGAN',b'BASS',b'PAD',b'REED',b'LEAD',b'BELL',b'HIT')
    for stem in STEMS:
        bts_name = layout['demo_prefix'] + stem + '.BTS'
        mml_name = layout['demo_prefix'] + stem + '.MML'
        bts,mml = files[bts_name],files[mml_name]
        require(pin(bts) == record['demos'][bts_name] and pin(mml) == record['demos'][mml_name], 'Published demo pin differs: ' + stem)
        require(0 < len(bts) <= 512 and re.fullmatch(rb'DWA1[\r\n][0-9 \t\r\n]+',bts), 'Published pattern format differs: ' + stem)
        values = [int(x) for x in bts.split()[1:]]
        require(len(values) == 45 and 40 <= values[0] <= 240 and values[1] in (0,1) and
                all(0 <= x <= 127 for x in values[2:5]), 'Published pattern metadata differs: ' + stem)
        for row in range(5):
            low,high = (35,81) if row == 3 else (26,96) if row == 4 else (45,96)
            require(all(x == 0 or low <= x <= high for x in values[5+row*8:13+row*8]), 'Published pattern note range differs: ' + stem)
        require(0 < len(mml) < 2048 and mml.startswith(b'MML3' if values[1] else b'MML2') and
                (b'[P]' in mml) == bool(values[1]) and all(part in mml for part in (b'[A]',b'[B]',b'[C]',b'[N]')) and
                re.findall(rb'@([A-Z]+)',mml) == [preset_names[x>>4] for x in values[2:5]], 'Published MML/pattern mismatch: ' + stem)
    return pin(raw)


def build(base_kit, native_build, out, kit='MML3AUTH', regression_kit=None):
    out = Path(out).resolve()
    require(not out.exists(), 'Output directory must be new')
    base_raw, base = read_kit(base_kit, kit)
    require('EVIDENCE/BEFORE.JSN' not in base, 'Use the preceding bank baseline, not an already updated Beats kit')
    runtimes, sources, build_raw = native_inputs(native_build)
    regression = None
    if kit == 'PITEXPR':
        peer = Path(regression_kit) if regression_kit else Path(base_kit).parent / 'MML3AUTH.ZIP'
        regression = read_kit(peer, 'MML3AUTH')
    out.mkdir(parents=True)
    demo_source = out / 'DEMSRC'
    demo_source.mkdir()
    for name in ('BEATPAT.C', 'BEATPAT.H', 'BEATEXP.C', 'BEATEXP.H'):
        require(SOURCE_PREFIX + name in sources, 'Captured demo input missing: ' + name)
        (demo_source / name).write_bytes(sources[SOURCE_PREFIX + name])
    demos = make_demos(out / 'HOST', here=demo_source)
    files = assemble(base_raw, base, runtimes, sources, build_raw, demos, kit=kit, regression=regression)
    path = out / (kit + '.ZIP')
    write_packet(path, files, kit)
    identity = verify_packet(path, files, base, kit)
    (out / 'PACKET.JSN').write_bytes(json_bytes({'kit': kit, 'candidate': identity, 'native_build': pin(build_raw),
        'scope': 'Tandy Beats Lab authoring clients; exact current shared runtime preservation; native execution acceptance separate'}))
    return identity


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit', choices=tuple(KITS), default='MML3AUTH')
    parser.add_argument('--base-kit', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--regression-kit', type=Path, help='PITEXPR: matching bank-baseline MML3AUTH peer for frozen regression sources')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.base_kit, args.build, args.out, args.kit, args.regression_kit), indent=2))
