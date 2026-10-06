#!/usr/bin/env python3
"""Verify and package qualified artifacts. Does not compile native source."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
import zipfile

ROOT = Path(__file__).resolve().parents[2]
INPUTS = Path(__file__).with_name('INPUTS.JSON')
ASSETS = {'PSGBASE.ZIP', 'BEATS.ZIP', 'WININST12.ZIP', 'BUDENV02.ZIP',
          'SOUNDP01.ZIP', 'SOUNDP02.ZIP', 'README.TXT', 'BUILD.JSON', 'SHA256SUMS'}
README = b'''OEMSound-Tandy verified packages (not a native source rebuild)\r\n\r\nPSGBASE.ZIP: unchanged earlier PSGPLAY/TCHIME/TSOUND milestone.\r\nBEATS.ZIP: unchanged baseline-source Beats/MIDIMAP milestone preview.\r\nWININST12.ZIP: separate experimental expressive Windows instruments.\r\nBUDENV02.ZIP: separate experimental media-free DOS Buddy overlay.\r\nSOUNDP01.ZIP / SOUNDP02.ZIP: source and historical evidence ONLY.\r\nRead SCOPE.TXT first; original kit manifests also describe omitted files.\r\n\r\nExtract each selected ZIP into its own folder and read its original guide.\r\nKeep paired app-local MIDIMAP.DRV beside its clients, outside WINDOWS/SYSTEM.\r\nNo SYSTEM.INI, CF, startup or user installation changes are made by CI.\r\nThe Buddy overlay needs the separately obtained exact BUDCAP04 baseline;\r\nits score/arrangement retains separate rights described in ATTRIB.TXT.\r\nNo original video, speech clips, compiler, SDK or Windows is bundled here.\r\n\r\nCI verifies pinned archive and member hashes, source-publication inventory,\r\nand package exclusions. It does not rerun native/emulator/audio tests.\r\nHistorical emulator evidence is not physical Tandy qualification.\r\nNative rebuilds require lawful external period compiler, Windows headers\r\nand libraries plus DOSBox-X. Current checkout lacks LINK4.EXE,\r\nSLIBCEW.LIB and LIBW.LIB; Open Watcom is not a validated drop-in build.\r\n\r\nBUILD.JSON records the exact source commit and every packaged member.\r\nSHA256SUMS covers all assets except itself. No stable promotion is implied.\r\n'''


def identity(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def safe_name(name):
    parts = name.rstrip('/').split('/')
    require(name and not name.startswith('/') and '\\' not in name
            and all(p not in ('', '.', '..') and ':' not in p for p in parts),
            'Unsafe member: ' + name)
    banned = {'BIN', 'LIB', 'INCLUDE', 'DDK', 'DOS', 'WINDOWS', '.GIT', '.AWS'}
    require(not any(p.upper() in banned for p in parts), 'Excluded directory: ' + name)
    require(PurePosixPath(name).suffix.lower() not in
            {'.lib', '.obj', '.dll', '.sys', '.com', '.img', '.iso', '.zip'},
            'Excluded file type: ' + name)


def inspect_zip(path, runtime):
    members = {}
    seen = set()
    with zipfile.ZipFile(path) as archive:
        require(sum(i.file_size for i in archive.infolist()) < 50_000_000,
                'Archive too large')
        for item in archive.infolist():
            safe_name(item.filename)
            require(item.filename.casefold() not in seen, 'Duplicate ZIP member')
            seen.add(item.filename.casefold())
            require(not stat.S_ISLNK(item.external_attr >> 16), 'ZIP symlink')
            if item.is_dir():
                continue
            data = archive.read(item)
            members[item.filename] = identity(data)
            if PurePosixPath(item.filename).suffix.lower() in {'.exe', '.drv'}:
                require(runtime.get(item.filename) == identity(data),
                        'Unqualified runtime: ' + item.filename)
            if PurePosixPath(item.filename).suffix.lower() in {'.c', '.h', '.asm'}:
                require(b'Microsoft Corporation' not in data,
                        'Microsoft source excluded: ' + item.filename)
        require({n: m for n, m in members.items()
                 if PurePosixPath(n).suffix.lower() in {'.exe', '.drv'}} == runtime,
                'Runtime set changed')
    return members


def verify_original_manifest(path, manifest, prefix=''):
    with zipfile.ZipFile(path) as archive:
        rows = json.loads(archive.read(manifest))
        if isinstance(rows, list):
            rows = {r['path']: {'bytes': r['bytes'], 'sha256': r['sha256']} for r in rows}
        for name, metadata in rows.items():
            require(identity(archive.read(prefix + name)) == metadata,
                    'Historical manifest mismatch: ' + name)


def verify_original_checksums(path, prefix):
    with zipfile.ZipFile(path) as archive:
        for line in archive.read(prefix + 'SHA256SUMS').decode('ascii').splitlines():
            expected, name = line.split('  ', 1)
            safe_name(prefix + name)
            require(hashlib.sha256(archive.read(prefix + name)).hexdigest() == expected,
                    'Historical checksum mismatch: ' + name)


def write_zip(path, files):
    # Stored members avoid compression-library differences across hosts.
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_STORED) as archive:
        for name, data in sorted(files.items()):
            safe_name(name)
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)


def read_exact(root, name):
    path = root / name
    require(path.is_file() and not path.is_symlink(), 'Missing or linked input: ' + name)
    require(path.resolve().is_relative_to(root.resolve()), 'Input escapes repository')
    return path.read_bytes()


def build(root, out, source_sha):
    require(re.fullmatch('[0-9a-f]{40}', source_sha), 'Expected exact source commit')
    require(not out.exists(), 'Output must be new')
    inputs = json.loads(INPUTS.read_bytes())
    archive_data = {}
    records = {}
    for name, spec in inputs['archives'].items():
        data = read_exact(root, spec['path'])
        require(identity(data) == spec['identity'], 'Pinned archive changed: ' + name)
        members = inspect_zip(root / spec['path'], spec['runtime'])
        require(len(members) == spec['files'], 'Archive member count changed')
        if name in {'PSGBASE.ZIP', 'BEATS.ZIP'}:
            verify_original_manifest(root / spec['path'], 'MANIFEST.JSON')
        else:
            prefix = 'WININST/' if name == 'WININST12.ZIP' else 'BUDENV02/'
            verify_original_checksums(root / spec['path'], prefix)
        archive_data[name] = data
        records[name] = {'kind': spec['kind'], 'input': spec['path'],
                         **identity(data), 'members': members}
    publication = read_exact(root, 'experimental/SOURCE-PUBLICATION.json')
    require(identity(publication) == inputs['source_publication'], 'Publication inventory changed')
    published = json.loads(publication)['files']
    sources = {}
    for name, metadata in published.items():
        data = read_exact(root, name)
        require(identity(data) == {'bytes': metadata['bytes'], 'sha256': metadata['sha256']},
                'Published source changed: ' + name)
        require(hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
                == metadata['git_blob_sha1'], 'Published Git blob changed: ' + name)
        sources[name] = data
    for kit in ('SOUNDP01', 'SOUNDP02'):
        prefix = 'experimental/' + kit + '/'
        selected = {kit + '/' + n[len(prefix):]: data for n, data in sources.items()
                    if n.startswith(prefix)}
        # Reject accidental binary publication and extra unmanifested source files.
        actual = {p.relative_to(root).as_posix() for p in (root / 'experimental' / kit).rglob('*')
                  if p.is_file()}
        require(actual == {n for n in published if n.startswith(prefix)}, 'Source file set changed')
        require(all(PurePosixPath(n).name == 'LICENSE' or PurePosixPath(n).suffix.upper() in
                    {'.C', '.H', '.ASM', '.DEF', '.RSP', '.BAT', '.PY', '.TXT', '.MD',
                     '.JSON', '.LOG', '.MAP', '.CNF', '.SHA'} for n in selected),
                'Unexpected source-publication file type')
        selected['SCOPE.TXT'] = sources['experimental/SOUND-PUBLICATION.md']
        selected['FILES.JSON'] = (json.dumps({n: identity(d) for n, d in sorted(selected.items())},
                                             indent=2, sort_keys=True) + '\n').encode()
        archive_data[kit + '.ZIP'] = selected
    out.mkdir(parents=True)
    for name, data in archive_data.items():
        if isinstance(data, dict):
            write_zip(out / name, data)
            records[name] = {'kind': 'experimental-source-only',
                             **identity((out / name).read_bytes()),
                             'members': inspect_zip(out / name, {})}
        else:
            (out / name).write_bytes(data)
    (out / 'README.TXT').write_bytes(README)
    report = {'kind': 'manifest-verified-existing-artifacts', 'source_commit': source_sha,
              'native_compilation': False, 'fresh_native_qa': False,
              'proprietary_tools_included': False, 'archives': records}
    (out / 'BUILD.JSON').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n',
                                   encoding='utf-8', newline='\n')
    (out / 'SHA256SUMS').write_text(checksums(out), encoding='ascii', newline='\n')
    verify(out, source_sha)


def checksums(out):
    return ''.join(identity((out / n).read_bytes())['sha256'] + '  ' + n + '\n'
                   for n in sorted(ASSETS - {'SHA256SUMS'}))


def verify(out, source_sha):
    require({p.name for p in out.iterdir()} == ASSETS, 'Output asset set changed')
    require(all(p.is_file() and not p.is_symlink() for p in out.iterdir()), 'Linked/nonfile output')
    require((out / 'SHA256SUMS').read_text() == checksums(out), 'Output checksum mismatch')
    require((out / 'README.TXT').read_bytes() == README, 'Package guide changed')
    report = json.loads((out / 'BUILD.JSON').read_bytes())
    require(report['source_commit'] == source_sha and report['native_compilation'] is False
            and report['fresh_native_qa'] is False and report['proprietary_tools_included'] is False
            and report['kind'] == 'manifest-verified-existing-artifacts', 'Incorrect provenance')
    inputs = json.loads(INPUTS.read_bytes())
    require(set(report['archives']) == ASSETS - {'README.TXT', 'BUILD.JSON', 'SHA256SUMS'},
            'Archive report set changed')
    for name, metadata in report['archives'].items():
        require(identity((out / name).read_bytes()) ==
                {'bytes': metadata['bytes'], 'sha256': metadata['sha256']}, 'Archive hash changed')
        runtime = inputs['archives'].get(name, {}).get('runtime', {})
        require(inspect_zip(out / name, runtime) == metadata['members'], 'Member report changed')
        if name in inputs['archives']:
            spec = inputs['archives'][name]
            require(identity((out / name).read_bytes()) == spec['identity']
                    and metadata['kind'] == spec['kind'], 'Original archive changed')
        else:
            require(metadata['kind'] == 'experimental-source-only', 'Source scope changed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--source-sha', required=True)
    parser.add_argument('--verify', action='store_true')
    args = parser.parse_args()
    if args.verify:
        verify(args.out, args.source_sha)
    else:
        build(ROOT, args.out, args.source_sha)
    print('PASS: manifest-verified packages; native_compilation=false')
