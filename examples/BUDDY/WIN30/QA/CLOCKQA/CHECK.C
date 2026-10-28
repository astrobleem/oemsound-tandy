#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define SP_HOST
#include "../SPKCLK.C"
DWORD spHostTicks=100,spHostVector=0x12345678;
static unsigned gate=0xa0,writes,openSound,task=1,pins,timers,kills;
static int failSound,failTimer,protectedMode;
WORD GetWinFlags(void){return protectedMode?1:0;}
int OpenSound(void){if(failSound)return -1;openSound++;return 1;}
void CloseSound(void){if(!openSound)abort();openSound--;}
WORD GetCurrentTask(void){return task;}
HMODULE GetModuleHandle(const char *s){return strcmp(s,"SPKCLK")?0:40;}
WORD GetModuleFileName(HMODULE h,char *p,WORD n){(void)h;(void)n;strcpy(p,"SPKCLK.DLL");return 10;}
HMODULE LoadLibrary(const char *s){(void)s;pins++;return 40;}
void FreeLibrary(HMODULE h){(void)h;if(timers||!pins)abort();pins--;}
WORD CreateSystemTimer(WORD ms,FARPROC f){(void)f;if(ms!=55)abort();if(failTimer)return 0;timers++;return 9;}
void KillSystemTimer(WORD t){if(t!=9||active||!timers||(gate&3))abort();timers--;kills++;}
void TimerEntry(void){SpeakerTick();}
WORD SaveIRQ(void){return 0x200;}
void RestoreIRQ(WORD f){if(f!=0x200)abort();}
DWORD TicksMS(WORD ticks){return (DWORD)ticks*54+(((DWORD)ticks*60647)>>16);}
unsigned byteIn(unsigned p){if(p!=0x61)abort();return gate;}
void byteOut(unsigned p,unsigned v){if(p==0x61){if((v&0xfc)!=0xa0)abort();gate=v;}else if(p!=0x42&&p!=0x43)abort();writes++;}
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}checks++;}while(0)
int main(void)
{
    unsigned checks=0,oldWrites,i;SPSTATE a[5]={{0,2712},{100,0},{220,3044},{400,0},{500,0}};
    C(SCVersion()==0x100);protectedMode=1;C(SCOpen()==2);protectedMode=0;
    failSound=1;oldWrites=writes;C(SCOpen()==3&&writes==oldWrites);failSound=0;
    C(SCOpen()==0&&openSound==1&&pins==1&&!(gate&3));C(SCOpen()==1);
    C(SCLoad(a,5,500)==0);C(SCStart()==0&&timers==1&&(gate&3)==3);
    /* No foreground pumping: resident ticks alone articulate the score. */
    spHostTicks=101;TimerEntry();C((gate&3)==3);
    spHostTicks=102;TimerEntry();C(!(gate&3)&&SCClock()==109);
    spHostTicks=105;TimerEntry();C((gate&3)==3);
    spHostTicks=108;TimerEntry();C(!(gate&3));
    spHostTicks=110;TimerEntry();C(SCDebug(7)==1&&!active&&SCClock()==549);
    C(SCDebug(4)==54);task=2;C(SCStop()==1&&pins==1);task=1;
    C(SCStop()==0&&!openSound&&!pins&&!timers&&SCDebug(8)==1);
    oldWrites=writes;C(SCStop()==0&&writes==oldWrites);
    C(SCOpen()==0);a[1].at=0;C(SCLoad(a,5,500)==2);C(SCStart()==1);a[1].at=100;
    C(SCLoad(a,SP_CAPACITY+1,500)==1);a[2].divisor=100;C(SCLoad(a,5,500)==2);a[2].divisor=3044;
    a[4].divisor=2000;C(SCLoad(a,5,500)==2);a[4].divisor=0;
    C(SCLoad(a,5,500)==0);failTimer=1;C(SCStart()==2&&!timers&&!active);failTimer=0;
    C(SCStart()==0);C(SCStop()==0&&!(gate&3)&&!timers);
    /* Repeated sessions cannot leak a sound owner, timer or module pin. */
    for(i=0;i<32;i++){C(SCOpen()==0);C(SCLoad(a,5,500)==0);C(SCStart()==0);C(SCStop()==0);}
    C(!openSound&&!pins&&!timers&&!(gate&3)&&kills==34);
    C(SCOpen()==0);C(SCLoad(a,5,500)==0);spHostTicks=0x1800ae;
    C(SCStart()==0);spHostTicks=0;TimerEntry();C(SCClock()==109&&!(gate&3));
    C(SCStop()==0&&!timers&&!pins);
    printf("PASS %u checks; bounded resident state playback and teardown\n",checks);return 0;
}
