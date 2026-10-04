#!/usr/bin/env python3
"""Fault-stub checks of exact current Windows helper/service source, no emulator."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];source=(root/'source/TINYVID.C').read_text()
def block(index):
 start=source.index('{',index);depth=1;end=start+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
def fn(name):
 i=source.index('static ',source.index(name)-30);return source[i:source.index('{',i)]+block(i)
def run(name,text):
 with tempfile.TemporaryDirectory(prefix='tinywin-test-') as d:
  p=Path(d)/'test.c';p.write_text(text);subprocess.run(['cc','-std=c99','-O0',str(p),'-o',str(Path(d)/'test')],check=True);subprocess.run([str(Path(d)/'test')],check=True)
 print(name,'PASS')
common='#include <stdint.h>\n#include <assert.h>\ntypedef uint32_t DWORD; typedef int32_t LONG; typedef unsigned short WORD; typedef unsigned char BYTE;\n'
release=common+'''
#define MODM_RESET 9
#define MODM_CLOSE 4
#define MIDI_COOKIE 0x544d
static int soundOpen,lastError,calls;static DWORD cleanupError,rreset,rclose;
static DWORD midi(WORD id,WORD m,DWORD u,DWORD p1,DWORD p2){calls++;return m==9?rreset:rclose;}
'''+fn('releaseSound(void)')+'''
int main(void){int a,b;for(a=0;a<2;a++)for(b=0;b<2;b++){
 soundOpen=1;lastError=0;calls=0;cleanupError=0;rreset=a;rclose=b;releaseSound();
 assert(calls==2&&!soundOpen);assert(cleanupError==(unsigned)(a+2*b));assert(lastError==((a||b)?38:0));}
 soundOpen=1;lastError=15;cleanupError=0;rreset=rclose=1;releaseSound();assert(lastError==15);
 calls=0;soundOpen=0;releaseSound();assert(!calls);return 0;}
''';run('cleanup failure reporting and close attempt',release)
gdi=common+'''
#define FRAME_BYTES 1536
#define WIDTH 64
#define HEIGHT 48
#define VIDEO_HEADER 24
#define DIB_RGB_COLORS 0
#define SRCCOPY 0
#define FAR
#define NULL ((void*)0)
typedef int HDC;typedef int HBITMAP;typedef int BITMAPINFO;
static int mode=2,videoFile,mainWindow,memoryDC,bitmap=2,oldBitmap=1,dibInfo;
static DWORD videoFrames=32,lastFrame=0xffffffffUL,droppedFrames,maxVideoRead,maxDraw,renderedFrames;
static int hadFrame,error,fault,selections;static BYTE pixels[1536],bottomUp[1536];
static DWORD GetTickCount(void){return 100;}
static LONG _llseek(int f,LONG off,int origin){return fault==1?-1:off;}
static int _lread(int f,BYTE *p,int n){return fault==2?n-1:n;}
static HDC GetDC(int w){return fault==3?0:1;}
static int ReleaseDC(int w,HDC dc){return 1;}
static int SelectObject(int dc,int obj){selections++;if(fault==4&&selections==1)return 0;if(fault==5&&selections==2)return 0;return selections==1?bitmap:oldBitmap;}
static int SetDIBits(int a,int b,int c,int d,BYTE *p,BITMAPINFO *i,int j){return fault==6?47:48;}
static int BitBlt(int a,int b,int c,int d,int e,int f,int g,int h,int i){return fault==7?0:1;}
static int fail(int code,int music){error=code;return 0;}
'''+fn('updateVideo(DWORD elapsed)')+'''
int main(void){int n;for(n=1;n<=7;n++){fault=n;selections=error=0;lastFrame=0xffffffffUL;renderedFrames=0;assert(!updateVideo(100));assert(error&&!renderedFrames);}
 fault=0;selections=error=0;lastFrame=0xffffffffUL;assert(updateVideo(100));assert(renderedFrames==1&&lastFrame==0&&hadFrame);
 videoFrames=966;selections=0;lastFrame=0xffffffffUL;assert(updateVideo(240000));assert(lastFrame==960);return 0;}
''';run('GDI, partial DIB, seek and read failures',gdi)
serviceblock=block(source.index('if(playing) {',source.index('long FAR PASCAL WndProc')))
service=common+'''
static DWORD now,elapsed,gap,lastTick,maxGap,startTick,timelineBase,playEnd;
static DWORD fakeClock,readDelay;static int musicCalls,videoCalls,stops,playing;
static DWORD GetTickCount(void){return fakeClock;}
static int updateMusic(DWORD e){musicCalls++;return 1;}
static int updateVideo(DWORD e){videoCalls++;fakeClock+=readDelay;return 1;}
static void stop(char *s){stops++;playing=0;}
static long service(void)
'''+serviceblock+'''
int main(void){startTick=timelineBase=0;playEnd=200;
 fakeClock=150;readDelay=100;musicCalls=videoCalls=stops=0;playing=1;service();assert(musicCalls==1&&videoCalls==1&&stops==1&&!playing);
 fakeClock=200;readDelay=0;musicCalls=videoCalls=stops=0;playing=1;service();assert(!musicCalls&&!videoCalls&&stops==1);
 fakeClock=100;readDelay=20;musicCalls=videoCalls=stops=0;playing=1;service();assert(musicCalls==2&&videoCalls==1&&!stops);return 0;}
''';run('cue end before and across video I/O',service)
start=source.index('if(automatic&&!started&&');end=source.index('if(playing) {',start);autocode=source[start:end]
auto=common+'''
static int automatic=1,started,fullPlayback,window=3,active,focus,iconic,plays;
static int GetActiveWindow(void){return active;}static int GetFocus(void){return focus;}static int IsIconic(int w){return iconic;}
static void play(int full){plays++;}
static void attempt(void){
'''+autocode+'''
}
int main(void){active=3;focus=0;attempt();assert(!plays&&!started);focus=3;iconic=1;attempt();assert(!plays&&!started);iconic=0;attempt();assert(plays==1&&started);attempt();assert(plays==1);return 0;}
''';run('automatic play requires focus and visible window',auto)
print('Four exact-source host fault suites passed. These are stubs, not injected Win16 hardware/API faults.')
