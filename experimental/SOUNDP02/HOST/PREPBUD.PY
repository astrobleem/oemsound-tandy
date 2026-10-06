from pathlib import Path
import argparse,hashlib,zipfile,shutil
p=argparse.ArgumentParser(description='Prepare a NEW Windows Buddy folder from the exact original WINPLAY archive.')
p.add_argument('baseline_zip',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
if a.output.exists():p.error('Output must be a new directory; existing installations are never overwritten.')
if hashlib.sha256(a.baseline_zip.read_bytes()).hexdigest()!='be318264ff102c5e113ed536f8d90fe6543a735c90c800f7ce48b0dad94b8e70':p.error('Expected the original repository WINPLAY.ZIP; archive identity differs.')
root=Path(__file__).resolve().parents[1]
with zipfile.ZipFile(a.baseline_zip) as z:
 media={n:z.read(n) for n in ['WEEZER.WZV','WEEZER.WZM','WEEZER.CUE']}
a.output.mkdir(parents=True)
for n,b in media.items():(a.output/n).write_bytes(b)
for n in ['TINYVID.EXE','MIDIMAP.DRV']:shutil.copyfile(root/'RUNTIME/BUDDY'/n,a.output/n)
shutil.copyfile(root/'ATTRIB.TXT',a.output/'ATTRIB.TXT')
print('Prepared new folder. Run TINYVID.EXE in Windows 3.0 real mode. Space starts the song; Enter starts full video; Escape stops.')
