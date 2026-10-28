"""Exact-source Buddy fault tests. Host stubs do not prove native Win16 ABI."""
from pathlib import Path
import argparse,re,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
a.out.mkdir(parents=True,exist_ok=False);source=(Path(__file__).resolve().parents[1]/'source/TINYVID.C').read_text()
def fn(name):
 m=re.search(r'^static (?:void|int) '+name+r'\([^\n]*\)\n\{.*?^\}',source,re.M|re.S)
 assert m,name
 return m.group()
def run(name,text):
 f=a.out/(name+'.C');f.write_text(text);exe=a.out.resolve()/(name+'.EXE')
 cc=([sys.executable,str(a.cc.resolve()),'--'] if a.cc.suffix.lower()=='.py' else [str(a.cc.resolve())])
 r=subprocess.run(cc+['-x','c','-std=c89','-O0',str(f.resolve()),'-o',str(exe)],capture_output=True,text=True)
 (a.out/(name+'-BUILD.LOG')).write_text(r.stdout+r.stderr);r.check_returncode()
 r=subprocess.run([str(exe)],capture_output=True,text=True);(a.out/(name+'-TEST.LOG')).write_text(r.stdout+r.stderr);r.check_returncode();print(name,'PASS')
common='''#include <stdint.h>
#include <assert.h>
#include <string.h>
typedef uint32_t DWORD;typedef int32_t LONG;typedef unsigned short WORD;typedef unsigned char BYTE;
'''
release=common+'''
#define MODM_RESET 9
#define MODM_CLOSE 4
#define MIDI_COOKIE 0x544d
#define BQ_OK 0
static int soundOpen,lastError,calls,speaker,serviceOpened,speakerNote,queueReady,queue;
static DWORD cleanupError,rreset,rclose,stopFail;
static DWORD midi(WORD id,WORD m,DWORD u,DWORD p1,DWORD p2){calls++;return m==9?rreset:rclose;}
static WORD clockStop(void){calls++;return (WORD)stopFail;}
static int BqStop(int *q){return 0;}
static void speakerSilence(void){calls++;}
static void CloseSound(void){calls++;}
'''+fn('releaseSound')+'''
int main(void){int a,b;for(a=0;a<2;a++)for(b=0;b<2;b++){
 soundOpen=1;lastError=0;calls=0;cleanupError=0;rreset=a;rclose=b;releaseSound();
 assert(calls==2&&soundOpen==b);assert(cleanupError==(unsigned)(a+2*b));assert(lastError==((a||b)?38:0));}
 serviceOpened=soundOpen=1;stopFail=1;calls=cleanupError=0;releaseSound();
 assert(calls==1&&cleanupError==4&&lastError==51&&serviceOpened&&soundOpen);
 stopFail=0;releaseSound();assert(!serviceOpened&&!soundOpen);
 speaker=1;soundOpen=1;calls=0;releaseSound();assert(calls==2&&!soundOpen);return 0;}
'''
run('CLEANUP',release)
gdi=common+'''
#define FRAME_BYTES 1536
#define WIDTH 64
#define HEIGHT 48
#define VIDEO_HEADER 24
#define DIB_RGB_COLORS 0
#define SRCCOPY 0
#define FAR
typedef int HDC;typedef int BITMAPINFO;
static int mode=2,videoFile,mainWindow,memoryDC,bitmap=2,oldBitmap=1,dibInfo;
static DWORD videoFrames=966,lastFrame=0xffffffffUL,droppedFrames,maxVideoRead,maxDraw,renderedFrames;
static DWORD maxVideoAge,totalVideoAge,maxVideoEarly,totalVideoEarly,videoLead,playEnd=241400;
static int hadFrame,error,fault,selections,seekCalls,planarVideo,bitmapDirect=1,bitmapBottomUp,planeMap[4],videoX=2,videoY=2;
static unsigned bitmapRowStep,bitmapPlaneStep;
static BYTE pixels[1536],bottomUp[1536];
static DWORD GetTickCount(void){return 100;}
static DWORD movieNow(void){return 100;}
static LONG _llseek(int f,LONG off,int origin){seekCalls++;return fault==1?-1:off;}
static int _lread(int f,BYTE *p,int n){return fault==2?n-1:n;}
static HDC GetDC(int w){return fault==3?0:1;}
static int ReleaseDC(int w,HDC dc){return 1;}
static int SelectObject(int dc,int obj){selections++;if(fault==4&&selections==1)return 0;if(fault==5&&selections==2)return 0;return selections==1?bitmap:oldBitmap;}
static int SetDIBits(int a,int b,int c,int d,BYTE *p,BITMAPINFO *i,int j){return fault==6?47:48;}
static LONG SetBitmapBits(int b,LONG n,BYTE *p){return fault==6?n-1:n;}
static int BitBlt(int a,int b,int c,int d,int e,int f,int g,int h,int i){return fault==7?0:1;}
static int fail(int code,int music){error=code;return 0;}
'''+fn('updateVideo')+'''
int main(void){int n,p;for(p=0;p<2;p++){planarVideo=p;
 for(n=1;n<=7;n++){fault=n;selections=error=0;lastFrame=0xffffffffUL;renderedFrames=0;assert(!updateVideo(100));assert(error&&!renderedFrames);}
 fault=0;selections=error=seekCalls=0;lastFrame=0xffffffffUL;assert(updateVideo(100));assert(renderedFrames==1&&lastFrame==0&&hadFrame&&seekCalls==1);
 selections=0;assert(updateVideo(250));assert(lastFrame==1&&seekCalls==1);
 selections=0;lastFrame=0xffffffffUL;assert(updateVideo(240000));assert(lastFrame==960&&seekCalls==2);
 }return 0;}
'''
run('VIDEOIO',gdi)
start=source.index('if(automatic&&!started&&');end=source.index('if(playing) {',start);autocode=source[start:end]
run('AUTO',common+'''
static int automatic=1,started,fullPlayback,window=3,active,focus,iconic,plays;
static int GetActiveWindow(void){return active;}static int GetFocus(void){return focus;}static int IsIconic(int w){return iconic;}
static void play(int full){plays++;}
static void attempt(void){
'''+autocode+'''
}
int main(void){active=3;focus=0;attempt();assert(!plays&&!started);focus=3;iconic=1;attempt();assert(!plays&&!started);iconic=0;attempt();assert(plays==1&&started);attempt();assert(plays==1);return 0;}
''')

