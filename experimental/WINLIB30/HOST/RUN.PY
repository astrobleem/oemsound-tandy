"""Run actual R4 source host cases; no sanitizer/runtime or native claim."""
from pathlib import Path
import os, subprocess, json, hashlib
r=Path(__file__).resolve().parents[1]
out=Path(__file__).resolve().parent
cc=Path(r"C:\msys64\ucrt64\bin\cc.exe")
env=os.environ.copy(); env["PATH"]=str(cc.parent)+os.pathsep+env.get("PATH","")
cases=[
 ("raw-format",["-std=c89","TESTS/host.c","SOURCE/RAWFILE.C","SOURCE/TESTCORE.C"]),
 ("midi-transport",["-std=c99","-DSYNTH_HOST","-I","TESTS/mdhost","-I","SOURCE","TESTS/mdhost/driver.c","SOURCE/MIDISYN.C","SOURCE/INSTR.C"]),
 ("synth",["-std=c99","-DSYNTH_HOST","-I","SOURCE","TESTS/synth_host.c","SOURCE/MIDISYN.C","SOURCE/INSTR.C"]),
 ("live",["-std=c99","-DSYNTH_HOST","-I","TESTS/mdhost","-I","SOURCE","TESTS/live_host.c","SOURCE/MIDISYN.C","SOURCE/INSTR.C"]),
 ("beat",["-std=c99","TESTS/beat_host.c"]),
]
results=[]
for name,args in cases:
 exe=out/(name+".exe")
 cmd=[str(cc),"-x","c",*args,"-o",str(exe)]
 compiled=subprocess.run(cmd,cwd=r,env=env,capture_output=True,text=True,timeout=30)
 log=compiled.stdout+compiled.stderr
 result={"name":name,"compile":compiled.returncode,"passed":False}
 if not compiled.returncode:
  ran=subprocess.run([str(exe)],cwd=r,env=env,capture_output=True,text=True,timeout=10)
  log+="\n"+ran.stdout+ran.stderr
  result.update(exit_code=ran.returncode,passed=ran.returncode==0,output=ran.stdout.strip(),exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest())
 (out/(name+"-plain.log")).write_text(log)
 results.append(result); print(name,result.get("output",log[-500:]))
record={"compiler":"MSYS2 UCRT64 GCC16.1.0","sanitizer":"not available: initial links failed missing libubsan; these runs are unsanitized","scope":"actual foreground/provider source with fake raw hardware; not native Windows/IRQ/physical timing","results":results}
(out/"PLAIN.JSON").write_text(json.dumps(record,indent=2))
assert all(x["passed"] for x in results)
