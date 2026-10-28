"""Build a private complete Beats/Effects kit from source-pinned acceptance.
No upload, release update, or publication operation is performed.
"""
from pathlib import Path
import argparse,hashlib,json,runpy,shutil,zipfile
p=argparse.ArgumentParser(description=__doc__)
for n in ('build','raw-build','raw-host','libwintandy','host','clock','native','out'):p.add_argument('--'+n,type=Path,required=True)
a=p.parse_args();here=Path(__file__).resolve().parent;repo=here.parents[2]
runpy.run_path(str(here.parent/'EFFECTS/LAUNCPIN.PY'))['validate'](here.parent/'EFFECTS')
out=a.out.resolve();out.mkdir();kit=out/'BEATSFX';kit.mkdir()
def pin(b):return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest())
build=json.loads((a.build/'BUILD.JSON').read_bytes())
raw_build=json.loads((a.raw_build/'BUILD.JSON').read_bytes())
raw_process=json.loads((a.raw_build/'PROCESS.JSON').read_bytes())
assert raw_process['owned_child_stopped'] and raw_process['lease_released'] and raw_process['return']==0
raw_host=json.loads((a.raw_host/'RESULT.JSON').read_bytes())
assert all(row['passed'] for row in raw_host['results'])
assert any(row['name']=='rebase' for row in raw_host['results'])
for path,wanted in raw_host['source_sha256'].items():assert hashlib.sha256((a.libwintandy/path).read_bytes()).hexdigest()==wanted,path
assert raw_host['source_sha256']['libtandywin/AUDIO/SOURCE/TIMECLK.C']==raw_build['sources']['TIMECLK.C']['sha256']
host=json.loads((a.host/'RESULT.JSON').read_bytes());native=json.loads((a.native/'RESULT.JSON').read_bytes())
clock=json.loads((a.clock/'RESULT.JSON').read_bytes())
audio=json.loads((a.native/'AUDIO.JSON').read_bytes())
assert host['status']==clock['status']==native['status']==audio['status']=='PASS'
assert native['playback_only'] is False
assert native['visual_only'] is False and native['trace_input'] is False
assert audio['runtime']==native['runtime']
assert audio['validator']==pin((here/'FXAUDIO.PY').read_bytes())
assert audio['native_result']==pin((a.native/'RESULT.JSON').read_bytes())
assert native['harness']==pin((here/'FXNATIVE.PY').read_bytes())
for name,wanted in audio['recordings'].items():assert pin((a.native/name).read_bytes())==wanted,name
assert native['runtime']==build['runtimes']['BEATS.EXE']
assert native['raw_runtime']==raw_build['runtime']==pin((a.raw_build/'RAWCLK.DLL').read_bytes())
assert native['owned_child_stopped'] and native['lease_released'] and native['inventory_after']=='0'
for name,relative in build['source_paths'].items():
 original=(repo/relative).read_bytes();staged=(a.build/name).read_bytes()
 assert pin(original)==build['source_originals'][name],name
 assert pin(staged)==build['sources'][name],name
 assert staged==original.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
 dest=kit/'SOURCE'/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(staged)
for name,wanted in raw_build['sources'].items():
 original=(repo/'experimental/WINLIB30/SOURCE'/name).read_bytes();staged=(a.raw_build/name).read_bytes()
 assert pin(original)==pin(staged)==wanted,name
 dest=kit/'SOURCE/experimental/WINLIB30/SOURCE'/name;dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists():assert dest.read_bytes()==staged,name
 else:dest.write_bytes(staged)
shutil.copyfile(repo/'experimental/WINLIB30/SOURCE/BLDRAW.PY',kit/'SOURCE/experimental/WINLIB30/SOURCE/BLDRAW.PY')
for evidence in (host,clock):
 for path,wanted in evidence['sources'].items():assert hashlib.sha256(Path(path).read_bytes()).hexdigest()==wanted,path
for name,wanted in build['runtimes'].items():
 data=(a.build/name).read_bytes();assert pin(data)==wanted,name;(kit/name).write_bytes(data)
release=json.loads((repo/'tools/WINAMP/RELEASE.JSN').read_bytes())
for name in ('TWXT2.ZIP','TWSETUP.EXE'):
 assert pin((repo/'artifacts/win30'/name).read_bytes())==release['assets'][name],name
