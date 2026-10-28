#define WINVER 0x0300
#include <windows.h>
#include "SPKAPI.H"
#ifdef SP_HOST
#define SPAPI
extern DWORD spHostTicks,spHostVector;
extern unsigned byteIn(unsigned);
extern void byteOut(unsigned,unsigned);
#else
#define SPAPI FAR PASCAL __loadds __export
#endif
extern WORD FAR PASCAL CreateSystemTimer(WORD,FARPROC);
extern void FAR PASCAL KillSystemTimer(WORD);
extern void FAR PASCAL TimerEntry(void);
extern WORD _cdecl SaveIRQ(void);
extern void _cdecl RestoreIRQ(WORD);
extern DWORD _cdecl TicksMS(WORD);
extern void FAR PASCAL NullEntry(void);
extern WORD _cdecl HiFind(void);
extern WORD _cdecl HiInstall(WORD);
extern WORD _cdecl HiStop(void);
extern WORD _cdecl HiStatus(void);
extern WORD _cdecl HiDetach(void);
extern WORD _cdecl HiRestart(void);
extern WORD _cdecl HiMetric(WORD);
extern WORD _cdecl HookMetric(WORD);
extern void _cdecl HiPrepare(WORD);
extern void _cdecl HiProgram(WORD);
extern void _cdecl HiAudio(WORD);
extern WORD _cdecl HiPeriod(void);
extern DWORD _cdecl HiRead(void);
extern void _cdecl HiAbandon(void);
static SPSTATE events[SP_CAPACITY];
static WORD used,owner;
static volatile WORD index,active,timer,lastDiv,finished;
static volatile WORD delivered,skipped;
static volatile DWORD maxLate,maxGap;
static volatile WORD changes,noteOns,noteOffs;
static volatile DWORD totalLate;
static DWORD lastStamp,endAt,endMS,ivtBefore,ivtAfter;
static WORD resumePending,scoreDivisor,dosSuspends;
static volatile DWORD position;
static HMODULE pin;
static char path[144];
#ifndef SP_HOST
static unsigned byteIn(unsigned);
#pragma aux byteIn = "in al,dx" "xor ah,ah" parm [dx] value [ax] modify exact [ax];
static void byteOut(unsigned,unsigned);
#pragma aux byteOut = "out dx,al" parm [dx] [ax] modify exact [];
#endif
/* Foreground integer conversions: all ISR scheduling uses PIT cycles. */
static DWORD toCycles(DWORD ms){return ms*1193UL+(ms*182UL+999UL)/1000UL;}
#ifdef SP_HOST
static DWORD toMS(DWORD cycles){DWORD hi=cycles>>16,lo=cycles&65535UL;
 return hi*54UL+((hi*60647UL+lo*54UL+((lo*60647UL)>>16))>>16);}
#else
extern DWORD _cdecl CyclesMS(DWORD);
static DWORD toMS(DWORD cycles){return CyclesMS(cycles);}
#endif
#ifdef SP_HOST
static DWORD timerVector(void){return spHostVector;}
static WORD systemSegment(void){return 0x1234;}
static int systemMatched(void){return 1;}
#else
static DWORD timerVector(void){return *(DWORD FAR *)0x20UL;}
static WORD systemSegment(void){FARPROC p=GetProcAddress(GetModuleHandle("SYSTEM"),(LPSTR)2L);return HIWORD((DWORD)p);}
static int systemMatched(void){WORD seg=systemSegment(),i;BYTE FAR *p;
 static BYTE enabled[6]={0x2e,0x80,0x3e,0x7e,2,0},disabled[6]={0x1e,0x2e,0x8e,0x1e,0x0f,0};
 if(!seg)return 0;p=(BYTE FAR *)((DWORD)seg<<16);
 if(p[0x27e]!=1)return 0;
 for(i=0;i<6;i++)if(p[0x401+i]!=enabled[i]||p[0x42d+i]!=disabled[i])return 0;
 return 1;
}
#endif
WORD SPAPI SCReady(void){return !(GetWinFlags()&WF_PMODE)&&!HiFind()&&systemMatched()&&!HiStatus();}
static WORD interval(DWORD delta){if(delta<9546UL)return 9546;if(delta>65535UL)return delta<75081UL?(WORD)(delta>>1):65535;return (WORD)delta;}
static WORD nextPeriod(DWORD future){WORD i=index;
 if(!active||i>=used)return 65535;
 if(events[i].at<=future){if(i+1>=used)return 9546;i++;}
 return events[i].at>future?interval(events[i].at-future):9546;
}
static void silence(void){byteOut(0x61,byteIn(0x61)&0xfc);if(lastDiv)noteOffs++;lastDiv=0;}
static void tone(WORD divisor)
{
    if(divisor==lastDiv)return;
    changes++;silence();if(!divisor)return;
    byteOut(0x43,0xb6);byteOut(0x42,divisor&255);
    byteOut(0x42,divisor>>8);byteOut(0x61,byteIn(0x61)|3);lastDiv=divisor;noteOns++;
}
/* Same fixed-register callback as the library. The existing broker alone
   owns PIT0/IRQ0. HiAudio(0) prevents all broker PSG writes on generic CGA. */
