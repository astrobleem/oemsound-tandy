#!/usr/bin/env python3
"""Host-only logic checks. This does not build a Windows executable."""
import hashlib,json,pathlib,re,subprocess,sys
root=pathlib.Path(__file__).resolve().parent
source=pathlib.Path(sys.argv[1]).resolve()
s=source.read_text();original_sha=hashlib.sha256(source.read_bytes()).hexdigest()
s,n=re.subn(r'#include <windows.h>', '',s);assert n==1
s,n=re.subn(r'/\* Compact native prompt:.*?(?=static void psgByte)', 'static int confirmExit(HINSTANCE inst) { return MessageBox(inst, \"mock confirmation\", \"Exit\", MB_OKCANCEL); }\n',s,flags=re.S);assert n==1
s,n=re.subn(r'_asm\s*\{.*?\}', 'record_port(value);',s,flags=re.S);assert n==1
s,n=re.subn(r'\*\(unsigned char FAR \*\)0xfc000000UL', 'read_signature()',s);assert n==1
(root/'HOST_GENERATED.C').write_text((root/'HOST_MOCKS.H').read_text()+'\n'+s+'\n'+(root/'HOST_TESTS.C').read_text())
cmd=['gcc','-std=c89','-Wall','-Wextra','-pedantic','-Wno-unused-parameter','-x','c',str(root/'HOST_GENERATED.C'),'-o',str(root/'host-tests')]
r=subprocess.run(cmd,capture_output=True,text=True);(root/'HOST_BUILD.LOG').write_text(r.stdout+r.stderr);r.check_returncode()
r=subprocess.run([str(root/'host-tests')],capture_output=True,text=True);(root/'HOST_RESULT.TXT').write_text(r.stdout+r.stderr)
print(r.stdout+r.stderr,end='')
(root/'SCOPE.JSON').write_text(json.dumps({'source':str(source),'source_sha256':original_sha,'changed_production_source':False,'transformations':['Replace Windows header with host API mocks','Replace compact confirmation implementation with controlled confirmation result; native GUI checked separately','Replace inline OUT assembly with timestamped port-byte recorder','Replace real-mode BIOS hardware-signature dereference with controlled mock'],'scope':'Host-only branch and lifecycle checks. Preserves WinMain. Does not validate 16-bit code generation, Windows message delivery, PSG hardware settling, port behavior or wall-clock sound cutoff during cooperative task starvation.','result':r.returncode},indent=2)+'\n')
r.check_returncode()
