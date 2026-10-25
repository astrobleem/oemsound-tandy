#define WINVER 0x0300
#include <windows.h>
#include "TIMECLK.H"
#include "TCBURST.H"
#include "FMTCORE.H"
#include "GUNZIP.H"
#include "FMTAPP.H"
/* Policy bound on one plain stream (VGM bytes, later also inflated VGZ):
   it limits the preflight scan and, for VGZ, temp-disk use. Offsets are DWORD
   and _llseek takes a signed LONG, so nothing in the path is 16-bit. */
#define FMT_MAX_PLAIN 16777216L
typedef WORD (FAR PASCAL *CALL0)(void);
typedef WORD (FAR PASCAL *CALL1)(WORD);
typedef DWORD (FAR PASCAL *CALLD)(void);
typedef WORD (FAR PASCAL *CALLCAP)(WORD,TCBINFO FAR *,WORD);
typedef WORD (FAR PASCAL *CALLPUT)(TCBURST FAR *);
static HMODULE module;static CALL0 openClock,startClock,endClock,closeClock,resumeClock,pauseClock,unpauseClock;
static CALL1 debugClock;static CALLD readClock;static CALLPUT putClock;static CALLCAP getCaps;
static char dll[144],reason[144]="Ready";static WORD saved[29],paused,ended,pending;
static HTASK owner;static HFILE file=HFILE_ERROR,tfile=HFILE_ERROR;static char tname[144];static OFSTRUCT tof;
static DWORD inputSize,sourceSize,cacheAt=0xffffffffUL;
static WORD cacheCount;static BYTE cache[512];static FXCORE core;static FXBURST next;
static TCBURST wire;static TCBINFO caps;
static WORD lastAdmission,minQueued;
static WORD opening,cancelled,progressKind;
static DWORD sinkDone,sinkTotal;static FMTPROGRESS progress;
void FmtSetProgress(FMTPROGRESS p){progress=p;}
static WORD notify(WORD phase,DWORD done,DWORD total)
{
 if(cancelled)return 0;
 if(progress&&!progress(phase,done,total)){cancelled=1;return 0;}
 return !cancelled;
}
static DWORD refusedAt,refusedClock,refusedSource;static WORD refusedQueue,refusedGroups;
static void setReason(const char *s){WORD i=0;while(i<sizeof(reason)-1&&s[i]){reason[i]=s[i];i++;}reason[i]=0;}
static int fileRead(void *u,unsigned long at,unsigned char *out,unsigned short n)
{
 WORD i,take;DWORD base;(void)u;if(at>inputSize||n>inputSize-at)return 0;
 while(n){base=at&~511UL;if(cacheAt!=base){
  if(opening&&progressKind==FMT_CHECK&&!notify(FMT_CHECK,base,inputSize))return 0;
  if(_llseek(file,(LONG)base,0)!=(LONG)base)return 0;
  take=inputSize-base>512UL?512:(WORD)(inputSize-base);
  cacheCount=_lread(file,cache,take);if(cacheCount!=take)return 0;cacheAt=base;
 }i=(WORD)(at-cacheAt);take=cacheCount-i;if(take>n)take=n;if(!take)return 0;
 while(take--){*out++=cache[i++];at++;n--;}
 }return 1;
}
static int readSource(void *u,unsigned long at,unsigned char *out,unsigned short n)
{
 (void)u;return fileRead(0,at,out,n);
}
/* VGZ is inflated once, before sound is acquired, into an automatically named
   DOS temp file that FmtClose deletes. The player then reads it exactly like
   a plain VGM (random access through the 512-byte cache, so the replay after
   preflight and the loop jump cost no re-inflation). */