void _cdecl SpeakerTick(void)
{
 DWORD now,gap,late;WORD batch=0,divisor=0,have=0;
 if(!active)return;
 now=HiRead();gap=now-lastStamp;lastStamp=now;
 if(gap>4194304UL){active=0;finished=2;silence();return;}
 position+=gap;if(gap>maxGap)maxGap=gap;
 while(index<used&&events[index].at<=position&&batch<16){
  divisor=events[index].divisor;late=position-events[index].at;totalLate+=late;
  if(late>maxLate)maxLate=late;
  index++;batch++;delivered++;have=1;}
 if(have){if(batch>1)skipped+=batch-1;scoreDivisor=divisor;tone(divisor);resumePending=0;}
 else if(resumePending){tone(scoreDivisor);resumePending=0;}
 if(index<used&&events[index].at<=position){active=0;finished=2;silence();return;}
 if(position>=endAt){active=0;finished=1;silence();}
 HiProgram(nextPeriod(position+(DWORD)HiPeriod()));
}
void _cdecl TCBassSuspend(void){if(owner){silence();resumePending=1;dosSuspends++;}}
void _cdecl FastAbandon(void){active=0;if(owner)silence();HiAbandon();}
WORD SPAPI SCOpen(void)
{
    HMODULE h;WORD n;
    if(owner||timer)return 1;
    if(!SCReady())return 2;
    if(OpenSound()<0)return 3;
    owner=GetCurrentTask();if(!owner){CloseSound();return 3;}
    h=GetModuleHandle("SPKFAST");n=h?GetModuleFileName(h,path,sizeof(path)):0;
    if(!n||n>=sizeof(path)||(pin=LoadLibrary(path))<32) {
        pin=0;CloseSound();owner=0;return 4;
    }
    ivtBefore=timerVector();
    used=index=active=finished=delivered=skipped=0;maxLate=maxGap=0;
    resumePending=scoreDivisor=dosSuspends=0;lastStamp=0;
    changes=noteOns=noteOffs=0;totalLate=0;
    position=0;silence();return 0;
}
WORD SPAPI SCLoad(SPSTATE FAR *data,WORD count,DWORD duration)
{
    WORD i;
    if(!owner||owner!=GetCurrentTask()||active||timer||!data||
       !count||count>SP_CAPACITY||!duration||duration>600000UL)return 1;
    used=0;for(i=0;i<count;i++) {
        if(data[i].at>duration||(i&&data[i].at<=data[i-1].at)||
           (data[i].divisor&&data[i].divisor<151))return 2;
        events[i]=data[i];events[i].at=toCycles(data[i].at);
    }
    if(events[0].at||events[count-1].at!=toCycles(duration)||
       events[count-1].divisor)return 2;
    used=count;endAt=toCycles(duration);endMS=duration;return 0;
}
WORD SPAPI SCStart(void)
{
 WORD flags,t;
 if(!owner||owner!=GetCurrentTask()||active||timer||!used||!SCReady())return 1;
 t=CreateSystemTimer(55,(FARPROC)NullEntry);if(!t)return 2;
 flags=SaveIRQ();position=lastStamp=0;index=delivered=0;
 active=1;finished=0;timer=t;scoreDivisor=0;
 silence();RestoreIRQ(flags);HiAudio(0);HiPrepare(nextPeriod(0));
 if(HiInstall(systemSegment())){flags=SaveIRQ();active=0;silence();RestoreIRQ(flags);
  HiDetach();KillSystemTimer(t);timer=0;return 2;}
 return 0;
}
WORD SPAPI SCStop(void)
{
    WORD flags,t,n;HMODULE h;
    if(owner&&owner!=GetCurrentTask())return 1;
    flags=SaveIRQ();active=0;t=timer;
    if(owner)silence();
    RestoreIRQ(flags);
    if(t){if(HiStop())return 1;
        for(n=0;HiStatus()&&n<4000;n++)Yield();
        if(HiStatus()||HiDetach())return 1;
        KillSystemTimer(t);timer=0;}
    ivtAfter=timerVector();
    if(owner){CloseSound();owner=0;}
    h=pin;pin=0;if(h)FreeLibrary(h);return 0;
}
DWORD SPAPI SCClock(void)
{
 WORD flags;DWORD value;
 if(active&&!HiStatus()&&HiMetric(8)&&!HiMetric(9)){
  HiPrepare(nextPeriod(position));HiRestart();}
 flags=SaveIRQ();value=position;RestoreIRQ(flags);return finished==1?endMS:toMS(value);
}
WORD SPAPI SCDebug(WORD q)
{
    WORD flags,r=0;flags=SaveIRQ();
    switch(q) {
    case 0:r=owner!=0;break;case 1:r=timer!=0;break;
    case 2:r=active;break;case 3:r=delivered;break;
    case 4:r=(WORD)toMS(maxLate);break;case 5:r=(WORD)toMS(maxGap);break;
    case 10:r=1;break;case 18:r=HookMetric(3);break;case 19:r=HookMetric(0);break;case 20:r=dosSuspends;break;
    case 6:r=byteIn(0x61)&3;break;case 7:r=finished;break;
    case 8:r=ivtBefore==ivtAfter;break;case 9:r=index;break;
    case 11:r=lastDiv;break;case 12:r=skipped;break;
    case 13:r=(WORD)toMS(totalLate);break;case 14:r=(WORD)(toMS(totalLate)>>16);break;
    case 15:r=changes;break;case 16:r=noteOns;break;case 17:r=noteOffs;break;
    }
    RestoreIRQ(flags);return r;
}
WORD SPAPI SCVersion(void){return 0x0100;}
