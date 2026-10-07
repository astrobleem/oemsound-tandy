"""Verify exact public source snapshots and release archive identities; no native tools."""
import argparse,hashlib,json,re,zipfile
from pathlib import Path,PurePosixPath
ROOT=Path(__file__).resolve().parents[2]/'experimental'/'PITPUBLIC'
H=lambda b:hashlib.sha256(b).hexdigest()
def check_sources():
 pins=json.loads((ROOT/'SOURCE-PINS.json').read_bytes())
 total=0
 for kit,files in pins.items():
  source=ROOT/kit/'SOURCE'
  actual={p.relative_to(source).as_posix():H(p.read_bytes()) for p in source.rglob('*') if p.is_file()}
  assert actual==files,(kit,'source inventory/hash changed')
  names={n.lower() for n in files}
  for n in files:
   path=PurePosixPath(n)
   assert not set(path.parts)&{'BIN','LIB','DDK','INCLUDE','DOS'}
   assert path.suffix.upper() in {'.C','.H','.ASM','.DEF','.RSP','.BAT','.PY','.MD','.TXT','.JSON','.LOG','.PATCH'} or path.name=='LICENSE'
   if path.suffix.upper() not in {'.C','.H'}:continue
   for inc in re.findall(rb'^\s*#\s*include\s*"([^"\r\n]+)"',(source/n).read_bytes(),re.M):
    target=(source/path.parent/inc.decode()).resolve()
    assert target.is_relative_to(source.resolve()),(kit,n,inc,'outside snapshot')
    rel=target.relative_to(source.resolve()).as_posix().lower()
    # BUILD.PY deliberately stages SHARED headers beside each client.
    staged = inc.decode().upper() in {'PITCLNT.H','SNDOWN.H'} and any(
        f.upper().endswith('/SHARED/'+inc.decode().upper()) for f in files)
    staged = staged or (kit=='MML3AUTH' and inc.decode().upper() in {'MMLCORE.H','MMLCORE.C'}
        and any(f.upper().endswith('/MMLFIDL/'+inc.decode().upper()) for f in files))
    assert rel in names or staged,(kit,n,inc,'missing local include')
  total+=len(files)
 print('PASS source snapshots, hashes, exclusions and local C include closure:',total,'files')
def check_assets(folder):
 p=Path(folder); provenance=json.loads((p/'PROVENANCE.json').read_bytes())
 expected=set(provenance['assets'])|{'PROVENANCE.json','SHA256SUMS'}
 assert {f.name for f in p.iterdir()}==expected
 for name,meta in provenance['assets'].items():
  b=(p/name).read_bytes();assert [len(b),H(b)]==[meta['bytes'],meta['sha256']],name
  if not name.endswith('.ZIP'):continue
  with zipfile.ZipFile(p/name) as z:
   assert z.testzip() is None
   assert len(z.namelist())==len(set(z.namelist()))
   assert set(z.namelist())==set(meta['members'])
   for n,m in meta['members'].items():
    path=PurePosixPath(n);assert not path.is_absolute() and '..' not in path.parts and '\\' not in n
    assert path.suffix.upper() not in {'.WAV','.MP3','.M4A','.AVI','.PNG','.GIF','.BMP','.ZIP','.LIB','.OBJ','.IMG','.SYS','.DLL','.COM'}
    assert not set(path.parts)&{'BIN','LIB','INCLUDE','DDK','DOS'}
    data=z.read(n);assert [len(data),H(data)]==[m['bytes'],m['sha256']],n
    if path.suffix.upper() in {'.EXE','.DRV'}:assert H(data)==provenance['runtime_pins'][name][n]
 checks=''.join(H((p/n).read_bytes())+'  '+n+'\n' for n in sorted(expected-{'SHA256SUMS'}))
 assert (p/'SHA256SUMS').read_text()==checks
 print('PASS all public assets, ZIP member identities/exclusions and runtime pins')
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--assets');a=ap.parse_args();check_sources()
 if a.assets:check_assets(a.assets)
