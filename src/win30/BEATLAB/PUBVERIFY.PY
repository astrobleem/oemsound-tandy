"""Verify the approved Beats Effects artifact and explicit source-version transitions.
This reads recorded native evidence; it does not claim fresh native execution.
"""
from pathlib import Path, PurePosixPath
import argparse, hashlib, io, json, zipfile

ROOT = Path(__file__).resolve().parents[3]
APPROVED = dict(bytes=1284343, sha256='b0bba1de8573bca46b5ca0e1e849397b3ae16e83f01447cb104f26ee49cd248a')

def pin(data):
    return dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())

def lf(data):
    return data.replace(b'\r\n', b'\n')

def crlf(data):
    return lf(data).replace(b'\n', b'\r\n')

def verify(root=ROOT, extract=None):
    root = Path(root).resolve()
    data = (root/'artifacts/win30/BEATSFX.ZIP').read_bytes()
    assert pin(data) == APPROVED, 'Approved Beats archive differs'
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        names = z.namelist()
        assert len(names) == len(set(names)) and z.testzip() is None
        assert all(n.startswith('BEATSFX/') and '\\' not in n and ':' not in n and
                   '..' not in PurePosixPath(n).parts for n in names)
        files = {n[8:]: z.read(n) for n in names}
        manifest = json.loads(files['MANIFEST.JSN'])['files']
        assert set(files) == set(manifest) | {'MANIFEST.JSN'}
        for name, wanted in manifest.items():
            assert pin(files[name]) == wanted, name
        build = json.loads(files['EVIDENCE/BUILD.JSN'])
        raw = json.loads(files['EVIDENCE/RAWBLD.JSN'])
        native = json.loads(files['EVIDENCE/NATIVE.JSN'])
        audio = json.loads(files['EVIDENCE/AUDIO.JSN'])
        assert native['status'] == audio['status'] == 'PASS'
        assert native['assertions'] == 65 and len(audio['segments']) == 16
        assert all(s['pass_check'] for s in audio['segments'])
        assert not native['playback_only'] and not native['visual_only'] and not native['trace_input']
        assert native['normal_dos_return'] and native['owned_child_stopped'] and native['lease_released'] and native['inventory_after'] == '0'
        assert audio['native_result'] == pin(files['EVIDENCE/NATIVE.JSN'])
        assert native['runtime'] == audio['runtime'] == build['runtimes']['BEATS.EXE']
        assert native['raw_runtime'] == raw['runtime'] == pin(files['RAWCLK.DLL'])
        for name, wanted in build['runtimes'].items():
            assert pin(files[name]) == wanted, name
        for name, relative in build['source_paths'].items():
            staged = files['SOURCE/'+relative]
            assert pin(staged) == build['sources'][name], name
            assert crlf((root/relative).read_bytes()) == staged, 'Current app source differs: '+relative
        for name, wanted in raw['sources'].items():
            relative = 'experimental/WINLIB30/SOURCE/'+name
            assert pin(files['SOURCE/'+relative]) == wanted, name
            assert pin((root/relative).read_bytes()) == wanted, 'Current RAW source differs: '+name
        for name, wanted in (('FXNATIVE.PY', native['harness']), ('FXAUDIO.PY', audio['validator'])):
            relative = 'src/win30/BEATLAB/'+name
            assert pin(files['SOURCE/'+relative]) == wanted, name
            assert lf((root/relative).read_bytes()) == lf(files['SOURCE/'+relative]), 'Current verifier differs: '+name
        release = json.loads((root/'tools/WINAMP/RELEASE.JSN').read_bytes())
        for name in ('TWXT2.ZIP', 'TWSETUP.EXE'):
            assert pin(files[name]) == release['assets'][name], name
        if extract:
            out = Path(extract).resolve();out.mkdir(parents=True, exist_ok=False)
            z.extractall(out)
    return files, build, raw

def accepts_recorded_pin(relative, wanted, root=ROOT):
    """Permit only an exact old release -> approved new compiled-input pair.

    All unrelated sources retain their existing current-source equality checks.
    The old binary is still verified against its own archived compiled source.
    """
    relative = str(relative).replace('\\', '/')
    shared = 'experimental/WINLIB30/SOURCE/'
    if not (relative in {shared+n for n in ('TIMECLK.C', 'TCBURST.H', 'TIMECLK.DEF')} or
            relative.startswith('src/win30/BEATLAB/')):
        return False
    root = Path(root).resolve()
    try:
        files, build, raw = verify(root)
        if relative.startswith(shared):
            with zipfile.ZipFile(io.BytesIO(files['TWXT2.ZIP'])) as old:
                previous = old.read('TWXT2/SOURCE/RAW/'+relative[len(shared):])
        else:
            if relative not in build['source_paths'].values():
                return False
            data = (root/'artifacts/win30/MML3AUTH.ZIP').read_bytes()
            release = json.loads((root/'tools/WINAMP/RELEASE.JSN').read_bytes())
            assert pin(data) == release['assets']['MML3AUTH.ZIP']
            with zipfile.ZipFile(io.BytesIO(data)) as old:
                previous = old.read('MML3AUTH/SOURCE/'+relative)
        return pin(previous) == wanted
    except (AssertionError, OSError, KeyError, ValueError, zipfile.BadZipFile):
        return False

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--extract', type=Path)
    a = p.parse_args();files, build, raw = verify(extract=a.extract)
    print('PASS approved150-file archive,37 app/11 RAW compiled inputs, runtime/evidence/source binding; recorded native65/audio16')
