#!/usr/bin/env python3
"""Small negative fixtures for host validation and ISA-audit regressions."""
from pathlib import Path
import argparse,struct,subprocess,sys,json
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args();out=a.out.resolve();out.mkdir(parents=True,exist_ok=False);host=Path(__file__).resolve().parent
results={}
for name,code,valid in [('8086',bytes.fromhex('b80000c3'),True),('imul386',bytes.fromhex('0fafc3c3'),False),('nearje386',bytes.fromhex('0f840000c3'),False),('push186',bytes.fromhex('680000c3'),False),('operand386',bytes.fromhex('66b800000000c3'),False)]:
 exe=out/(name+'.EXE');size=32+len(code);h=[0x5a4d,size,1,0,2,0,0,0,0,0,0,0,0,0]
 exe.write_bytes(struct.pack('<14H',*h)+b'\0'*4+code);exe.with_suffix('.MAP').write_text('_TEXT CODE AUTO 0000:0000 %08x\n0000:0000 main_\n'%len(code))
 r=subprocess.run([sys.executable,str(host/'AUDIT.PY'),str(exe)],capture_output=True,text=True)
 (out/(name+'.json')).write_text(r.stdout+r.stderr);assert (r.returncode==0)==valid,(name,r.stdout,r.stderr);results[name]=dict(accepted=r.returncode==0,expected=valid)
duration=10000;row=lambda t:struct.pack('<I4B4BBB',t,0,0,0,0,0,0,0,0,0,0)
for name,count,valid in [('valid',1,True),('empty',0,False),('oversized',10001,False)]:
 d=out/name;d.mkdir();(d/'WEEZER.WZV').write_bytes(struct.pack('<4sHHHHIII',b'WZV2',64,48,4,24,40,duration,1536)+bytes(40*1536))
 (d/'WEEZER.WZM').write_bytes(struct.pack('<4sHHIII',b'WZM1',20,14,1,duration,0)+row(0));(d/'WEEZER.CUE').write_bytes(struct.pack('<4sIII',b'WZC1',duration,0,duration))
 (d/'SPEAKER.WZM').write_bytes(struct.pack('<4sHHIII',b'WZM1',20,14,count,duration,0)+b''.join(row(i) for i in range(count)))
 r=subprocess.run([sys.executable,str(host/'VALIDATE.PY'),str(d)],capture_output=True,text=True);(out/(name+'.log')).write_text(r.stdout+r.stderr)
 assert (r.returncode==0)==valid,(name,r.stdout,r.stderr);results[name]=dict(accepted=r.returncode==0,expected=valid)
(out/'TOOLS-CHECK.json').write_text(json.dumps(results,indent=2));print(json.dumps(results,indent=2))
