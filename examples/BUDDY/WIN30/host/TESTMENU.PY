"""Focused exact-source menu-entry regression; does not launch Windows/audio."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
p.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[1]/'source/TINYVID.C')
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=False);source=a.source.read_text()
def fn(name):
 m=re.search(r'^static void '+name+r'\([^\n]*\)\n\{.*?^\}',source,re.M|re.S)
 assert m,name
 return m.group()
# Execute the unchanged event cases from the real WndProc switch. The mouse
# path sends WM_INITMENU without WM_SYSKEYDOWN/focus/activation changes.
dispatch=source[source.index('    case WM_SIZE:'):source.index('    case WM_PAINT:')]
c=r'''#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint32_t DWORD;
#define TRUE 1
#define BQ_OK 0
#define MODM_RESET 9
#define MODM_CLOSE 4
#define MIDI_COOKIE 0x544d
#define WM_SIZE 5
#define WM_KILLFOCUS 8
#define WM_SYSKEYDOWN 0x104
#define WM_ACTIVATE 6
#define WM_INITMENU 0x116
#define SIZE_MINIMIZED 1
static int playing,paused,finished,soundOpen,speaker,serviceOpened,speakerNote,queueReady,queue;
static int lastError,serviceUsed,mainWindow,gate,owner,timer,muted,closes,stops,reports,defaultCalls,stopFault;
static int active=1,focus=1;
static DWORD timelineBase=35652,playEnd=200365,lastElapsed,resumePosition,trace[16],lastFrame,renderedFrames,droppedFrames;
static DWORD appliedEvents,skippedEvents,finishTick,cleanupError,position=61423;
static char status[40];
static DWORD movieNow(void){return position;}
static int clockStop(void){stops++;gate=0;if(stopFault)return 1;owner=timer=0;return 0;}
static unsigned clockDebug(int n){return 0;}
static int BqPause(int*q,DWORD*p){*p=position;return 0;}
static int BqStop(int*q){return 0;}
static int midi(int a,int m,DWORD u,DWORD x,DWORD y){if(m==MODM_RESET){gate=0;muted++;}else{owner=0;closes++;}return 0;}
static void speakerSilence(void){gate=0;muted++;}
static void CloseSound(void){owner=0;closes++;}
static int inp(int p){return gate;}
static void closeStreams(void){}
static DWORD GetTickCount(void){return 1000;}
static char *lstrcpy(char*a,const char*b){return strcpy(a,b);}
static void report(char*r,DWORD e){reports++;}
static void InvalidateRect(int w,void*p,int e){}
static int fail(int c,int m){lastError=c;return 0;}
'''+fn('releaseSound')+'\n'+fn('stop')+'\n'+fn('pausePlayback')+r'''
static int message(unsigned message,unsigned wp){switch(message){
'''+dispatch+r'''
}defaultCalls++;return 77;}
static void reset(int device){
 playing=soundOpen=owner=gate=1;paused=finished=0;serviceOpened=device<2;
 speaker=device!=3;timer=serviceOpened;queueReady=device==3;serviceUsed=serviceOpened;
 stops=muted=closes=reports=defaultCalls=stopFault=lastError=cleanupError=0;
}
int main(void){int d;for(d=0;d<4;d++){
 reset(d);assert(message(WM_INITMENU,42)==0);
 assert(active==1&&focus==1); /* no focus or keyboard message precedes entry */
 assert(!playing&&paused&&resumePosition==position&&!soundOpen&&!serviceOpened);
 assert(!owner&&!timer&&!gate&&reports==1&&!defaultCalls);
 assert(message(WM_INITMENU,42)==0&&reports==1); /* already paused */
 assert(message(0x117,0)==77&&!playing&&paused&&reports==1); /* dismiss: no restart */
 reset(d);message(WM_SYSKEYDOWN,32);message(WM_INITMENU,42);
 assert(!playing&&paused&&!owner&&!timer&&!gate&&reports==1); /* keyboard is idempotent */
}
 reset(0);stopFault=1;message(WM_INITMENU,42);
 assert(!playing&&paused&&!gate&&owner&&timer&&soundOpen&&serviceOpened&&cleanupError==4);
 assert(stops==1); /* failed release retains live owner/module for safe retry */
 playing=paused=0;soundOpen=serviceOpened=0;reports=0;
 assert(message(WM_INITMENU,42)==0&&!playing&&!paused&&!reports);
 return 0;}
'''
f=a.out/'MENU.C';f.write_text(c);exe=a.out.resolve()/'MENU.EXE'
cc=([sys.executable,str(a.cc.resolve()),'--']if a.cc.suffix.lower()=='.py'else[str(a.cc.resolve())])
r=subprocess.run(cc+['-x','c','-std=c89','-O0',str(f.resolve()),'-o',str(exe)],capture_output=True,text=True)
(a.out/'BUILD.LOG').write_text(r.stdout+r.stderr);r.check_returncode()
r=subprocess.run([str(exe)],capture_output=True,text=True);(a.out/'TEST.LOG').write_text(r.stdout+r.stderr)
result=dict(passed=r.returncode==0,source_sha256=hashlib.sha256(a.source.read_bytes()).hexdigest(),cases=10,scope='Host exact WndProc event cases plus actual pause/stop/release functions; modeled OS/timer/mapper calls, not native menu ABI',emulator_started=False,playback_started=False)
(a.out/'RESULT.JSON').write_text(json.dumps(result,indent=2)+'\n');r.check_returncode();print('PASS: mouse/keyboard menu pause, release and no automatic restart')
