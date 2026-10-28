
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SP_HOST
#include "../FASTCLK.C"
DWORD spHostTicks,spHostVector=0x12345678;
static unsigned gate=0xa0,writes,sound,pins,timers,task=1;
static WORD period=9546,queuedPeriod=9546,running,installed,online,pending;
static DWORD cycles;
static int failSound,failInstall,failTimer,protectedMode,missing;
WORD GetWinFlags(void){return protectedMode;}
int OpenSound(void){if(failSound)return -1;sound++;return 1;}
void CloseSound(void){if(!sound)abort();sound--;}
WORD GetCurrentTask(void){return task;}
HMODULE GetModuleHandle(const char *s){return strcmp(s,"SPKFAST")?0:40;}
WORD GetModuleFileName(HMODULE h,char *p,WORD n){(void)h;(void)n;strcpy(p,"SPKFAST.DLL");return 11;}
HMODULE LoadLibrary(const char *p){(void)p;pins++;return 40;}
void FreeLibrary(HMODULE h){(void)h;if(timers||installed||!pins)abort();pins--;}
WORD CreateSystemTimer(WORD ms,FARPROC f){(void)f;if(ms!=55)abort();if(failTimer)return 0;timers++;return 9;}
void KillSystemTimer(WORD t){if(t!=9||active||installed||!timers||(gate&3))abort();timers--;}
void TimerEntry(void){SpeakerTick();} void NullEntry(void){}
WORD SaveIRQ(void){return 0x200;}void RestoreIRQ(WORD f){if(f!=0x200)abort();}
DWORD TicksMS(WORD t){return (DWORD)t*55;}
unsigned byteIn(unsigned p){if(p!=0x61)abort();return gate;}
void byteOut(unsigned p,unsigned v){if(!sound)abort();if(p==0x61){if((v&0xfc)!=0xa0)abort();gate=v;}else if(p!=0x42&&p!=0x43)abort();writes++;}
WORD HiFind(void){return missing;}WORD HiStatus(void){return running;}
WORD HiInstall(WORD seg){if(seg!=0x1234||failInstall)return 1;installed=running=online=1;cycles=0;return 0;}
WORD HiStop(void){running=0;return 0;}WORD HiDetach(void){installed=0;return 0;}
WORD HiRestart(void){running=1;return 0;}
WORD HiMetric(WORD q){return q==8?online:q==9?pending:0;}
WORD HookMetric(WORD q){return q==0?installed:0;}
void HiPrepare(WORD p){period=queuedPeriod=p;}void HiProgram(WORD p){if(p<9546)abort();queuedPeriod=p;}
void HiAudio(WORD yes){if(yes)abort();}WORD HiPeriod(void){return period;}
DWORD HiRead(void){return cycles;}void HiAbandon(void){running=installed=0;}
void Yield(void){}
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}checks++;}while(0)
static DWORD le32(const unsigned char*p){return (DWORD)p[0]|((DWORD)p[1]<<8)|((DWORD)p[2]<<16)|((DWORD)p[3]<<24);}
#include "../SPEAKER.H"
int main(void){unsigned checks=0,oldWrites,i,count;DWORD duration;FILE*f;unsigned char h[20],b[14];static SPSTATE a[SP_CAPACITY];
 C(SCReady());missing=1;C(SCOpen()==2);missing=0;protectedMode=1;C(SCOpen()==2);protectedMode=0;
 failSound=1;oldWrites=writes;C(SCOpen()==3&&writes==oldWrites);failSound=0;
 C(SCOpen()==0&&sound==1&&pins==1);C(SCOpen()==1);C(SCStop()==0&&!sound&&!pins);
 f=fopen("media/SPEAKER.WZM","rb");C(f!=0);C(fread(h,1,20,f)==20);count=le32(h+8);duration=le32(h+12);C(count<SP_CAPACITY&&duration==241400);
 for(i=0;i<count;i++){C(fread(b,1,14,f)==14);a[i].at=le32(b);a[i].divisor=speakerPeriods[b[5]];}fclose(f);
 a[count].at=duration;a[count++].divisor=0;
 C(SCOpen()==0);C(SCLoad(a,count,duration)==0);
 failTimer=1;C(SCStart()==2&&!timers&&!active);failTimer=0;
 failInstall=1;C(SCStart()==2&&!timers&&!active&&!(gate&3));failInstall=0;
 C(SCStart()==0&&installed&&running&&!lastDiv&&delivered==0);
 /* TBRIDGE's first synchronized BIOS edge calls back at logical cycle zero. */
 SpeakerTick();C(lastDiv==a[0].divisor&&delivered==1);
 for(i=0;i<50000&&active;i++){cycles+=period;period=queuedPeriod;SpeakerTick();if(i==100){TCBassSuspend();running=0;C(!(gate&3));C(SCClock()>0&&running);}}
 C(!active&&finished==1&&index==count&&delivered==count);C(!skipped&&!(gate&3));C(toMS(maxLate)<=8);C(SCClock()>=duration);
 task=2;C(SCStop()==1&&installed);task=1;C(SCStop()==0&&!installed&&!running&&!sound&&!pins&&!timers&&SCDebug(8));
 oldWrites=writes;C(SCStop()==0&&writes==oldWrites);
 printf("PASS %u checks: %u full-song states, %u broker callbacks, max late %lu ms, %u DOS suspend; teardown clean\n",checks,count,i,(unsigned long)toMS(maxLate),dosSuspends);return 0;}
