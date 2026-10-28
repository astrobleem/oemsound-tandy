"""Make a fresh verified local Windows runtime package; never publish it."""
import argparse,hashlib,json,os,re,shutil,subprocess,zipfile,zlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build',type=Path,required=True);p.add_argument('--watcom',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parent;out=a.out.resolve();wc=a.watcom.resolve()
if out.exists():p.error('--out must be new')
receipt=json.loads((a.build/'RESULT.JSON').read_text());assert receipt['pass_build']
out.mkdir(parents=True);runtime=out/'BUDWIN';runtime.mkdir();manifest={}
for n,d in receipt['binaries'].items():
 f=a.build/'RUNTIME'/n;assert hashlib.sha256(f.read_bytes()).hexdigest()==d['sha256'];shutil.copyfile(f,runtime/n)
for n in ['WEEZER.WZV','WEEZER.WZP','WEEZER.WZM','SPEAKER.WZM','WEEZER.CUE','CGA16.BMP']:shutil.copyfile(root/'media'/n,runtime/n)
shutil.copyfile(root.parents[2]/'LICENSE',runtime/'LICENSE.TXT');shutil.copyfile(root/'README.TXT',runtime/'README.TXT');
for n in ['LIBWT.TXT','NOTICES.TXT']:shutil.copyfile(root/n,runtime/n)
shutil.copyfile(root.parent/'ATTRIB.TXT',runtime/'ATTRIB.TXT')
for f in sorted(runtime.iterdir()):
 assert re.fullmatch(r'[A-Z0-9_]{1,8}\.[A-Z0-9_]{1,3}',f.name),f.name
 b=f.read_bytes();manifest[f.name]=dict(bytes=len(b),crc32=zlib.crc32(b),sha256=hashlib.sha256(b).hexdigest())
build=out/'SETBUILD';build.mkdir()
for n in ['BUDSET.C','BUDSET.LNK']:shutil.copyfile(root/'setup'/n,build/n)
crc=[]
for i in range(256):
 c=i
 for _ in range(8):c=(c>>1)^(0xedb88320 if c&1 else 0)
 crc.append(c)
header='static const unsigned long crcTable[256]={\n'+','.join('0x%08xUL'%c for c in crc)+'};\n'
header+='typedef struct {char name[13];unsigned long bytes,crc;} PAYLOAD;\n'
header+='static const PAYLOAD payload[]={\n'+''.join('{"%s",%dUL,0x%08xUL},\n'%(n,d['bytes'],d['crc32'])for n,d in manifest.items())+'};\n'
header+='#define PAYLOAD_COUNT %d\n'%len(manifest);(build/'PAYLOAD.H').write_text(header)
env=dict(os.environ,WATCOM=str(wc),INCLUDE=str(wc/'H'))
logs=[]
for cmd in [[str(wc/'BINNT/wcc.exe'),'-bt=dos','-0','-ms','-os','-s','-w3','BUDSET.C'],[str(wc/'BINNT/wlink.exe'),'@BUDSET.LNK']]:
 r=subprocess.run(cmd,cwd=build,env=env,capture_output=True,text=True);logs.append(r.stdout+r.stderr)
 (out/'SETBUILD.LOG').write_text('\n'.join(logs));r.check_returncode()
shutil.copyfile(build/'BUDSET.EXE',runtime/'BUDSET.EXE')
files={}
for f in sorted(runtime.iterdir()):files[f.name]=dict(bytes=f.stat().st_size,sha256=hashlib.sha256(f.read_bytes()).hexdigest())
(runtime/'SHA256.TXT').write_bytes(''.join(d['sha256']+'  '+n+'\r\n'for n,d in files.items()).encode())
def zipfiles(path,files):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
  for name,f in sorted(files):
   info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3
   info.external_attr=0o100644<<16
   z.writestr(info,f.read_bytes(),compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
zipfiles(out/'WINPLAY.ZIP',[(f.name,f)for f in runtime.iterdir()])
repo=root.parents[2]
source=[(f.relative_to(repo).as_posix(),f)for f in root.rglob('*')if f.is_file() and '__pycache__'not in f.parts and f.suffix.lower()!='.pyc']
source +=[(name,repo/name)for name in ['LICENSE','examples/BUDDY/ATTRIB.TXT','experimental/WINLIB30/SOURCE/WINHOOK.ASM','experimental/WINLIB30/SOURCE/TBRIDGE.ASM']]
zipfiles(out/'SOURCE.ZIP',source)
result=dict(local_candidate=True,published=False,installer_manifest=manifest,files=files,zip_sha256=hashlib.sha256((out/'WINPLAY.ZIP').read_bytes()).hexdigest(),zip_bytes=(out/'WINPLAY.ZIP').stat().st_size,source_zip_sha256=hashlib.sha256((out/'SOURCE.ZIP').read_bytes()).hexdigest(),source_zip_bytes=(out/'SOURCE.ZIP').stat().st_size)
(out/'PACKAGE.JSON').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(files=len(files),zip_bytes=result['zip_bytes'],zip_sha256=result['zip_sha256'])))
