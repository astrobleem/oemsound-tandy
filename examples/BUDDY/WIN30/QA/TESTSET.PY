"""Host tests of the exact DOS installer C, with private Windows filesystem inputs.
Native DOS qualification is separate. All fixtures and failed outputs retained.
"""
import argparse,json,os,shutil,subprocess,sys,uuid
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--package',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=False);root=Path(__file__).resolve().parents[1]
for n in ['BUDSET.C']:shutil.copyfile(root/'setup'/n,a.out/n)
shutil.copyfile(a.package/'SETBUILD/PAYLOAD.H',a.out/'PAYLOAD.H')
compiler=([sys.executable,str(a.cc.resolve()),'--']if a.cc.suffix.lower()=='.py'else[str(a.cc.resolve())])
(a.out/'FAULT.C').write_text('''#include <stdio.h>
static size_t BrokenWrite(const void*p,size_t s,size_t n,FILE*f){if(n>1)return fwrite(p,s,n-1,f);return 0;}
#define fwrite BrokenWrite
#include "BUDSET.C"
''')
for stem in ['BUDSET','FAULT']:
 r=subprocess.run(compiler+['-x','c','-std=c89',str((a.out/(stem+'.C')).resolve()),'-o',str((a.out/(stem+'.EXE')).resolve())],capture_output=True,text=True)
 (a.out/(stem+'-BUILD.LOG')).write_text(r.stdout+r.stderr);r.check_returncode()
fixture=Path(os.environ['TEMP'])/('BW'+uuid.uuid4().hex[:6]);fixture.mkdir(exist_ok=False)
source=fixture/'SOURCE';shutil.copytree(a.package/'BUDWIN',source);target=fixture/'INST';rows=[]
def run(stem,args,want):
 r=subprocess.run([str((a.out/(stem+'.EXE')).resolve()),*map(str,args)],cwd=source,capture_output=True,text=True)
 rows.append(dict(exe=stem,args=list(map(str,args)),exit=r.returncode,expected=want,output=r.stdout+r.stderr));assert r.returncode==want,rows[-1]
run('BUDSET',[target],0)
manifest=json.loads((a.package/'PACKAGE.JSON').read_text())['installer_manifest']
assert set(p.name for p in target.iterdir())==set(manifest)
for n in manifest:assert(target/n).read_bytes()==(source/n).read_bytes(),n
run('BUDSET',['/CHECK',target],0)
(target/'USER.DAT').write_bytes(b'preserve this file');run('BUDSET',[target],4);assert(target/'USER.DAT').read_bytes()==b'preserve this file'
(source/'ATTRIB.TXT').write_bytes(b'corrupt');bad=fixture/'BAD'
run('BUDSET',[bad],3);assert not bad.exists()
shutil.copyfile(a.package/'BUDWIN/ATTRIB.TXT',source/'ATTRIB.TXT')
run('FAULT',[bad],5);assert not bad.exists(),'Failed install did not roll back its new directory'
for invalid in ['C:\\','C:\\BAD\\..\\X','C:\\TOOLONG99','relative','C:\\BAD.EXT','C:\\A B']:
 run('BUDSET',[invalid],2)
(target/'SPEAKER.WZM').write_bytes(b'modified user file');run('BUDSET',['/CHECK',target],3)
assert(target/'SPEAKER.WZM').read_bytes()==b'modified user file'
result=dict(passed=True,checks=len(rows),fixture=str(fixture),files_retained=True,scope='Actual installer C on host Windows; does not establish DOS ABI',cases=rows)
(a.out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');print('PASS',len(rows),'installer cases; source CRC, new target, exact copy, no overwrite, rollback and modified-file preservation')