paused=common+"""
#define HFILE_ERROR -1
#define BQ_OK 0
#define MODM_RESET 9
#define MIDI_COOKIE 1
#define TRUE 1
typedef int HFILE;
typedef struct {DWORD when;BYTE note[4],velocity[4],retrigger;} MUSICSTATE;
static int playing,fullPlayback,soundOpen,lastError,finished,paused=1,queueBlocked,usedQueue,serviceUsed,seekPaused=1;
static int queueReady,queue,queueOps,mode=3,serviceOpened,speaker,mainWindow,nextValid,bufferCount,bufferIndex;
static DWORD resumePosition=61423,timelineBase,playEnd,lastElapsed,videoFrames,eventCount,duration,recordsRead,previousEvent,lastFrame;
static DWORD maxVideoAge,totalVideoAge,maxVideoEarly,totalVideoEarly,videoLead,maxGap,maxLateness,totalLateness,appliedEvents,skippedEvents;
static DWORD renderedFrames,droppedFrames,maxVideoRead,maxMusicRead,maxDraw,musicFailures,videoFailures,cleanupError,startTick,lastTick;
static DWORD trace[16],videoAt;
static MUSICSTATE current;
static char status[40];
static int acquired,applied,started,preparedCalls,reported;
static void releaseSound(void){assert(!soundOpen);}
static void closeStreams(void){}
static char *lstrcpy(char*a,const char*b){return strcpy(a,b);}
static int openVideo(void){duration=241400;return 1;}
static int openMusic(void){return 1;}
static int openCues(void){timelineBase=35652;playEnd=200365;return 1;}
static int acquireSound(void){acquired++;return 1;}
static void stop(char*s){assert(0);}
static int musicAt(DWORD t,MUSICSTATE*s){s->when=t;return 1;}
static int prepareSpeaker(MUSICSTATE*s){preparedCalls++;return 1;}
static int BqStart(int*q,int*o,DWORD t,DWORD e,MUSICSTATE*s){applied++;return 0;}
static int midi(int a,int b,int c,DWORD d,DWORD e){applied++;return 0;}
static int applyState(MUSICSTATE*s){applied++;return 1;}
static void pathFor(char*p,char*n){}
static HFILE _lcreat(char*p,int a){return -1;}
static void _lclose(HFILE f){}
static DWORD GetTickCount(void){return 100;}
static int clockStart(void){started++;return 0;}
static int fail(int a,int b){return 0;}
static int updateMusic(DWORD t){applied++;return 1;}
static DWORD movieNow(void){return timelineBase;}
static int updateVideo(DWORD t){videoAt=t;return 1;}
static void InvalidateRect(int h,void*p,int e){}
static void UpdateWindow(int h){}
static int GetActiveWindow(void){return mainWindow;}
static int GetFocus(void){return mainWindow;}
static int IsIconic(int h){return 0;}
static void report(char*s,DWORD t){reported++;assert(t==0);}
"""+fn('play')+"""
int main(void){play(0);assert(!acquired&&!applied&&!started&&!preparedCalls&&reported==1);
 assert(paused&&!playing&&resumePosition==61423&&timelineBase==61423&&videoAt==61423);
 assert(trace[1]==2&&trace[14]==61423&&!soundOpen);return 0;}
"""
run('PAUSESEEK',paused)



