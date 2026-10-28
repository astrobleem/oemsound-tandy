from pathlib import Path
import subprocess,os,struct,json,re,hashlib,argparse,sys,zipfile
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='gcc');p.add_argument('--launcher',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];out=a.out.resolve();out.mkdir(parents=True,exist_ok=False)
source=r/'SOURCE';tests=r/'HOST/tests';library=r/'libtandy'
compiler=[sys.executable,str(a.launcher),'--'] if a.launcher else [a.cc]
cmd=compiler+['-x','c','-std=c89','-Wall','-Wextra','-Werror','-I'+str(tests),'-I'+str(source),'-I'+str(library/'include'),str(tests/'backend.c'),str(source/'AUDIO.C'),str(library/'src/lt_chip.c'),'-o',str(out/'backend.exe')]
with zipfile.ZipFile(r.parent/'DOSPLAY.ZIP') as z:
 for name in ['WEEZER.WZM','SPEAKER.WZM']: (out/name).write_bytes(z.read(name))
env=dict(os.environ)
if os.name=='nt':env['PATH']=r'C:\msys64\ucrt64\bin;'+env.get('PATH','')
subprocess.run(cmd,cwd=r,check=True)
def records(name):
 b=(out/name).read_bytes();assert b[:4]==b'WZM1'
 hs,rs,count,duration,reserved=struct.unpack_from('<HHIII',b,4)
 assert hs==20 and rs==14 and reserved==0 and len(b)==20+14*count
 rows=[struct.unpack_from('<I4B4BBB',b,20+14*i) for i in range(count)]
 assert all(a[0]<b[0] for a,b in zip(rows,rows[1:]));return rows,duration
psg,duration=records('WEEZER.WZM');speaker,d2=records('SPEAKER.WZM');assert duration==d2
for backend,name,output in [('P','WEEZER.WZM','psg.bin'),('S','SPEAKER.WZM','speaker.csv'),('N','WEEZER.WZM','silent.bin')]:
 subprocess.run([str(out/'backend.exe'),backend,str(out/name),str(out/output)],cwd=r,env=env,check=True)
periods=list(map(int,re.search(r'\{(.*?)\}',(source/'PERIODS.H').read_text(),re.S)[1].replace('\n','').split(',')))
expected=bytearray([0x9f,0xbf,0xdf,0xff]);previous=[0]*8
for row in psg:
 n=row[1:5];v=row[5:9];retrigger=row[9]
 for i in range(4):
  if previous[i]==n[i] and previous[4+i]==v[i] and not (retrigger&(1<<i)):continue
  expected.append(0x9f|(i<<5))
  if not n[i]:continue
  atten=14-((v[i]-1)*14)//126
  if i<3:
   period=periods[n[i]-45];expected.extend([0x80|(i<<5)|(period&15),period>>4])
  else:expected.append(0xe2 if n[i] in [35,36] else 0xe5 if n[i] in [38,40] else 0xe4 if n[i] in [42,44,46] else 0xe6)
  expected.append(0x90|(i<<5)|atten)
 previous=list(n+v)
expected.extend([0x9f,0xbf,0xdf,0xff]);assert expected==(out/'psg.bin').read_bytes()
assert all(row[1]==row[3]==row[4]==0 and row[9]&~2==0 for row in speaker)
assert all(24<=row[2]<=96 for row in speaker if row[2])
ports=[tuple(map(int,line.split(','))) for line in (out/'speaker.csv').read_text().splitlines()]
assert all(port in [0x61,0x42,0x43] for port,value in ports)
assert all(value&0xfc==0xa4 for port,value in ports if port==0x61)
assert ports[-1]==(0x61,0xa4)
assert (out/'silent.bin').read_bytes()==b''
# Integer-clock model for each queued state over complete source duration.
late=[];now=frac=index=0
while now<duration:
 now+=8;frac+=397544
 if frac>=1193182:now+=1;frac-=1193182
 consumed=0
 while index<len(speaker) and speaker[index][0]<=now and consumed<8:
  late.append(now-speaker[index][0]);index+=1;consumed+=1
assert max(late)<=9 and index==len(speaker)
report=dict(status='HOST_PASS',psg_states=len(psg),psg_bytes_identical=len(expected),speaker_states=len(speaker),speaker_ports_only=[hex(x) for x in sorted(set(p for p,v in ports))],upper_port61_bits_preserved=True,silent_backend_writes=0,modeled_max_late_ms=max(late),library_sources={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (r/'libtandy/src').iterdir()})
(out/'HOST.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
