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
static SPSTATE events[SP_CAPACITY];
static WORD used,owner;
static volatile WORD index,active,timer,lastDiv,finished;
static volatile WORD delivered,maxLate,maxGap,lastTick,skipped;
static volatile WORD changes,noteOns,noteOffs;
static volatile DWORD totalLate;
static DWORD epoch,endAt,ivtBefore,ivtAfter;
static volatile DWORD position;
static HMODULE pin;
static char path[144];
#ifndef SP_HOST
static unsigned byteIn(unsigned);
#pragma aux byteIn = "in al,dx" "xor ah,ah" parm [dx] value [ax] modify exact [ax];
static void byteOut(unsigned,unsigned);
#pragma aux byteOut = "out dx,al" parm [dx] [ax] modify exact [];
#endif
static DWORD milliseconds(DWORD ticks)
{
    if(ticks>12000UL)return 660000UL;
    return TicksMS((WORD)ticks);
}
#ifdef SP_HOST
static DWORD biosTicks(void){return spHostTicks;}
static DWORD timerVector(void){return spHostVector;}
#else
static DWORD biosTicks(void){return *(DWORD FAR *)0x0040006cUL;}
static DWORD timerVector(void){return *(DWORD FAR *)0x00000020UL;}
#endif
static DWORD elapsedTicks(void)
{
    DWORD now=biosTicks();
    return now>=epoch?now-epoch:now+(0x001800b0UL-epoch);
}
static void silence(void){byteOut(0x61,byteIn(0x61)&0xfc);if(lastDiv)noteOffs++;lastDiv=0;}
static void tone(WORD divisor)
{
    if(divisor==lastDiv)return;
    changes++;silence();if(!divisor)return;
    byteOut(0x43,0xb6);byteOut(0x42,divisor&255);
    byteOut(0x42,divisor>>8);byteOut(0x61,byteIn(0x61)|3);lastDiv=divisor;noteOns++;
}
/* Called only by a fixed Windows SYSTEM timer: no USER/DOS, allocation,
   stream parsing, device detection or movable application callbacks. */
void _cdecl SpeakerTick(void)
{
    DWORD ticks,now,late,gap;WORD batch=0,divisor=0,have=0;
    if(!active)return;
    ticks=elapsedTicks();now=milliseconds(ticks);gap=now-position;position=now;
    if(gap>maxGap)maxGap=gap>65535UL?65535:(WORD)gap;
    while(index<used&&events[index].at<=now&&batch<16) {
        divisor=events[index].divisor;late=now-events[index].at;
        totalLate+=late;
        if(late>maxLate)maxLate=late>65535UL?65535:(WORD)late;
        index++;batch++;have=1;delivered++;
    }
    if(have){if(batch>1)skipped+=batch-1;tone(divisor);}
    if(index<used&&events[index].at<=now) {
        active=0;finished=2;silence();return;
    }
    if(now>=endAt){active=0;finished=1;silence();}
}
WORD SPAPI SCOpen(void)
{
    HMODULE h;WORD n;
    if(owner||timer)return 1;
    if(GetWinFlags()&WF_PMODE)return 2;
    if(OpenSound()<0)return 3;
    owner=GetCurrentTask();if(!owner){CloseSound();return 3;}
    h=GetModuleHandle("SPKCLK");n=h?GetModuleFileName(h,path,sizeof(path)):0;
    if(!n||n>=sizeof(path)||(pin=LoadLibrary(path))<32) {
        pin=0;CloseSound();owner=0;return 4;
    }
    ivtBefore=timerVector();
    used=index=active=finished=delivered=maxLate=maxGap=lastTick=skipped=0;
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
        events[i]=data[i];
    }
    if(events[0].at||events[count-1].at!=duration||
       events[count-1].divisor)return 2;
    used=count;endAt=duration;return 0;
}
WORD SPAPI SCStart(void)
{
    WORD flags,t;
    if(!owner||owner!=GetCurrentTask()||active||timer||!used)return 1;
    t=CreateSystemTimer(55,(FARPROC)TimerEntry);if(!t)return 2;
    flags=SaveIRQ();epoch=biosTicks();position=0;index=0;
    active=1;finished=0;timer=t;SpeakerTick();RestoreIRQ(flags);return 0;
}
WORD SPAPI SCStop(void)
{
    WORD flags,t;HMODULE h;
    if(owner&&owner!=GetCurrentTask())return 1;
    flags=SaveIRQ();active=0;t=timer;timer=0;
    if(owner)silence();
    RestoreIRQ(flags);
    if(t)KillSystemTimer(t);
    ivtAfter=timerVector();
    if(owner){CloseSound();owner=0;}
    h=pin;pin=0;if(h)FreeLibrary(h);return 0;
}
DWORD SPAPI SCClock(void)
{
    WORD flags;DWORD value;flags=SaveIRQ();
    value=active?milliseconds(elapsedTicks()):position;
    RestoreIRQ(flags);return value;
}
WORD SPAPI SCDebug(WORD q)
{
    WORD flags,r=0;flags=SaveIRQ();
    switch(q) {
    case 0:r=owner!=0;break;case 1:r=timer!=0;break;
    case 2:r=active;break;case 3:r=delivered;break;
    case 4:r=maxLate;break;case 5:r=maxGap;break;
    case 6:r=byteIn(0x61)&3;break;case 7:r=finished;break;
    case 8:r=ivtBefore==ivtAfter;break;case 9:r=index;break;
    case 11:r=lastDiv;break;case 12:r=skipped;break;
    case 13:r=(WORD)totalLate;break;case 14:r=(WORD)(totalLate>>16);break;
    case 15:r=changes;break;case 16:r=noteOns;break;case 17:r=noteOffs;break;
    }
    RestoreIRQ(flags);return r;
}
WORD SPAPI SCVersion(void){return 0x0100;}