selector=common+"""
#define FAR
#define WF_PMODE 1
#define SOUND_AUTO 0
#define SOUND_SPEAKER 1
#define SOUND_TANDY 2
#define OF_READ 0
#define HFILE_ERROR -1
#define MODM_GETDEVCAPS 2
typedef int HFILE;
typedef struct {unsigned wVoices,vDriverVersion;} TCAPS;
typedef DWORD (*MIDIPROC)(unsigned,unsigned,DWORD,TCAPS*,DWORD);
typedef unsigned (*DEBUGPROC)(unsigned);
typedef void (*FARPROC)(void);
static int speaker,soundChoice,module,hostTandy,pmode,loads;
static MIDIPROC midi;static DEBUGPROC debug;
static unsigned GetWinFlags(void){return pmode;}
static void pathFor(char*p,char*n){strcpy(p,n);}
static HFILE _lopen(char*p,unsigned f){return 1;}
static void _lclose(HFILE f){}
static int LoadLibrary(char*p){loads++;return 40;}
static DWORD Caps(unsigned a,unsigned b,DWORD c,TCAPS*p,DWORD e){p->wVoices=3;p->vDriverVersion=0x102;return 0;}
static FARPROC GetProcAddress(int m,char*n){return strcmp(n,"MIDIMESSAGE")?NULL:(FARPROC)Caps;}
"""+fn('chooseSound').replace('*(BYTE FAR *)0xfc000000UL','hostTandy').replace('(DWORD)(LPVOID)&caps','&caps')+"""
int main(void){soundChoice=SOUND_AUTO;chooseSound();assert(speaker&&!loads);
 hostTandy=0x21;chooseSound();assert(!speaker&&loads==1);
 hostTandy=0;soundChoice=SOUND_TANDY;chooseSound();assert(!speaker);
 soundChoice=SOUND_SPEAKER;hostTandy=0x21;chooseSound();assert(speaker);
 soundChoice=SOUND_AUTO;pmode=1;chooseSound();assert(speaker);return 0;}
"""
run('SELECTOR',selector)


startup=paused.rsplit('int main(void)',1)[0]
startup=startup.replace('static int acquired,applied,started,preparedCalls,reported;','static int acquired,applied,started,preparedCalls,reported,displayReady,firstPicture,serviceCase=1,focusGood=1;')
startup=startup.replace('static void releaseSound(void){assert(!soundOpen);}','static void releaseSound(void){soundOpen=serviceOpened=0;}')
startup=startup.replace('acquired++;return 1;','acquired++;soundOpen=1;serviceOpened=serviceCase;return 1;')
startup=startup.replace('static int clockStart(void){started++;return 0;}','static int clockStart(void){assert(displayReady);started++;return 0;}')
startup=startup.replace('static int applyState(MUSICSTATE*s){applied++;return 1;}','static int applyState(MUSICSTATE*s){assert(displayReady);applied++;return 1;}')
startup=startup.replace('static int midi(int a,int b,int c,DWORD d,DWORD e){applied++;return 0;}','static int midi(int a,int b,int c,DWORD d,DWORD e){assert(displayReady);applied++;return 0;}')
startup=startup.replace('static int updateVideo(DWORD t){videoAt=t;return 1;}','static int updateVideo(DWORD t){assert(!started);firstPicture=1;videoAt=t;return 1;}')
startup=startup.replace('static void UpdateWindow(int h){}','static void UpdateWindow(int h){assert(firstPicture);displayReady=1;}')
startup=startup.replace('static int GetFocus(void){return mainWindow;}','static int GetFocus(void){return focusGood?mainWindow:mainWindow+1;}')
startup+="""
int main(void){
 playing=paused=seekPaused=0;speaker=1;play(0);
 assert(playing&&acquired==1&&preparedCalls==1&&displayReady&&firstPicture&&started==1);
 playing=paused=soundOpen=serviceOpened=displayReady=firstPicture=started=applied=preparedCalls=acquired=0;
 serviceCase=0;speaker=0;play(0);
 assert(playing&&!preparedCalls&&!started&&applied>=2&&displayReady);
 playing=paused=soundOpen=serviceOpened=displayReady=firstPicture=started=applied=preparedCalls=acquired=0;
 serviceCase=1;speaker=1;focusGood=0;play(0);
 assert(!playing&&paused&&!soundOpen&&!serviceOpened&&!started&&preparedCalls==1);return 0;}
"""
run('STARTORDER',startup)
print('PASS: six exact-source host suites including first-picture-before-audio ordering')