with zipfile.ZipFile(repo/'artifacts/win30/TWXT2.ZIP') as z:
 for name in ('RAWCLK.DLL','TBRIDGE.EXE','RESERVE.COM'):(kit/name).write_bytes(z.read('TWXT2/'+name))
# Effects is independently published, outside the Winamp release ledger.
# Preserve the exact artifact tracked on current main2257b575.
assert pin((repo/'artifacts/win30/EFFECTS.ZIP').read_bytes())==dict(bytes=1061628,sha256='41992e5ba39d50a33bef7ae241f3c5679187d3919607b17fb43556656576e2e0')
with zipfile.ZipFile(repo/'artifacts/win30/EFFECTS.ZIP') as z:
 effects_manifest=json.loads(z.read('EFFECTS/MANIFEST.JSN'))
 assert effects_manifest['native']['status']=='PASS'
 for relative,wanted in effects_manifest['files'].items():assert pin(z.read('EFFECTS/'+relative))==wanted,relative
 for name in ('TBRIDGE.EXE','RESERVE.COM'):
  assert z.read('EFFECTS/'+name)==(kit/name).read_bytes(),name
 for name in ('EFFECTS.EXE','FXSTART.EXE','FXPLAY.H','MIT.TXT','COIN.FXP','COINSP.FXP','LASER.FXP','LASERSP.FXP','BOOM.FXP','BOOMSP.FXP','JUMP.FXP','JUMPSP.FXP','BLIP.FXP','BLIPSP.FXP'):
  (kit/name).write_bytes(z.read('EFFECTS/'+name))
 (kit/'EFFECTS.TXT').write_bytes(z.read('EFFECTS/README.TXT'))
 for name in z.namelist():
  if name.startswith('EFFECTS/SOURCE/LIBTANDY/'):
   dest=kit/name[len('EFFECTS/'):];dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(z.read(name))
(kit/'RAWCLK.DLL').write_bytes((a.raw_build/'RAWCLK.DLL').read_bytes())
for name in ('TWXT2.ZIP','TWSETUP.EXE'):shutil.copyfile(repo/'artifacts/win30'/name,kit/name)
for name in ('FXGROOVE.BTS',):shutil.copyfile(a.host/name,kit/name)
for name in ('README.MD','FXINTEG.MD','FXVALID.MD','REFERENCE.MD','BUILD.PY','FXTEST.PY','FXCLOCK.PY','FXNATIVE.PY','FXAUDIO.PY','AUDTEST.PY','FXPACK.PY'):
 dest=kit/'SOURCE/src/win30/BEATLAB'/name;dest.write_bytes((here/name).read_bytes())
for path in (here/'TESTS').iterdir():
 if path.is_file():(kit/'SOURCE/src/win30/BEATLAB/TESTS').mkdir(exist_ok=True);shutil.copyfile(path,kit/'SOURCE/src/win30/BEATLAB/TESTS'/path.name)
for relative in ('src/win30/LAUNCH/RUNDOS.PY','src/win30/LAUNCH/TPLAY.C','src/win30/DRIVER/PITCORE.H'):
 dest=kit/'SOURCE'/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes((repo/relative).read_bytes())
for path in (here.parent/'EFFECTS').iterdir():
 if path.is_file():
  dest=kit/'SOURCE/src/win30/EFFECTS'/path.name;dest.parent.mkdir(parents=True,exist_ok=True)
  if not dest.exists():dest.write_bytes(path.read_bytes())
for path in (here.parent/'FIDDLE').iterdir():
 if path.is_file():
  dest=kit/'SOURCE/src/win30/FIDDLE'/path.name;dest.parent.mkdir(parents=True,exist_ok=True)
  if not dest.exists():dest.write_bytes(path.read_bytes())
for relative in ('BEATMML/MIDIMAP.C','BEATMML/INSTR.C','BEATMML/INSTR.H','BEATMML/MIDIAPI.H','BEATMML/PERIODS.H','MMLFIDL/MMLCORE.C','MMLFIDL/MMLCORE.H'):
 source=repo/'experimental/PITPUBLIC/MML3AUTH/SOURCE/src/win30'/relative
 dest=kit/'SOURCE/src/win30'/relative;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(source.read_bytes())
