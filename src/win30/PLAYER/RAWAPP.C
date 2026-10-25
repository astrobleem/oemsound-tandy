#define WINVER 0x0300
#include <windows.h>
#include "RAWAPP.H"
#include "TIMECLK.H"
typedef WORD (FAR PASCAL *CLOCK0)(void);
typedef WORD (FAR PASCAL *CLOCK1)(WORD);
typedef WORD (FAR PASCAL *CLOCKPUT)(TCFRAME FAR *);
static CLOCK0 clockOpen,clockStart,clockEnd,clockClose,clockResume;
static CLOCK1 clockDebug;static CLOCKPUT clockPush;
static HMODULE clockModule;static char clockPath[144];static WORD saved[22];
#define TCOpen clockOpen
#define TCPush clockPush
#define TCStart clockStart
#define TCEnd clockEnd
#define TCClose clockClose
#define TCDebug clockDebug
void RawInit(LPSTR directory){lstrcpy(clockPath,directory);lstrcat(clockPath,"RAWCLK.DLL");}
static WORD loadClock(void){
 clockModule=LoadLibrary(clockPath);if(clockModule<32){clockModule=0;return 1;}
 clockOpen=(CLOCK0)GetProcAddress(clockModule,(LPSTR)1L);clockPush=(CLOCKPUT)GetProcAddress(clockModule,(LPSTR)2L);
 clockStart=(CLOCK0)GetProcAddress(clockModule,(LPSTR)3L);clockEnd=(CLOCK0)GetProcAddress(clockModule,(LPSTR)4L);
 clockClose=(CLOCK0)GetProcAddress(clockModule,(LPSTR)5L);clockDebug=(CLOCK1)GetProcAddress(clockModule,(LPSTR)6L);clockResume=(CLOCK0)GetProcAddress(clockModule,(LPSTR)11L);
 if(!clockOpen||!clockPush||!clockStart||!clockEnd||!clockClose||!clockDebug||!clockResume){FreeLibrary(clockModule);clockModule=0;return 1;}return 0;
}
static WORD diagnostic(WORD i){return clockModule?clockDebug(i):(i<22?saved[i]:0);}
static RFSong song;static TCFRAME pending;
static HGLOBAL input,events;static HTASK owner;
static DWORD epoch,nextAt,remainder;static WORD ended,hasPending,lastHz;
static WORD fill(WORD budget)
{
 WORD r,c;
 while(budget-- && !ended){
  if(!hasPending){
   r=RFStep(&song);if(!r && song.error)return 3;
   pending.at=nextAt;pending.volume=0;pending.noise=song.noise;pending.mask=song.dirty;
   for(c=0;c<4;c++){pending.volume|=song.volume[c]<<(c*4);if(c<3)pending.p[c]=song.period[c];}
   song.dirty=0;hasPending=r?1:2;
   if(lastHz!=song.hz){remainder=0;lastHz=song.hz;}
   remainder+=1193181UL;nextAt+=remainder/song.hz;remainder%=song.hz;
  }
  r=TCPush(&pending);if(r==2)return 0;if(r)return 3;
  if(hasPending==2){if(TCEnd())return 3;ended=1;}
  hasPending=0;
 }return 0;
}
WORD RawClose(void)
{
 WORD i;
 if(owner && owner!=GetCurrentTask())return 1;
 if(owner){if(TCClose())return 1;owner=0;}
 if(input){GlobalUnlock(input);if(GlobalFree(input)){song.data=GlobalLock(input);return 1;}input=0;}
 if(events){GlobalUnlock(events);if(GlobalFree(events)){song.events=GlobalLock(events);return 1;}events=0;}
 if(clockModule){for(i=0;i<22;i++)saved[i]=clockDebug(i);FreeLibrary(clockModule);clockModule=0;}
 return 0;
}
WORD RawOpen(LPSTR path,WORD kind)
{
 HFILE f;LONG size;WORD r;
 if(RawClose())return 1;
 f=_lopen(path,OF_READ|OF_SHARE_DENY_WRITE);if(f==HFILE_ERROR)return 4;
 size=_llseek(f,0L,2);_llseek(f,0L,0);if(size<8||size>RF_MAX){_lclose(f);return 2;}
 input=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,(DWORD)size);events=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,(DWORD)RF_EVENTS);
 if(!input||!events){_lclose(f);RawClose();return 5;}
 song.data=GlobalLock(input);song.events=GlobalLock(events);if(!song.data||!song.events){_lclose(f);RawClose();return 5;}
 r=_lread(f,song.data,(WORD)size);_lclose(f);if(r!=(WORD)size){RawClose();return 4;}
 r=RFLoad(&song,song.data,(WORD)size,song.events,kind);if(r){RawClose();return r;}
 if(loadClock()){RawClose();return 8;}
 if(song.kind==1){WORD p,c,row,k;
  if(song.hz>120){RawClose();return 2;}
  for(p=0;p<song.patterns;p++)for(c=0;c<3;c++)for(row=0;row<64;row++){
   k=((p*4+c)*64+row)*3;
   if(song.events[k]!=255 && (song.events[k+1]&15)==15 && song.events[k+2]>120){RawClose();return 2;}
  }
 }
 r=TCOpen();if(r){RawClose();return r==2?6:7;}owner=GetCurrentTask();if(!owner){TCClose();RawClose();return 7;}
 ended=hasPending=lastHz=0;nextAt=remainder=0;
 r=fill(TC_CAP);if(r||TCStart()){RawClose();return 3;}
 epoch=GetTickCount();return 0;
}
WORD RawPump(void)
{
 if(!owner||owner!=GetCurrentTask())return 2;
 if(TCDebug(6))return 3;
 if(TCDebug(1)&&!TCDebug(20))clockResume();
 if(TCDebug(7))return 0;
 if(fill(128))return 3;return 1;
}
DWORD RawPosition(void){return owner?GetTickCount()-epoch:0;}
WORD RawDebug(WORD item){switch(item){case 0:return owner!=0;case 1:return diagnostic(12);case 2:return diagnostic(4);case 3:return diagnostic(8);case 4:return song.kind;case 5:return diagnostic(6);case 6:return diagnostic(2);case 7:return song.period[0];case 8:return song.volume[0];case 9:return diagnostic(3);case 10:return diagnostic(0);case 11:return diagnostic(19);case 12:return diagnostic(20);case 13:return diagnostic(21);}return 0;}
