#define TC_HOST 1
#ifndef CLOCK_SOURCE
#define CLOCK_SOURCE "../SOURCE/TIMECLK.C"
#endif
#include CLOCK_SOURCE
#include <stdio.h>
static int failures,checks,inIRQ,unsafe;static WORD traceBytes[4096],traceCount,hi,task=1,sound;
static DWORD now;
static WORD irqDisabled;
static int leaseExport=1,leaseModule=1,leaseBusy,leaseReleaseFail;
static int leaseHeld,leaseClaims,leaseReleases,stopCalls;
static int failTimer,failInstall,failDetach;
static void api(void);
#ifdef SPEAKER_CLOCK
static WORD pitToken,pitDivisor,pitSuspended,pitReleases;
static int pitBusy,pitMissing,pitFailNext;
static int fakeFast(WORD token,WORD divisor,WORD command){
 if(!token||token!=pitToken)return -1;
 if(command==TP_SUSPEND){pitDivisor=0;pitSuspended=1;return 0;}
 if(command==TP_REENTER){if(pitBusy)return -3;pitSuspended=0;return 0;}
 if(command==TP_MUTE){pitDivisor=0;return 0;}
 if(command!=TP_SET||divisor<TP_MIN_DIVISOR)return -13;
 if(pitSuspended)return -3;
 if(pitBusy||pitFailNext){pitFailNext=0;pitDivisor=0;pitSuspended=1;return -3;}
 pitDivisor=divisor;return 0;
}
static int fakePitGet(WORD version,TPINFO FAR *out,WORD bytes,
 WORD operation){api();
 if(version!=TP_VERSION||bytes!=sizeof(TPINFO)||!out)return -16;
 if(operation!=TP_QUERY&&(!sound||!leaseHeld))return -1;
 if(operation==TP_CLAIM){if(pitToken||pitBusy)return -3;
  pitToken=7;pitSuspended=0;}
 else if(operation==TP_RELEASE){
  if(!out->token||out->token!=pitToken)return -1;
  pitToken=pitDivisor=pitSuspended=0;pitReleases++;
 }
 out->size=sizeof(TPINFO);out->version=TP_VERSION;
 out->features=TP_FIXED|TP_EXCLUSIVE|TP_SUSPEND_SAFE;
 out->minDivisor=TP_MIN_DIVISOR;out->maxDivisor=TP_MAX_DIVISOR;
 out->token=operation==TP_QUERY?0:pitToken;out->fast=fakeFast;
 return 0;
}
#endif
#define C(x) do{checks++;if(!(x)){printf("FAIL line%d: %s\n",__LINE__,#x);failures++;}}while(0)
static void api(void){if(inIRQ)unsafe++;}
HTASK GetCurrentTask(void){api();return task;}
HMODULE GetModuleHandle(const char *p){api();
 if(!strcmp(p,"SOUND")&&!leaseModule)return 0;
 return 33;}
