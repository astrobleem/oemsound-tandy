"""Reject stale current inputs and unqualified archival-source transitions."""
from pathlib import Path
from importlib.machinery import SourceFileLoader
import io, json, shutil, tempfile, zipfile

HERE = Path(__file__).resolve().parent
v = SourceFileLoader('beats_public_verifier', str(HERE/'PUBVERIFY.PY')).load_module()
files, build, raw = v.verify()
checks = 1
with tempfile.TemporaryDirectory(prefix='beats-publish-') as folder:
    root = Path(folder)
    names = set(build['source_paths'].values()) | {
        'experimental/WINLIB30/SOURCE/'+n for n in raw['sources']} | {
        'src/win30/BEATLAB/FXNATIVE.PY', 'src/win30/BEATLAB/FXAUDIO.PY',
        'tools/WINAMP/RELEASE.JSN', 'artifacts/win30/BEATSFX.ZIP',
        'artifacts/win30/MML3AUTH.ZIP'}
    for name in names:
        target = root/name;target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(v.ROOT/name, target)
    v.verify(root);checks += 1
    old_clock = None
    with zipfile.ZipFile(io.BytesIO(files['TWXT2.ZIP'])) as old:
        for name in ('TIMECLK.C', 'TCBURST.H', 'TIMECLK.DEF'):
            wanted = v.pin(old.read('TWXT2/SOURCE/RAW/'+name))
            if name == 'TIMECLK.C':old_clock = wanted
            assert v.accepts_recorded_pin('experimental/WINLIB30/SOURCE/'+name, wanted, root)
            checks += 1
    with zipfile.ZipFile(root/'artifacts/win30/MML3AUTH.ZIP') as old:
        name = 'src/win30/BEATLAB/BEATS.C'
        assert v.accepts_recorded_pin(name, v.pin(old.read('MML3AUTH/SOURCE/'+name)), root)
        checks += 1
    assert not v.accepts_recorded_pin('src/win30/DRIVER/TSOUND.C', v.pin(b'unknown'), root)
    assert not v.accepts_recorded_pin('experimental/WINLIB30/SOURCE/TIMECLK.C', v.pin(b'unknown'), root)
    checks += 2
    for name in ('src/win30/BEATLAB/BEATS.C', 'experimental/WINLIB30/SOURCE/TIMECLK.C',
                 'src/win30/BEATLAB/FXAUDIO.PY', 'artifacts/win30/BEATSFX.ZIP'):
        path = root/name;original = path.read_bytes();path.write_bytes(original+b'changed')
        try:v.verify(root)
        except AssertionError:checks += 1
        else:raise AssertionError('Accepted stale input: '+name)
        assert not v.accepts_recorded_pin('experimental/WINLIB30/SOURCE/TIMECLK.C', old_clock, root)
        path.write_bytes(original)
    path = root/'src/win30/BEATLAB/BEATS.C'
    path.write_bytes(v.lf(path.read_bytes()));v.verify(root);checks += 1
print('PASS publication source/archive and version-transition checks='+str(checks))
