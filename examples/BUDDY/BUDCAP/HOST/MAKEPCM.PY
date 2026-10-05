from pathlib import Path
import subprocess,struct,json,hashlib,argparse
p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('output',type=Path);a=p.parse_args();src=a.input;r=a.output;r.mkdir(parents=True,exist_ok=True)
wins=[(23900,24800),(28500,31950),(219950,224700),(225150,226650)]
out=bytearray(struct.pack('<4sHH',b'SPC1',len(wins),0));manifest=[]
for i,(start,end) in enumerate(wins):
 raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(src),'-vn','-ac','1','-ar','6000','-af',f'highpass=f=150,lowpass=f=2700,aresample=6000,atrim=start_sample={start*6}:end_sample={end*6}','-f','u8','-'])
 b=bytes(1+round(x*71/255) for x in raw)
 assert len(b)==(end-start)*6 and len(b)<=48000
 out+=struct.pack('<IIHH',start,end,len(b),0)+b;manifest.append({'index':i,'start_ms':start,'end_ms':end,'samples':len(b),'sha256':hashlib.sha256(b).hexdigest()})
(r/'DIALOG.PCM').write_bytes(out);(r/'CLIPS.JSON').write_text(json.dumps(manifest,indent=2));print('bytes',len(out),'sample bytes',sum(x['samples'] for x in manifest))
