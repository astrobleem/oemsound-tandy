"""Compose a self-contained private Effects Lab kit against qualified receipts."""
from pathlib import Path
import argparse,hashlib,json,re,shutil,zipfile
p=argparse.ArgumentParser()
for n in ('build','host','native','libtandy','out'):p.add_argument('--'+n,type=Path,required=True)
a=p.parse_args();here=Path(__file__).resolve().parent;root=here.parents[2];out=a.out.resolve();out.mkdir();kit=out/'EFFECTS';kit.mkdir()
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
build=json.loads((a.build/'BUILD.JSON').read_text());host=json.loads((a.host/'RESULT.JSON').read_text());native=json.loads((a.native/'RESULT.JSON').read_text())
assert host['status']=='PASS' and native['status']=='PASS'
assert pin((a.build/'EFFECTS.EXE').read_bytes())==build['runtime']
assert native['exe_sha256']==build['runtime']['sha256']
assert pin((a.build/'FXSTART.EXE').read_bytes())==build['launcher']
source={}
for n in build['original_sources']:
 path=here/n if (here/n).is_file() else here.parent/'LAUNCH'/n if (here.parent/'LAUNCH'/n).is_file() else root/'experimental/WINLIB30/SOURCE'/n
 assert pin(path.read_bytes())==build['original_sources'][n],n
 source[path.relative_to(root).as_posix()]=path.read_bytes()
for path,sha in host['sources'].items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==sha,path
for path in here.iterdir():
 if path.is_file():source[path.relative_to(root).as_posix()]=path.read_bytes()
for n,b in source.items():d=kit/'SOURCE'/n;d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes(b)
for n in ('include/lt_audio.h','include/lt_sound.h','src/lt_audio.c','src/lt_chip.c','LICENSE'):
 d=kit/'SOURCE/LIBTANDY'/n;d.parent.mkdir(parents=True,exist_ok=True);d.write_bytes((a.libtandy/n).read_bytes())
for name in ('EFFECTS.EXE','FXSTART.EXE'):shutil.copyfile(a.build/name,kit/name)
with zipfile.ZipFile(root/'artifacts/win30/TWXT2.ZIP') as z:
 for n in ('RAWCLK.DLL','TBRIDGE.EXE','RESERVE.COM'):(kit/n).write_bytes(z.read('TWXT2/'+n))
for n in ('TWSETUP.EXE','TWXT2.ZIP'):shutil.copyfile(root/'artifacts/win30'/n,kit/n)
shutil.copyfile(root/'LICENSE',kit/'LICENSE.TXT');shutil.copyfile(a.libtandy/'LICENSE',kit/'MIT.TXT')
shutil.copyfile(here/'README.MD',kit/'README.TXT');shutil.copyfile(here/'FXPLAY.H',kit/'FXPLAY.H')
(kit/'GO.BAT').write_bytes(b'@ECHO OFF\r\nIF "%1"=="" GOTO DEFAULT\r\nFXSTART %1\r\nGOTO END\r\n:DEFAULT\r\nFXSTART C:\\WINDOWS\r\n:END\r\n')
stems=('COIN','LASER','BOOM','JUMP','BLIP')
for i,stem in enumerate(stems):
 b=(a.host/('P%u.FXP'%i)).read_bytes();(kit/(stem+'.FXP')).write_bytes(b)
 lines=b.decode('ascii').splitlines();v=lines[2].split();assert len(v)==12;v[-1]='1'
 (kit/(stem+'SP.FXP')).write_bytes(('TFX1\r\n'+lines[1]+'\r\n'+' '.join(v)+'\n').encode('ascii'))
evidence=kit/'EVIDENCE';evidence.mkdir()
for src,name in ((a.build/'BUILD.JSON','BUILD.JSN'),(a.build/'PROCESS.JSON','BLDPROC.JSN'),(a.native/'RESULT.JSON','NATIVE.JSN'),(a.native/'PROCESS.JSON','RUNPROC.JSN'),(a.native/'EFFECTS.LOG','NATIVE.LOG'),(a.host/'RESULT.JSON','HOST.JSN')):shutil.copyfile(src,evidence/name)
shutil.copyfile(a.native/'EDITOR.PNG',kit/'EDITOR.PNG')
if (here/'VALIDATE.MD').is_file():shutil.copyfile(here/'VALIDATE.MD',kit/'VALIDATE.TXT')
manifest={str(f.relative_to(kit)).replace('\\','/'):pin(f.read_bytes()) for f in kit.rglob('*') if f.is_file()}
(kit/'MANIFEST.JSN').write_text(json.dumps(dict(format='Effects Lab private kit v1',runtime=build['runtime'],launcher=build['launcher'],native=native,files=manifest),indent=2))
archive=out/'EFFECTS.ZIP'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for f in sorted(kit.rglob('*')):
  if f.is_file():z.write(f,'EFFECTS/'+str(f.relative_to(kit)).replace('\\','/'))
(out/'PACKAGE.JSON').write_text(json.dumps(dict(status='PASS',zip=pin(archive.read_bytes()),files=len(manifest)+1),indent=2));print(archive)