static int fakeLease(unsigned version,unsigned op){api();
 if(version!=1)return -16;
 if(!sound||task!=1)return -1;
 if(op==1){leaseClaims++;if(leaseBusy)return -3;
  if(!stopCalls)return -3;
  leaseHeld=1;return 0;}
 if(op==2){leaseReleases++;if(leaseReleaseFail)return -3;
  leaseHeld=0;return 0;}
 return -16;
}
WORD GetModuleFileName(HMODULE h,char *p,WORD n){api();(void)h;(void)n;strcpy(p,"RAWCLK.DLL");return 10;}
HMODULE LoadLibrary(const char *p){api();(void)p;return 33;}WORD FreeLibrary(HMODULE h){api();(void)h;return 0;}
FARPROC GetProcAddress(HMODULE h,LPSTR p){api();(void)h;
 if((uintptr_t)p>65535U&&!strcmp(p,"TANDYPSGLEASE"))
  return leaseExport?(FARPROC)fakeLease:0;
#ifdef SPEAKER_CLOCK
 if((uintptr_t)p>65535U&&!strcmp(p,"TANDYPITLEASE"))
  return pitMissing?0:(FARPROC)fakePitGet;
#endif
 return NullEntry;
}
WORD GetWinFlags(void){api();return 0;}int OpenSound(void){api();if(sound)return -1;sound=1;return 3;}
void CloseSound(void){api();sound=0;leaseHeld=0;}
void StopSound(void){api();stopCalls++;leaseHeld=0;}
void Yield(void){api();}
WORD CreateSystemTimer(WORD n,FARPROC p){api();(void)n;(void)p;return failTimer?0:7;}void KillSystemTimer(WORD n){api();(void)n;}
void TimerEntry(void){}void NullEntry(void){}
WORD HiInstall(WORD n){(void)n;if(failInstall)return 1;hi=1;return 0;}WORD HiStop(void){hi=0;return 0;}
/* The real bridge reports2 while running, not0. Negative override means normal. */
static int brokerStateOverride=-1;
DWORD HiRead(void){return now;}WORD HiStatus(void){return brokerStateOverride>=0?(WORD)brokerStateOverride:(hi?2:0);}WORD HiMetric(WORD n){return n==8?1:n==9?hi:0;}
WORD HiRestart(void){hi=1;return 0;}WORD HiDetach(void){return failDetach?1:0;}WORD HiPeriod(void){return 9546;}
void HiPrepare(WORD n){(void)n;}void HiProgram(WORD n){(void)n;}void HiAudio(WORD n){(void)n;}
WORD SaveIRQ(void){WORD prior=irqDisabled;irqDisabled=1;return prior;}
void RestoreIRQ(WORD n){irqDisabled=n;}
void PsgByte(WORD n){if(traceCount<4096)traceBytes[traceCount++]=n;}
DWORD ReadStamp(void){return now;}
static void tick(DWORD t){WORD prior=irqDisabled;now=t;inIRQ=1;
 irqDisabled=1;Tick();irqDisabled=prior;inIRQ=0;}