static int spool(void *u,const GZ8 GZ_FAR *p,GZ16 n)
{
 (void)u;if(!notify(FMT_INFLATE,sinkDone+n,sinkTotal))return 0;
 sinkDone+=n;return _lwrite(tfile,p,n)==n;
}
void FmtInit(LPSTR directory){lstrcpy(dll,directory);lstrcat(dll,"RAWCLK.DLL");}
static WORD load(void){
 module=LoadLibrary(dll);if(module<32){module=0;return 1;}
 getCaps=(CALLCAP)GetProcAddress(module,(LPSTR)20L);openClock=(CALL0)GetProcAddress(module,(LPSTR)21L);
 putClock=(CALLPUT)GetProcAddress(module,(LPSTR)22L);endClock=(CALL0)GetProcAddress(module,(LPSTR)23L);
 startClock=(CALL0)GetProcAddress(module,(LPSTR)3L);closeClock=(CALL0)GetProcAddress(module,(LPSTR)5L);
 debugClock=(CALL1)GetProcAddress(module,(LPSTR)6L);resumeClock=(CALL0)GetProcAddress(module,(LPSTR)11L);
 readClock=(CALLD)GetProcAddress(module,(LPSTR)12L);pauseClock=(CALL0)GetProcAddress(module,(LPSTR)14L);unpauseClock=(CALL0)GetProcAddress(module,(LPSTR)15L);
 if(!getCaps||!openClock||!putClock||!endClock||!startClock||!closeClock||!debugClock||!resumeClock||!readClock||!pauseClock||!unpauseClock)return 1;
 if(getCaps(TCB_VERSION,&caps,sizeof(caps))||caps.size!=sizeof(caps)||caps.version!=TCB_VERSION||caps.maxWrites!=16||caps.capacity!=TCB_CAPACITY||caps.minCycles!=FX_MIN||caps.features!=(TCB_ORDERED|TCB_RAW_NOISE)||caps.clockNumerator!=3579545UL||caps.clockDenominator!=3)return 1;
 return 0;
}
WORD FmtClose(void){WORD i;
 /* A foreground callback may request cancellation, but cannot free buffers
    while the decoder is still on its caller stack. */
 if(opening){cancelled=1;return 1;}
 if(owner&&owner!=GetCurrentTask())return 1;
 if(owner){if(closeClock())return 1;owner=0;}
 if(file!=HFILE_ERROR){_lclose(file);file=HFILE_ERROR;}
 if(tfile!=HFILE_ERROR){_lclose(tfile);tfile=HFILE_ERROR;}
 if(tname[0]){if(OpenFile(tname,&tof,OF_DELETE)==HFILE_ERROR){
  setReason("Cannot remove temporary file");return 1;}tname[0]=0;}
 if(module){if(debugClock)for(i=0;i<29;i++)saved[i]=debugClock(i);FreeLibrary(module);module=0;}
 paused=ended=pending=0;return 0;
}
static WORD fill(WORD budget){int r;WORD q,i;
 while(budget--&&!ended){if(!pending){r=FXNext(&core,&next);if(r<0){setReason(FXReason(&core));return 3;}
   if(!r){if(endClock())return 3;ended=1;return 0;}pending=1;
  }
  wire.at=next.at;wire.count=next.count;for(i=0;i<next.count;i++)wire.data[i]=next.data[i];
  q=putClock(&wire);lastAdmission=q;if(q==2)return 0;if(q){refusedAt=wire.at;refusedClock=readClock();refusedQueue=debugClock(0);refusedSource=core.sourceOffset;refusedGroups=(WORD)core.groups;lstrcpy(reason,q==4?"Playback buffer ran late":"Queue refused input");return 3;}pending=0;
 }return 0;
}
static WORD rejected(WORD code)
{
 if(cancelled){setReason("Cancelled");code=FMT_CANCELLED;}
 opening=progressKind=0;
 if(FmtClose())return 1;return code;
}
WORD FmtOpen(LPSTR path,WORD kind)
{
 LONG size;WORD r;DWORD plainSize;HGLOBAL ring;GZ8 GZ_FAR *window;
 static BYTE magic[2],tail[4];
 if(opening)return 1;if(FmtClose())return 1;
 opening=1;cancelled=progressKind=0;lstrcpy(reason,"Input rejected");
 file=_lopen(path,OF_READ|OF_SHARE_DENY_WRITE);
 if(file==HFILE_ERROR){setReason("Cannot open file");return rejected(4);}
 size=_llseek(file,0L,2);cacheAt=0xffffffffUL;
 inputSize=(DWORD)size;sourceSize=inputSize;
 if(size<18||size>FMT_MAX_PLAIN){setReason("File outside bounded profile");return rejected(2);}
 if(!fileRead(0,0,magic,2))return rejected(4);
 if(magic[0]==31&&magic[1]==139){
  if(kind!=FX_VGM){setReason("Compressed input is VGZ only");return rejected(2);}
  if(!fileRead(0,inputSize-4,tail,4))return rejected(4);
  sinkDone=0;sinkTotal=(DWORD)tail[0]|((DWORD)tail[1]<<8)|
      ((DWORD)tail[2]<<16)|((DWORD)tail[3]<<24);
  if(!GetTempFileName(0,"VGZ",0,tname)){
   tname[0]=0;setReason("Cannot create temporary file");return rejected(6);}
  tfile=_lcreat(tname,0);
  if(tfile==HFILE_ERROR){setReason("Cannot create VGZ temp file");return rejected(6);}
  ring=GlobalAlloc(GMEM_MOVEABLE,(DWORD)GZ_WINDOW);
  if(!ring||(window=GlobalLock(ring))==0){
   if(ring)GlobalFree(ring);setReason("Not enough memory to open VGZ");return rejected(5);}
  progressKind=FMT_INFLATE;
  r=gz_decode_stream(fileRead,0,inputSize,window,spool,0,
      (DWORD)FMT_MAX_PLAIN,&plainSize);
  GlobalUnlock(ring);GlobalFree(ring);progressKind=0;
  if(r){setReason(r==5?"VGZ temporary file write failed":
      r==2?"VGZ larger than bounded profile":r==4?"VGZ CRC or size check failed":
      "VGZ integrity or bounded decode refusal");return rejected(r==5?6:2);}
  _lclose(file);file=tfile;tfile=HFILE_ERROR;cacheAt=0xffffffffUL;
  size=_llseek(file,0L,2);
  if(size<18||(DWORD)size!=plainSize){setReason("VGZ temp file inconsistent");return rejected(6);}
  inputSize=sourceSize=(DWORD)size;
 }
 /* All progress callbacks precede sound ownership. No pumping or I/O in IRQ. */
 progressKind=FMT_CHECK;
 if(!notify(FMT_CHECK,0,sourceSize))return rejected(FMT_CANCELLED);
 r=FXPreflight(&core,readSource,0,sourceSize,kind);
 progressKind=0;
 if(r){setReason(FXReason(&core));return rejected(2);}
 if(!notify(FMT_CHECK,sourceSize,sourceSize))return rejected(FMT_CANCELLED);
 if(FXOpen(&core,readSource,0,sourceSize,kind)){
  setReason(FXReason(&core));return rejected(2);}
 if(load()){setReason("Format clock capability unavailable");return rejected(8);}
 r=openClock();if(r){setReason("Sound is busy or unavailable");return rejected(7);}
 owner=GetCurrentTask();if(!owner){closeClock();return rejected(7);}
 ended=pending=paused=lastAdmission=0;refusedAt=refusedClock=refusedSource=0;
 refusedQueue=refusedGroups=0;minQueued=TCB_CAPACITY;
 if(fill(TCB_CAPACITY)||startClock())return rejected(3);
 opening=0;setReason(kind==FX_MOD?"MOD PSG":"VGM PSG");return 0;
}
WORD FmtPump(void){WORD n;if(!owner||owner!=GetCurrentTask())return 2;
 if(debugClock(6)){lstrcpy(reason,"Playback timing lost");return 3;}if(!paused&&debugClock(1)&&!debugClock(20))resumeClock();
 if(debugClock(7))return 0;n=debugClock(0);if(!ended&&n<minQueued)minQueued=n;if(!paused&&fill(TCB_CAPACITY))return 3;return 1;
}
WORD FmtPause(void){WORD r;if(!owner||owner!=GetCurrentTask())return 1;
 r=paused?unpauseClock():pauseClock();if(!r)paused=!paused;return r;
}
DWORD FmtPosition(void){DWORD n=owner?readClock():0;return (n/3579545UL)*3000UL+((n%3579545UL)*3UL)/3580UL;}
WORD FmtDebug(WORD i){if(i==42)return (WORD)(core.groups>>16);if(i==43)return (WORD)(core.sourceOffset>>16);if(i==44)return (WORD)(refusedSource>>16);if(i==35)return (WORD)refusedAt;if(i==36)return (WORD)(refusedAt>>16);if(i==37)return (WORD)refusedClock;if(i==38)return (WORD)(refusedClock>>16);if(i==39)return refusedQueue;if(i==40)return (WORD)refusedSource;if(i==41)return refusedGroups;if(i==33)return lastAdmission;if(i==34)return minQueued;if(i==30)return module&&debugClock?debugClock(27):saved[27];if(i==31)return module&&debugClock?debugClock(28):saved[28];if(i==32)return owner?(WORD)readClock():0;if(i==27)return paused;if(i==28)return core.error;if(i==29)return (WORD)core.groups;return i<27?(module&&debugClock?debugClock(i):saved[i]):0;}
char *FmtReason(void){return reason;}
