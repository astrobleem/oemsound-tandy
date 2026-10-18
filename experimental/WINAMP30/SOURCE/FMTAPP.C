#define WINVER 0x0300
#include <windows.h>
#include "TIMECLK.H"
#include "TCBURST.H"
#include "FMTCORE.H"
#include "GUNZIP.H"
#include "FMTAPP.H"
typedef WORD (FAR PASCAL *CALL0)(void);
typedef WORD (FAR PASCAL *CALL1)(WORD);
typedef DWORD (FAR PASCAL *CALLD)(void);
typedef WORD (FAR PASCAL *CALLCAP)(WORD,TCBINFO FAR *,WORD);
typedef WORD (FAR PASCAL *CALLPUT)(TCBURST FAR *);
static HMODULE module;static CALL0 openClock,startClock,endClock,closeClock,resumeClock,pauseClock,unpauseClock;
static CALL1 debugClock;static CALLD readClock;static CALLPUT putClock;static CALLCAP getCaps;
static char dll[144],reason[144]="Ready";static WORD saved[29],paused,ended,pending;
static HTASK owner;static HFILE file=HFILE_ERROR;static HGLOBAL unpacked;
static BYTE FAR *memory;static DWORD inputSize,sourceSize,cacheAt=0xffffffffUL;
static WORD cacheCount;static BYTE cache[512];static FXCORE core;static FXBURST next;
static TCBURST wire;static TCBINFO caps;
static WORD lastAdmission,minQueued;
static DWORD refusedAt,refusedClock,refusedSource;static WORD refusedQueue,refusedGroups;
static void setReason(const char *s){WORD i=0;while(i<sizeof(reason)-1&&s[i]){reason[i]=s[i];i++;}reason[i]=0;}
static int fileRead(void *u,unsigned long at,unsigned char *out,unsigned short n)
{
 WORD i,take;DWORD base;(void)u;if(at>inputSize||n>inputSize-at)return 0;
 while(n){base=at&~511UL;if(cacheAt!=base){
  if(_llseek(file,(LONG)base,0)!=(LONG)base)return 0;
  take=inputSize-base>512UL?512:(WORD)(inputSize-base);
  cacheCount=_lread(file,cache,take);if(cacheCount!=take)return 0;cacheAt=base;
 }i=(WORD)(at-cacheAt);take=cacheCount-i;if(take>n)take=n;if(!take)return 0;
 while(take--){*out++=cache[i++];at++;n--;}
 }return 1;
}
static int readSource(void *u,unsigned long at,unsigned char *out,unsigned short n)
{
 (void)u;if(!memory)return fileRead(0,at,out,n);
 if(at>sourceSize||n>sourceSize-at)return 0;while(n--)*out++=memory[(WORD)at++];return 1;
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
 if(owner&&owner!=GetCurrentTask())return 1;
 if(owner){if(closeClock())return 1;owner=0;}
 if(unpacked){GlobalUnlock(unpacked);if(GlobalFree(unpacked)){memory=GlobalLock(unpacked);return 1;}unpacked=0;memory=0;}
 if(file!=HFILE_ERROR){_lclose(file);file=HFILE_ERROR;}
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
WORD FmtOpen(LPSTR path,WORD kind){LONG size;WORD r,plainSize;static BYTE magic[2];
 if(FmtClose())return 1;lstrcpy(reason,"Input rejected");
 file=_lopen(path,OF_READ|OF_SHARE_DENY_WRITE);if(file==HFILE_ERROR){lstrcpy(reason,"Cannot open file");return 4;}
 size=_llseek(file,0L,2);cacheAt=0xffffffffUL;inputSize=(DWORD)size;sourceSize=inputSize;
 if(size<18||size>4194304L){lstrcpy(reason,"File outside bounded profile");FmtClose();return 2;}
 if(!fileRead(0,0,magic,2)){FmtClose();return 4;}
 if(magic[0]==31&&magic[1]==139){
  if(kind!=FX_VGM||inputSize>60000UL){lstrcpy(reason,"Compressed input too large");FmtClose();return 2;}
  unpacked=GlobalAlloc(GMEM_MOVEABLE,60000UL);if(!unpacked||(memory=GlobalLock(unpacked))==0){FmtClose();return 5;}
  r=gz_decode(fileRead,0,inputSize,memory,60000,&plainSize);
  if(r){lstrcpy(reason,"VGZ integrity or bounded decode refusal");FmtClose();return 2;}sourceSize=plainSize;
 }else if(kind==FX_VGM&&inputSize>60000UL){lstrcpy(reason,"VGM exceeds 60000 bytes");FmtClose();return 2;}
 /* Complete source scan and replay initialization precede SOUND ownership. */
 if(FXPreflight(&core,readSource,0,sourceSize,kind)||FXOpen(&core,readSource,0,sourceSize,kind)){
  setReason(FXReason(&core));FmtClose();return 2;
 }
 if(load()){lstrcpy(reason,"Format clock capability unavailable");FmtClose();return 8;}
 r=openClock();if(r){lstrcpy(reason,"Sound is busy or unavailable");FmtClose();return 7;}owner=GetCurrentTask();
 if(!owner){closeClock();FmtClose();return 7;}
 ended=pending=paused=lastAdmission=0;refusedAt=refusedClock=refusedSource=0;refusedQueue=refusedGroups=0;minQueued=TCB_CAPACITY;if(fill(TCB_CAPACITY)||startClock()){FmtClose();return 3;}
 lstrcpy(reason,kind==FX_MOD?"MOD PSG":"VGM PSG");return 0;
}
WORD FmtPump(void){WORD n;if(!owner||owner!=GetCurrentTask())return 2;
 if(debugClock(6)){lstrcpy(reason,"Playback timing lost");return 3;}if(!paused&&debugClock(1)&&!debugClock(20))resumeClock();
 if(debugClock(7))return 0;n=debugClock(0);if(!ended&&n<minQueued)minQueued=n;if(!paused&&fill(TCB_CAPACITY))return 3;return 1;
}
WORD FmtPause(void){WORD r;if(!owner||owner!=GetCurrentTask())return 1;
 r=paused?unpauseClock():pauseClock();if(!r)paused=!paused;return r;
}
DWORD FmtPosition(void){DWORD n=owner?readClock():0;return (n/3579545UL)*3000UL+((n%3579545UL)*3UL)/3580UL;}
WORD FmtDebug(WORD i){if(i==35)return (WORD)refusedAt;if(i==36)return (WORD)(refusedAt>>16);if(i==37)return (WORD)refusedClock;if(i==38)return (WORD)(refusedClock>>16);if(i==39)return refusedQueue;if(i==40)return (WORD)refusedSource;if(i==41)return refusedGroups;if(i==33)return lastAdmission;if(i==34)return minQueued;if(i==30)return module&&debugClock?debugClock(27):saved[27];if(i==31)return module&&debugClock?debugClock(28):saved[28];if(i==32)return owner?(WORD)readClock():0;if(i==27)return paused;if(i==28)return core.error;if(i==29)return (WORD)core.groups;return i<27?(module&&debugClock?debugClock(i):saved[i]):0;}
char *FmtReason(void){return reason;}