shutil.copyfile(here/'FXINTEG.MD',kit/'DETAILS.TXT')
shutil.copyfile(repo/'LICENSE',kit/'LICENSE.TXT')
readme=(here/'README.MD').read_bytes().replace(b'[Integration and source details](FXINTEG.MD) - [Original demo reference](REFERENCE.MD)',b'More details: DETAILS.TXT. Original demo reference: REFDEMOS.TXT.')
(kit/'README.TXT').write_bytes(readme)
reference=(here/'REFERENCE.MD').read_text(encoding='utf-8')
reference=reference[reference.index('## The grid'):]
reference=reference.replace('requests supported by the existing shared SOUND service (D1 is 37 Hz).','frequencies in the normal-note table (D1 is37 Hz).')
reference='Original demos and normal-note reference\n\nRead README.TXT for current setup, imported patches, pause and the complete kit.\nThese notes describe normal-note patterns; FX steps use embedded BTS playback.\n\n'+reference
(kit/'REFDEMOS.TXT').write_bytes(reference.replace('\r\n','\n').replace('\n','\r\n').encode('utf-8'))
go=b'@ECHO OFF\r\nIF "%1"=="" GOTO DEFAULT\r\nBTSTART %1\r\nGOTO END\r\n:DEFAULT\r\nBTSTART C:\\WINDOWS\r\n:END\r\n'
(kit/'GO.BAT').write_bytes(go);(kit/'GOFX.BAT').write_bytes(go.replace(b'BTSTART',b'FXSTART'))
e=kit/'EVIDENCE';e.mkdir()
for path,name in ((a.build/'BUILD.JSON','BUILD.JSN'),(a.build/'PROCESS.JSON','BLDPROC.JSN'),(a.host/'RESULT.JSON','HOST.JSN'),(a.host/'TEST.LOG','HOST.LOG'),(a.clock/'RESULT.JSON','CLOCK.JSN'),(a.clock/'TEST.LOG','CLOCK.LOG'),(a.native/'RESULT.JSON','NATIVE.JSN'),(a.native/'AUDIO.JSON','AUDIO.JSN'),(a.native/'BEATS.LOG','NATIVE.LOG')):shutil.copyfile(path,e/name)
for path,name in ((a.raw_build/'BUILD.JSON','RAWBLD.JSN'),(a.raw_build/'PROCESS.JSON','RAWPROC.JSN')):shutil.copyfile(path,e/name)
shutil.copyfile(a.raw_host/'RESULT.JSON',e/'RAWTEST.JSN')
for name in ('EDITOR','PLAYING','PAUSED','STOPPED'):
 if (a.native/(name+'.PNG')).is_file():shutil.copyfile(a.native/(name+'.PNG'),e/('STOPREQ.PNG' if name=='STOPPED' else name+'.PNG'))
(e/'READ_ME.TXT').write_bytes(b'PNG frames are point-in-time observations. STOPREQ.PNG was captured shortly after Escape while the status repaint was pending; it is not a settled Stop-status screenshot. NATIVE.JSN and AUDIO.JSN contain the actual action-frame, silence and response-bound evidence. Capture analysis trims at most0.25s for buffering. Emulated hardware only; no physical-hardware or subjective listening claim.\r\n')
manifest={str(path.relative_to(kit)).replace('\\','/'):pin(path.read_bytes()) for path in kit.rglob('*') if path.is_file()}
(kit/'MANIFEST.JSN').write_text(json.dumps(dict(format='Private Beats/Effects integration v1',publication=False,files=manifest),indent=2))
archive=out/'BEATSFX.ZIP'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for path in sorted(kit.rglob('*')):
  if path.is_file():z.write(path,'BEATSFX/'+path.relative_to(kit).as_posix())
with zipfile.ZipFile(archive) as z:
 for name,wanted in manifest.items():assert pin(z.read('BEATSFX/'+name))==wanted,name
(out/'PACKAGE.JSON').write_text(json.dumps(dict(status='PASS',archive=pin(archive.read_bytes()),published=False,files=len(manifest)+1),indent=2))
print(archive)