int main(void){TCBINFO cap;TCBURST b,copy;TCFRAME f;WORD i,n;
 C(sizeof(TCFRAME)==16);C(sizeof(TCBURST)==24||sizeof(TCBURST)==22); /* host DWORD alignment differs */
 C(TCBGetCaps(TCB_VERSION,&cap,sizeof(cap))==0);C(cap.minCycles==9546UL&&cap.maxWrites==16);C(owner==0&&sound==0);
 C(TCBGetCaps(2,&cap,sizeof(cap))==3);C(TCBGetCaps(TCB_VERSION,&cap,1)==3);
 /* Failed acquisition never emits a PSG byte or keeps the legacy owner. */
 n=traceCount;leaseModule=0;C(TCOpen()==1);C(!sound&&!owner);
 C(traceCount==n&&!rawClaimed);leaseModule=1;
 leaseExport=0;C(TCOpen()==1);C(!sound&&!owner);
 C(traceCount==n&&!rawClaimed);leaseExport=1;
 leaseBusy=1;C(TCOpen()==1);C(!sound&&!owner);
 C(traceCount==n&&!rawClaimed);leaseBusy=0;
 C(TCBOpen()==0);C(leaseHeld&&rawClaimed);C(TCOpen()==1);
 C(burstMode==1);C(burstRegs[7]==15);
 C(interval(1)==9546);active=1;C(nextInterval(0)==9546);active=0;
 b.at=0;b.count=4;b.data[0]=0xe7;b.data[1]=0xf4;b.data[2]=0xc0;b.data[3]=2;C(TCBPush(&b)==0);
 b.at=9545;b.count=1;b.data[0]=0xf8;copy=b;n=count;C(TCBPush(&b)==4);C(count==n&&!memcmp(&b,&copy,sizeof(b)));
 b.at=9546;C(TCBPush(&b)==0);b.at=19092;b.count=2;b.data[0]=b.data[1]=0xe7;C(TCBPush(&b)==0);
 C(TCEnd()==1);C(TCBEnd()==0);traceCount=0;C(TCStart()==0);tick(0);C(noiseWrites==1);C(burstRegs[4]==32);C(traceCount==4);
 tick(9546);C(noiseWrites==1);C(burstRegs[7]==8);C(traceBytes[4]==0xf8);tick(19092);C(noiseWrites==3);C(finished==1&&active==0);C(unsafe==0);C(TCClose()==0);C(sound==0&&owner==0&&!leaseHeld&&!rawClaimed);
 C(TCBOpen()==0);b.count=1;b.data[0]=0xf8;
 for(i=0;i<TCB_CAPACITY;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);}
 b.at=(DWORD)TCB_CAPACITY*TCB_MIN_CYCLES;copy=b;n=tail;C(TCBPush(&b)==2);C(count==TCB_CAPACITY&&tail==n&&published==b.at-TCB_MIN_CYCLES);C(!memcmp(&copy,&b,sizeof(b)));
 task=2;C(TCBPush(&b)==1);C(TCClose()==1);
 C(leaseHeld&&rawClaimed&&sound);task=1;
 leaseReleaseFail=1;C(TCClose()==1);C(leaseHeld&&rawClaimed&&sound);
 leaseReleaseFail=0;C(TCClose()==0);C(!leaseHeld&&!rawClaimed&&!sound);
 C(TCBOpen()==0);b.at=0;b.count=1;b.data[0]=0xe7;C(TCBPush(&b)==0);b.at=100000;C(TCBPush(&b)==0);C(TCStart()==0);tick(0);C(noiseWrites==1);
 C(TCPause()==0);C(TCUnpause()==0);tick(1);C(noiseWrites==2);C(burstRegs[6]==7);C(TCClose()==0);
 C(TCBOpen()==0);b.count=1;b.data[0]=0xf8;
 for(i=0;i<TCB_CAPACITY;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);}C(TCStart()==0);tick(0);
 for(i=TCB_CAPACITY;i<1600;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);tick((DWORD)(i-TCB_CAPACITY+1)*TCB_MIN_CYCLES);C(error==0);}
 C(TCBEnd()==0);while(count)tick(now+TCB_MIN_CYCLES);C(burstWriteCount==1600);C(finished==1);C(TCClose()==0);
 C(TCBOpen()==0);b.at=0;C(TCBPush(&b)==0);b.at=TCB_MIN_CYCLES;C(TCBPush(&b)==0);C(TCStart()==0);tick(0);tick(TCB_MIN_CYCLES);tick(3UL*TCB_MIN_CYCLES);
 b.at=2UL*TCB_MIN_CYCLES;C(TCBPush(&b)==4);C(count==0&&error==0);b.at=4UL*TCB_MIN_CYCLES;C(TCBPush(&b)==0);tick(b.at);C(TCBEnd()==0);C(TCClose()==0);
 C(TCOpen()==0);memset(&f,0,sizeof(f));f.volume=65535;f.mask=15;C(TCPush(&f)==0);C(TCBPush(&b)==1);C(TCEnd()==0);C(TCStart()==0);tick(0);C(finished==1);C(TCClose()==0);
 #ifdef SPEAKER_CLOCK
 /* The same OpenSound owner may hold independent PSG and speaker leases. */
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 C(pitToken&&leaseHeld&&TCXStatus()==TCX_AVAILABLE);
 {TCXFRAME x;memset(&x,0,sizeof(x));x.frame.mask=15;
  x.frame.volume=65535;x.pitDivisor=2712;
  C(TCXFramePush(&x)==0);x.frame.at=100000;x.pitDivisor=0;
  C(TCXFramePush(&x)==0);C(TCStart()==0);tick(0);
  C(pitDivisor==2712&&leaseHeld);
  HiStop();inIRQ=1;TCBassSuspend();inIRQ=0;
  C(pitSuspended&&!pitDivisor&&leaseHeld);
  C(TCResume()==0);C(!pitSuspended&&leaseHeld);tick(1);
  C(pitDivisor==2712&&leaseHeld);C(TCClose()==0);
  C(!pitToken&&!pitDivisor&&!leaseHeld&&!sound&&pitReleases==1);
 }
#endif
 /* Failed timer/hook startup still has exactly the original raw owner,
    then normal close hands ownership to the next caller. */
 C(TCOpen()==0);memset(&f,0,sizeof(f));f.volume=65535;f.mask=15;
 C(TCPush(&f)==0);failTimer=1;C(TCStart()==2);
 C(leaseHeld&&rawClaimed&&sound&&!timer&&!timerPin);
 failTimer=0;C(TCClose()==0);C(!leaseHeld&&!rawClaimed&&!sound);
 C(TCOpen()==0);C(TCPush(&f)==0);failInstall=1;C(TCStart()==2);
 C(leaseHeld&&rawClaimed&&sound&&!timer&&!timerPin);
 failInstall=0;C(TCClose()==0);C(!leaseHeld&&!rawClaimed&&!sound);
 C(leaseClaims>1&&leaseReleases>1);
 printf("actual RAWCLK burst: %d checks, %d failures, unsafe IRQ calls=%d\n",checks,failures,unsafe);return failures!=0;
}
