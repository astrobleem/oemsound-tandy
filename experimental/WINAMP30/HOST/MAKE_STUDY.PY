"""Original 61.44-second three-voice study; no imported musical material."""
from pathlib import Path
import gzip,hashlib,json,math,struct,sys
root=Path(__file__).resolve().parent.parent
out=Path(sys.argv[1]) if len(sys.argv)>1 else root/'INPUTS';out.mkdir(parents=True,exist_ok=True)
# Original short cells, varied by harmony and register over 32 bars.
cells=[[0,4,7,4,2,5,9,5],[7,4,2,0,4,7,11,7],
       [0,2,4,7,9,7,4,2],[5,4,2,0,2,4,7,0]]
roots=[0,5,9,7,0,5,2,7]
score=[]
for bar in range(32):
    r=roots[bar%8];minor=bar%8==2
    chord=[0,3 if minor else 4,7,12]
    cell=cells[(bar//8)%4]
    for row in range(16):
        melody=72+cell[row//2]
        if bar>=16 and row>=8:melody-=12
        score.append((melody,48+r,60+r+chord[(row//4)%4]))
def frequency(note):return 440.0*2.0**((note-69)/12.0)
def period(note):return int(7093789/(64*frequency(note))+0.5)
def psg(note):return int(3579545/(32*frequency(note))+0.5)
body=bytearray();last=[None]*3
for notes in score:
    for channel,note in enumerate(notes):
        p=psg(note)
        if p!=last[channel]:
            body.extend((0x50,0x80|(channel<<5)|(p&15),0x50,p>>4))
            body.extend((0x50,0x90|(channel<<5)|[3,6,8][channel]))
            last[channel]=p
    body.extend((0x61,0xac,0x14)) # 5292 samples =120ms
for channel in range(4):body.extend((0x50,0x9f|(channel<<5)))
body.extend((0x61,0x3a,0x11,0x66)) #100ms silent tail
vgm=bytearray(0x40);vgm[:4]=b'Vgm '
struct.pack_into('<I',vgm,0x04,0x40+len(body)-4)
struct.pack_into('<I',vgm,0x08,0x150)
struct.pack_into('<I',vgm,0x0c,3579545)
struct.pack_into('<I',vgm,0x18,len(score)*5292+4410)
struct.pack_into('<H',vgm,0x28,6);vgm[0x2a]=15
struct.pack_into('<I',vgm,0x34,12)
vgm+=body;(out/'COPPER.VGM').write_bytes(vgm)
(out/'COPPER.VGZ').write_bytes(gzip.compress(bytes(vgm),mtime=0))
mod=bytearray(1084+8*1024);mod[:20]=b'Copperlight study'.ljust(20,b'\0')
for n,volume in enumerate((52,36,28)):
    at=20+n*30;mod[at:at+22]=('PSG voice%d'%(n+1)).encode().ljust(22,b'\0')
    mod[at+25]=volume
mod[950]=8;mod[951]=127;mod[952:960]=bytes(range(8));mod[1080:1084]=b'M.K.'
for row,notes in enumerate(score):
    for channel,note in enumerate(notes):
        p=period(note);instrument=channel+1;at=1084+row*16+channel*4
        mod[at:at+4]=bytes(((instrument&0xf0)|(p>>8),p&255,(instrument&15)<<4,0))
(out/'COPPER.MOD').write_bytes(mod)
def vlq(value):
    b=[value&127];value>>=7
    while value:b.append(128|(value&127));value>>=7
    return bytes(reversed(b))
events=[]
for channel in range(3):events.append((0,bytes((0xc0|channel,16))))
last=[None]*3
for row,notes in enumerate(score):
    for channel,note in enumerate(notes):
        if note==last[channel]:continue
        if last[channel] is not None:events.append((row*24,bytes((0x80|channel,last[channel],0))))
        events.append((row*24,bytes((0x90|channel,note,[100,80,60][channel]))))
        last[channel]=note
for channel,note in enumerate(last):events.append((len(score)*24,bytes((0x80|channel,note,0))))
events.sort(key=lambda item:item[0])
track=bytearray(b'\x00\xff\x51\x03\x07\xa1\x20');tick=0
for at,event in events:track+=vlq(at-tick)+event;tick=at
track+=b'\x00\xff\x2f\x00'
midi=b'MThd'+struct.pack('>IHHH',6,0,1,100)+b'MTrk'+struct.pack('>I',len(track))+track
(out/'COPPER.MID').write_bytes(midi)
rows=[{'path':p.name,'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(out.iterdir())]
(out/'STUDY.JSON').write_text(json.dumps({'title':'Copperlight study','original':True,'imported_media':False,'nominal_music_ms':61440,'files':rows},indent=2)+'\n')
print(json.dumps(rows,indent=2))
