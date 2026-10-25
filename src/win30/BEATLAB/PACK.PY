"""Prepare an unpublished Beats client update; preserve shared runtime bytes."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import shutil
import stat
import struct
import subprocess
import sys
import zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PRODUCTION = ('BEATS.C', 'BEATPAT.C', 'BEATPAT.H', 'BEATEXP.C',
              'BEATEXP.H', 'MIDIAPI.H', 'BEATS.DEF', 'BEATS.RSP', 'BUILD.BAT')
STEMS = ('STARTER', 'POCKET', 'LOWRIDER', 'FIVE', 'NIGHT')
WINDOWS_GCC = Path('C:/Users/chad/Documents/Codex/toolchains/gcc-ucrt64.py')
SOURCE_PREFIX = 'SOURCE/src/win30/BEATLAB/'
GUIDE = b'''DWA: DAWs With Attitude - Fiddle and Beats authoring kit\r
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
runtime and Fiddle bytes are preserved; the new Beats build and demo\r
identities are in BUILD.JSN. Older evidence retains its original scope.\r
'''
BEATS_GUIDE = b'''DWA Beats Lab - DAWs With Attitude\r
Straight Outta LLMompton. Produced by Dr. DRAM.\r
\r
Click Demo to cycle Starter, Pocket drums, Lowrider lead, Speaker bass\r
and Late-night keys. These original G-funk-style educational patterns\r
contain no copied recordings, lyrics or recognizable song melodies.\r
\r
Click a grid cell to toggle note/rest. Arrows select; Space/Enter toggles;\r
+/- changes pitch; Delete rests. Click tone labels for shipped presets.\r
PCspkr is row 5. Its visible PIT switch controls playback and export;\r
the Speaker bass demo enables it automatically. No extra activation step.\r
Tempo +/- or Page Down/Up changes BPM. Play starts at step 1. Stop/Escape\r
releases sound; P/S works with grid focus, and F5 toggles playback.\r
Tab/Shift+Tab navigates controls. About/F1 shows key controls.\r
\r
Enter NAME.BTS beside Load/Save. Save preserves all 40 cells, tempo,\r
presets and PIT state; Load reopens them. Ctrl+S saves, Ctrl+O loads.\r
Replacement asks first and keeps the previous file as .BAK. Bad input\r
leaves the current edit intact. Ready editable examples are:\r
  STARTER.BTS  POCKET.BTS  LOWRIDER.BTS  FIVE.BTS  NIGHT.BTS\r
\r
MML shows the generated score. Copy copies it; Export writes NAME.MML\r
beside the app without replacing an existing .MML. Same-named demo .MML\r
files are supplied for Fiddle. The grid loads .BTS, not arbitrary MML.\r
\r
Timing is straight eighths with 75 percent gate: no swing, glide, ties\r
or per-cell velocity. The unchanged mapper accepts melodic A2-C7;\r
PC-speaker bass uses its actual low limit A2 (110 Hz), not octave folding.\r
The speaker has fixed level and no PSG preset envelope.\r
\r
Run the complete kit's TWSETUP.EXE from DOS for shared sound setup. Keep\r
paired MIDIMAP.DRV beside Beats; do not install or swap it system-wide.\r
Stop one sound producer before starting another.\r
'''


def require(ok, message):
    if not ok:
        raise ValueError(message)


def pin(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode('utf-8')


def crlf(data):
    return data.replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')


def read_kit(path):
    raw = Path(path).read_bytes()
    release = json.loads((ROOT / 'tools/WINAMP/RELEASE.JSN').read_bytes())
    require(pin(raw) == release['assets']['MML3AUTH.ZIP'], 'Base kit is not the current pinned MML3AUTH release')
    files, seen = {}, set()
    with zipfile.ZipFile(Path(path)) as archive:
        require(sum(i.file_size for i in archive.infolist()) <= 64 * 1024 * 1024, 'Base kit too large')
        for item in archive.infolist():
            name = item.filename
            parts = PurePosixPath(name).parts
            require(name.startswith('MML3AUTH/') and '\\' not in name and ':' not in name and
                    all(p not in ('', '.', '..') for p in parts) and name == '/'.join(parts), 'Unsafe kit member')
            require(name.upper() not in seen and not stat.S_ISLNK(item.external_attr >> 16), 'Duplicate or linked kit member')
            seen.add(name.upper())
            require(not item.is_dir(), 'Unexpected directory-only member')
            files[name[len('MML3AUTH/'):]] = archive.read(item)
    manifest = json.loads(files['MANIFEST.JSN'])
    require(set(manifest) == set(files) - {'MANIFEST.JSN'}, 'Base kit inventory differs')
    for name, wanted in manifest.items():
        require(pin(files[name]) == wanted, 'Base member differs: ' + name)
    for name in ('TWSETUP.EXE', 'TWXT2.ZIP', 'BEATS/MIDIMAP.DRV', 'FIDDLE/MIDIMAP.DRV', 'FIDDLE/MMLFIDL.EXE'):
        require(name in files, 'Required preserved input missing: ' + name)
    return raw, files


def native_inputs(build, here=HERE):
    build, here = Path(build), Path(here)
    record_raw = (build / 'BUILD.JSON').read_bytes()
    record = json.loads(record_raw)
    require(record.get('target') == '8086 Windows 3.0 real mode; MSC6/LINK4', 'Unexpected native target')
    require(record.get('status') == 'native compile/link only; execution separate', 'Unexpected native build scope')
    require(set(record.get('sources', {})) == set(PRODUCTION) and
            set(record.get('source_originals', {})) == set(PRODUCTION), 'Incomplete compiled source inventory')
    sources = {}
    for name in PRODUCTION:
        original, staged = (here / name).read_bytes(), (build / name).read_bytes()
        require(pin(original) == record['source_originals'][name], 'Current source differs from native receipt: ' + name)
        require(pin(staged) == record['sources'][name] and staged == crlf(original), 'Compiled source differs: ' + name)
        sources[SOURCE_PREFIX + name] = staged
    exe = (build / 'BEATS.EXE').read_bytes()
    require(pin(exe) == record.get('runtime'), 'Native BEATS.EXE differs from build receipt')
    require(len(exe) >= 64 and exe[:2] == b'MZ', 'Not a native Beats executable')
    ne = struct.unpack_from('<I', exe, 0x3c)[0]
    require(ne + 64 <= len(exe) and exe[ne:ne+2] == b'NE' and
            struct.unpack_from('<H', exe, ne+0x3e)[0] == 0x0300, 'Expected Windows 3.0 NE executable')
    process = json.loads((build / 'PROCESS.JSON').read_bytes())
    require(all(process.get(k) is True for k in ('lease_acquired', 'lease_released', 'owned_child_stopped')) and
            process.get('return') == 0 and not process.get('error') and not process.get('watchdog'),
            'Native build did not complete under the shared lease')
    return exe, sources, record_raw


def make_demos(work, here=HERE):
    work, here = Path(work), Path(here)
    work.mkdir(parents=True, exist_ok=False)
    helper = work / 'DEMOS.C'
    helper.write_text('''#include <stdio.h>
#include <string.h>
#include "BEATPAT.H"
#if BP_DEMOS != 5
#error Update_the_packet_demo_inventory
#endif
int main(void){
 static const char *names[5]={"STARTER","POCKET","LOWRIDER","FIVE","NIGHT"};
 static BPAT p,loaded;static char text[BP_MML_CAP];char path[20];FILE *f;unsigned i,n;
 for(i=0;i<BP_DEMOS;++i){
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
                          '-Wno-misleading-indentation', '-I', str(here), str(helper),
                          str(here / 'BEATPAT.C'), str(here / 'BEATEXP.C'), '-o', str(exe)]
    env = dict(os.environ)
    if os.name == 'nt':
        env['PATH'] = 'C:/msys64/ucrt64/bin' + os.pathsep + env.get('PATH', '')
    subprocess.run(command, cwd=work, env=env, check=True, timeout=35)
    subprocess.run([str(exe)], cwd=work, env=env, check=True, timeout=10)
    return {'BEATS/' + stem + ext: (work / (stem + ext)).read_bytes()
            for stem in STEMS for ext in ('.BTS', '.MML')}


def assemble(base_raw, base, exe, sources, build_raw, demos, here=HERE):
    require(set(demos) == {'BEATS/' + stem + ext for stem in STEMS for ext in ('.BTS', '.MML')}, 'Demo inventory differs')
    require(all(data for data in demos.values()), 'Empty demo')
    require('EVIDENCE/BEFORE.JSN' not in base, 'Already updated kit is not a baseline')
    files = dict(base)
    files.update(sources)
    for name in ('BUILD.PY', 'PACK.PY', 'PACKTEST.PY', 'HOSTTEST.PY', 'README.MD', 'VALIDATE.MD'):
        files[SOURCE_PREFIX + name] = (Path(here) / name).read_bytes()
    for path in sorted((Path(here) / 'TESTS').rglob('*')):
        if path.is_file() and '__pycache__' not in path.parts and path.suffix.lower() in ('.c', '.h', '.py', '.md', '.txt', '.bts', '.mml'):
            files[SOURCE_PREFIX + path.relative_to(here).as_posix()] = path.read_bytes()
    files['SOURCE/src/win30/LAUNCH/RUNDOS.PY'] = (ROOT / 'src/win30/LAUNCH/RUNDOS.PY').read_bytes()
    files.update(demos)
    files.update({'BEATS/BEATS.EXE': exe, 'BEATS/README.TXT': BEATS_GUIDE,
                  'README.TXT': GUIDE, 'INSTALL.TXT': GUIDE,
                  'EVIDENCE/BEFORE.JSN': base['BUILD.JSN'], 'EVIDENCE/DWABUILD.JSN': build_raw})
    replaced = {'BEATS/BEATS.EXE', 'README.TXT', 'INSTALL.TXT', 'BUILD.JSN', 'MANIFEST.JSN'}
    preserved = {name: pin(data) for name, data in base.items() if name not in replaced}
    for name in preserved:
        require(files[name] == base[name], 'Unexpected baseline change: ' + name)
    files['BUILD.JSN'] = json_bytes({
        'schema': 2, 'kit': 'MML3AUTH', 'operation': 'DWA Beats client update; shared runtime and Fiddle unchanged',
        'baseline': pin(base_raw), 'baseline_record': 'EVIDENCE/BEFORE.JSN',
        'native_build': pin(build_raw), 'preserved': preserved,
        'changes': {name: pin(data) for name, data in files.items()
                    if name not in ('BUILD.JSN', 'MANIFEST.JSN') and (name not in base or data != base[name])},
        'demos': {name: pin(data) for name, data in demos.items()},
        'qualification': {'native_build': 'Compile/link receipt only; execution acceptance is separate',
                          'shared': 'Exact base installer and shared companion preserved',
                          'historical_unchanged_client_gate': False,
                          'publication': 'Unpublished local candidate; explicit approval required'}})
    files['MANIFEST.JSN'] = json_bytes({name: pin(data) for name, data in sorted(files.items()) if name != 'MANIFEST.JSN'})
    return files


def write_packet(path, files):
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo('MML3AUTH/' + name, (1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def verify_packet(path, expected, base):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        require(len(names) == len(set(n.upper() for n in names)), 'Duplicate output member')
        require(set(names) == {'MML3AUTH/' + n for n in expected}, 'Output inventory differs')
        files = {name: archive.read('MML3AUTH/' + name) for name in expected}
    require(files == expected, 'Output content differs')
    record = json.loads(files['BUILD.JSN'])
    for name, wanted in record['preserved'].items():
        require(files[name] == base[name] and pin(files[name]) == wanted, 'Preserved member differs: ' + name)
    require(files['EVIDENCE/BEFORE.JSN'] == base['BUILD.JSN'], 'Baseline receipt changed')
    require(pin(files['EVIDENCE/DWABUILD.JSN']) == record['native_build'], 'Native receipt changed')
    manifest = json.loads(files['MANIFEST.JSN'])
    require(manifest == {n: pin(b) for n, b in files.items() if n != 'MANIFEST.JSN'}, 'Output manifest differs')
    return pin(Path(path).read_bytes())


def build(base_kit, native_build, out):
    out = Path(out).resolve()
    require(not out.exists(), 'Output directory must be new')
    base_raw, base = read_kit(base_kit)
    exe, sources, build_raw = native_inputs(native_build)
    out.mkdir(parents=True)
    demos = make_demos(out / 'HOST')
    files = assemble(base_raw, base, exe, sources, build_raw, demos)
    path = out / 'MML3AUTH.ZIP'
    write_packet(path, files)
    identity = verify_packet(path, files, base)
    (out / 'PACKET.JSN').write_bytes(json_bytes({'candidate': identity, 'native_build': pin(build_raw),
        'scope': 'Unpublished DWA client update; exact shared runtime/Fiddle preservation; native execution qualification separate'}))
    return identity


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-kit', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.base_kit, args.build, args.out), indent=2))
