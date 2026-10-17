"""Stage private baseline/facade build pairs; no emulator is started here."""
from pathlib import Path
import hashlib, json, shutil

W = Path(__file__).resolve().parent
root = W.parent
out = W / 'READY30'
if out.exists():
    raise SystemExit('READY30 already exists; preserve previous evidence')
out.mkdir()
records = []
for variant in ('AUDOLD', 'AUDNEW'):
    dest = out / variant / 'MMLAB'
    dest.mkdir(parents=True)
    for p in (root / 'audio-adopt/SOURCE').iterdir():
        if p.is_file():
            shutil.copyfile(p, dest / p.name)
    if variant == 'AUDOLD':
        p = dest / 'MINIPLAY.C'
        data = p.read_bytes()
        assert data.count(b'#include "LWAUDIO.H"') == 1
        p.write_bytes(data.replace(b'#include "LWAUDIO.H"',
            b'#include "MCIAPI.H"\r\n#include "RAWAPP.H"'))
    shutil.copyfile(root / 'audio-adopt/BUILD/BUILD30.BAT',
                    dest / 'BUILD30.BAT')
    (out / variant / 'TMP').mkdir()
    p = dest / 'BUILD30.BAT'
    p.write_bytes(p.read_bytes().replace(b'SET TMP=C:\\TMP', b'SET TMP=D:\\TMP'))
    records.append({'job': variant, 'batch': 'MMLAB/BUILD30.BAT',
                    'products': ['MINIPLAY.EXE', 'RAWQA.EXE',
                    'QCHIME.EXE', 'QXP.EXE', 'QEXIT.EXE', 'QPSG.EXE',
                    'QMOUTH.EXE', 'RAWCLK.DLL', 'MIDICLK.DLL',
                    'MCITNDY.DLL', 'TBRIDGE.EXE']})

shell = root / 'windows-adopt/examples/TSHELL'
for variant in ('SHELLOLD', 'SHELLNEW'):
    dest = out / variant
    dest.mkdir()
    # Use the exact input list from the existing build recipe without running it.
    import ast
    tree = ast.parse((shell / 'BUILD.PY').read_text())
    names = None
    for node in tree.body:
        if isinstance(node, ast.For) and isinstance(node.iter, ast.Tuple):
            values = ast.literal_eval(node.iter)
            if 'TSHELL.C' in values:
                names = values
    assert names
    for name in names:
        shutil.copyfile(shell / name, dest / name)
    for name in ('LTWUI.C', 'LTWUI.H', 'LWTANDY.H'):
        shutil.copyfile(root / 'windows-adopt/libtandywin' / name,
                        dest / name)
    # DOS command tails are limited to 127 bytes. Split existing compile
    # groups without changing any compiler flags or linked object list.
    p = dest / 'BUILD.BAT'
    lines = []
    for line in p.read_text().splitlines():
        if line.startswith('CL '):
            before, redirect = line.split(' >', 1) if ' >' in line else (line, '')
            # Both ">" and ">>" have the first literal space-plus->.
            words = before.split()
            sources = [s for s in words if s.upper().endswith('.C')]
            flags = [s for s in words if not s.upper().endswith('.C')]
            for i, source in enumerate(sources):
                redir = (' >' if i == 0 and not redirect.startswith('>')
                         else ' >>') + ' BUILD.LOG'
                lines.append(' '.join(flags + [source]) + redir)
                lines.append('IF ERRORLEVEL 1 GOTO FAIL')
        else:
            lines.append(line)
    p.write_bytes(('\r\n'.join(lines) + '\r\n').encode('ascii'))
    if variant == 'SHELLOLD':
        p = dest / 'TSHELL.C'
        data = p.read_bytes()
        assert data.count(b'#include "LWTANDY.H"') == 1
        p.write_bytes(data.replace(b'#include "LWTANDY.H"',
                                  b'#include "LTWUI.H"'))
    records.append({'job': variant, 'batch': 'BUILD.BAT',
                    'products': ['TSHELL.EXE']})

for row in records:
    directory = out / row['job']
    row['sources'] = [{'name': p.relative_to(directory).as_posix(),
        'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
        for p in sorted(directory.rglob('*')) if p.is_file()]
(out / 'PREP.JSON').write_text(json.dumps(records, indent=2))
print('Prepared four isolated jobs. No emulator launched.')
