#!/usr/bin/env python3
"""Build the DOS-only 8086 executable; does not launch an emulator."""
from pathlib import Path
import argparse,os,subprocess,hashlib,json
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--watcom',type=Path,default=Path(os.environ.get('WATCOM',r'C:\WATCOM')));a=p.parse_args()
root=Path(__file__).resolve().parents[1];out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
wat=a.watcom.resolve();env=dict(os.environ,WATCOM=str(wat),INCLUDE=str(wat/'H'))
sources=[root/'SOURCE/DOSPLAY.C',root/'SOURCE/AUDIO.C',root/'libtandy/src/lt_tick_dos.c',root/'libtandy/src/lt_chip.c']
with (out/'BUILD.LOG').open('wb') as log:
 for src in sources:
  subprocess.run([str(wat/'BINNT/WCC.EXE'),'-0','-ms','-os','-s','-w3','-ecc','-i='+str(wat/'H'),'-i='+str(root/'SOURCE'),'-i='+str(root/'libtandy/include'),str(src)],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run([str(wat/'BINNT/WASM.EXE'),'-0','-ms',str(root/'SOURCE/HARDWARE.ASM')],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 (out/'DOSPLAY.LNK').write_text('system dos\nname DOSPLAY.EXE\noption map=DOSPLAY.MAP\noption stack=4096\nfile DOSPLAY.obj,AUDIO.obj,lt_tick_dos.obj,lt_chip.obj,HARDWARE.obj\n')
 subprocess.run([str(wat/'BINNT/WLINK.EXE'),'@DOSPLAY.LNK'],cwd=out,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
exe=out/'DOSPLAY.EXE';r=dict(bytes=exe.stat().st_size,sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),compiler='Open Watcom C16 1.9 -0 -ms -os -s -w3 -ecc',application_stack=4096,emulator_started=False)
(out/'BUILD-RESULT.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
