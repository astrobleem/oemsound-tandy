#!/usr/bin/env python3
"""Host-only bounded WZV2 conversion; no soundtrack or network operation."""
import argparse,hashlib,json,math,struct,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
from rgbi import read_exact,probe,filter_string,indexed_rgbi,pack_frame,unpack_frame
p=argparse.ArgumentParser();p.add_argument('input',type=Path);p.add_argument('output',type=Path)
p.add_argument('--width',type=int,default=256);p.add_argument('--height',type=int,default=160);p.add_argument('--fps',type=int,default=4)
a=p.parse_args();w,h,fps=a.width,a.height,a.fps
if (w,h,fps) not in [(256,160,4)]:p.error('Supported:256x160@4')
metadata=probe(a.input);duration=round(float(metadata['format']['duration'])*1000)
if not 0<duration<=600000:p.error('Duration outside0..600s')
expected=math.ceil(duration*fps/1000);framebytes=w*h//2
cmd=['ffmpeg','-hide_banner','-loglevel','error','-i',str(a.input),'-map','0:v:0','-vf',f'fps={fps},scale={w}:{h}:flags=area','-an','-sn','-dn','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
a.output.parent.mkdir(parents=True,exist_ok=True);proc=subprocess.Popen(cmd,stdout=subprocess.PIPE);samples=[];count=0;padded=0;last=None
try:
 with a.output.open('wb') as out:
  out.write(struct.pack('<4sHHHHIII',b'WZV2',w,h,fps,24,expected,duration,framebytes))
  while True:
   raw=read_exact(proc.stdout,w*h*3)
   if not raw:break
   if len(raw)!=w*h*3:raise ValueError('Short RGB frame')
   img=indexed_rgbi(Image.frombytes('RGB',(w,h),raw),'none');packed=pack_frame(img)
   assert unpack_frame(packed,w,h).tobytes()==img.tobytes()
   if count<expected:
    out.write(packed);last=packed
    if count%(fps*8)==0:samples.append((count,img.copy()))
   count+=1
  if proc.wait():raise RuntimeError('ffmpeg failed')
  if count<expected:
   if last is None or expected-count>2:raise ValueError('Unexpected missing frames')
   padded=expected-count;out.write(last*padded)
finally:
 if proc.poll() is None:proc.kill();proc.wait()
if count>expected+2:raise ValueError('Unexpected frame count')
assert a.output.stat().st_size==24+expected*framebytes
columns=8;zoom=2;tilew=w*zoom;tileh=h*zoom;rows=math.ceil(len(samples)/columns)
sheet=Image.new('RGB',(columns*(tilew+8)+8,rows*(tileh+20)+8),(24,24,28));draw=ImageDraw.Draw(sheet)
for i,(frame,img) in enumerate(samples):
 x=8+(i%columns)*(tilew+8);y=8+(i//columns)*(tileh+20);sheet.paste(img.convert('RGB').resize((tilew,tileh),Image.Resampling.NEAREST),(x,y));draw.text((x,y+tileh),f'{frame/fps:.0f}s',fill='white')
sheet.save(a.output.with_name(a.output.name+'.png'))
r={'source_sha256':hashlib.file_digest(a.input.open('rb'),'sha256').hexdigest(),'width':w,'height':h,'fps':fps,'duration_ms':duration,'frames':expected,'decoded_frames':count,'padded_final_frames':padded,'frame_bytes':framebytes,'bytes':a.output.stat().st_size,'sha256':hashlib.file_digest(a.output.open('rb'),'sha256').hexdigest(),'palette':'standardIBM/TandyRGBI','dither':'none','command':cmd,'roundtrip':'everyframevalidated','privacy':'privateuser-suppliedtransformation'}
a.output.with_name(a.output.name+'.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
