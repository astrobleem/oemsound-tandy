#define WINVER 0x0300
#include <windows.h>
#define TC_DRIVER
#include "TIMECLK.H"
extern WORD FAR PASCAL CreateSystemTimer(WORD,FARPROC);
extern void FAR PASCAL KillSystemTimer(WORD);
extern void FAR PASCAL TimerEntry(void);
extern void FAR PASCAL NullEntry(void);
extern WORD _cdecl HiInstall(WORD);
extern WORD _cdecl HiStop(void);
extern DWORD _cdecl HiRead(void);
extern WORD _cdecl HiStatus(void),HiMetric(WORD),HiRestart(void),HiDetach(void),HiPeriod(void);
extern void _cdecl HiPrepare(WORD),HiProgram(WORD),HiAudio(WORD);
extern WORD _cdecl SaveIRQ(void);
extern void _cdecl RestoreIRQ(WORD);
extern void _cdecl PsgByte(WORD);
extern DWORD _cdecl ReadStamp(void);
static HMODULE timerPin;static char timerPath[144];
static WORD pinCode(void){WORD n;HMODULE h;if(timerPin)return 0;
 h=GetModuleHandle("RAWCLK");if(!h)return 1;n=GetModuleFileName(h,timerPath,sizeof(timerPath));if(!n||n>=sizeof(timerPath))return 1;
 h=LoadLibrary(timerPath);if(h<32)return 1;timerPin=h;return 0;
}
static void unpinCode(void){HMODULE h=timerPin;timerPin=0;if(h)FreeLibrary(h);}
static TCFRAME queue[TC_CAP],lastFrame,restoredFrame;static WORD lastValid,restorePending;
HTASK owner=0;static HTASK probeOwner;
WORD timer=0,probeTimer=0,head=0,tail=0,count=0,end=0,active=0,finished=0,error=0;
static DWORD epoch,published,lastDue,lastStamp,clockCycles;
static WORD wideClock,userPaused;
static WORD due(DWORD at,DWORD now){return wideClock?(LONG)(now-at)>=0:now>=at;}
static WORD calls,sent,maxBatch,maxLate,maxGap,overflows,writes,underruns,allCalls;
static WORD probeCount,probeMin,probeMax,probeTicks;
static DWORD probeFirst,probeLast;
/* Bounded lookahead into already-published frames. No parsing/division. */
static WORD interval(DWORD delta){if(delta<9000UL)return 65535U;if(delta>65535UL)return delta<74535UL?(WORD)(delta>>1):65535U;return (WORD)delta;}
static WORD nextInterval(DWORD future){WORD i=head,n=count;if(!active||!n)return 65535U;
 if(due(queue[i].at,future)){if(n<2)return 65535U;i=(i+1)&(TC_CAP-1);}
 return !due(queue[i].at,future)?interval(queue[i].at-future):65535U;}
static void output(WORD v){PsgByte(v);writes++;}
static void mute(void){WORD c;for(c=0;c<4;c++)output(0x9f|(c<<5));}
static void frame(TCFRAME *f){WORD c;lastFrame=*f;lastValid=1;for(c=0;c<3;c++)if(f->mask&(1<<c)){output(0x80|(c<<5)|(f->p[c]&15));output((f->p[c]>>4)&63);output(0x90|(c<<5)|((f->volume>>(c*4))&15));}if(f->mask&8){output(0xe0|(f->noise&7));output(0xf0|(f->volume>>12));}}
/* Fixed code/data only. Bounded1 frame transition, no Windows/DOS calls,
   allocation, division or input parsing. Reads resident PIT-cycle accounting and writes PSG output. */
void _cdecl Tick(void)
{
 DWORD now,elapsed,gap,late;WORD batch=0,restoring;
 allCalls++;now=ReadStamp();
 if(probeTimer){if(probeCount){gap=now-probeLast;if(gap<probeMin)probeMin=(WORD)gap;if(gap>probeMax)probeMax=gap>65535UL?65535U:(WORD)gap;}else probeFirst=now;
  probeLast=now;probeCount++;probeTicks=(WORD)now;}
 if(!active)return;
 restoring=restorePending;restorePending=0;now=HiRead();calls++;
 if(epoch==0xffffffffUL){epoch=lastStamp=now;clockCycles=0;gap=0;}
 else {gap=now-lastStamp;lastStamp=now;
  if(gap>4194304UL){error=3;active=0;mute();return;}clockCycles+=gap;}
 if((gap>>6)>maxGap)maxGap=(gap>>6)>65535UL?65535U:(WORD)(gap>>6);
 elapsed=clockCycles;
 if(restoring&&lastValid&&(!count||!due(queue[head].at,elapsed))){restoredFrame=lastFrame;restoredFrame.mask=15;frame(&restoredFrame);restoring=0;}
 while(count && due(queue[head].at,elapsed) && batch<1){
  if(restoring){restoredFrame=queue[head];restoredFrame.mask=15;frame(&restoredFrame);restoring=0;}else frame(&queue[head]);late=elapsed-queue[head].at;if((late>>6)>maxLate)maxLate=(late>>6)>65535UL?65535U:(WORD)(late>>6);lastDue=queue[head].at;head=(head+1)&(TC_CAP-1);count--;batch++;sent++;
 }
 if(batch>maxBatch)maxBatch=batch;
 if(count && due(queue[head].at,elapsed)){error=3;active=0;mute();return;}
 if(!count){if(end){finished=1;active=0;mute();}else if(elapsed-lastDue>1193181UL){underruns++;error=4;active=0;mute();}}
 HiProgram(nextInterval(elapsed+(DWORD)HiPeriod()));
}
static WORD owns(void){return owner && owner==GetCurrentTask();}
WORD TCAPI TCOpen(void){WORD flags;
 if(owner || probeTimer || timerPin)return 1;
 if((GetWinFlags()&WF_PMODE)||*(BYTE FAR *)0xfc000000UL!=0x21)return 2;
 if(OpenSound()<0)return 1;owner=GetCurrentTask();if(!owner){CloseSound();return 1;}StopSound();
 flags=SaveIRQ();head=tail=count=end=active=finished=error=0;calls=sent=maxBatch=maxLate=maxGap=overflows=writes=underruns=0;published=lastDue=0;lastValid=restorePending=wideClock=userPaused=0;mute();RestoreIRQ(flags);return 0;
}
WORD TCAPI TCPush(TCFRAME FAR *f){WORD flags,t;
 if(!owns()||!f||end||error)return 1;
 if(f->mask>15 || f->noise>7 || (!wideClock&&f->at>2147725800UL))return 3;
 for(t=0;t<3;t++)if(f->p[t]>1023 || ((f->mask&(1<<t))&&!f->p[t]&&((f->volume>>(t*4))&15)!=15))return 3;
 flags=SaveIRQ();if(count==TC_CAP){overflows++;RestoreIRQ(flags);return 2;}t=tail;RestoreIRQ(flags);
 if((count||active||sent) && (wideClock?(LONG)(f->at-published)<0:f->at<published))return 3;
 queue[t]=*f;
 flags=SaveIRQ();if(error||end){RestoreIRQ(flags);return 1;}published=f->at;tail=(tail+1)&(TC_CAP-1);count++;RestoreIRQ(flags);return 0;
}
WORD TCAPI TCStart(void){WORD t,flags;FARPROC p;if(!owns()||active||timer||!count)return 1;
 if(pinCode())return 2;t=CreateSystemTimer(55,(FARPROC)NullEntry);if(!t){unpinCode();return 2;}
 p=GetProcAddress(GetModuleHandle("SYSTEM"),(LPSTR)2L);HiAudio(1);
 flags=SaveIRQ();epoch=0xffffffffUL;lastStamp=clockCycles=0;active=1;RestoreIRQ(flags);HiPrepare(nextInterval(0));
 if(!p||HiInstall(HIWORD((DWORD)p))){active=0;HiDetach();KillSystemTimer(t);unpinCode();return 2;}
 timer=t;return 0;
}
WORD TCAPI TCEnd(void){WORD flags;if(!owns())return 1;flags=SaveIRQ();
 if(!published && !count && !sent){RestoreIRQ(flags);return 3;}
 if(queue[(tail-1)&(TC_CAP-1)].volume!=65535U){RestoreIRQ(flags);return 3;}
 end=1;if(!count){finished=1;active=0;mute();}RestoreIRQ(flags);return 0;}
WORD TCAPI TCClose(void){WORD t,flags,n;if(owner&&!owns())return 1;if(probeTimer)return 1;
 flags=SaveIRQ();active=0;restorePending=0;t=timer;if(owner)mute();RestoreIRQ(flags);
 if(t){if(HiStop())return 1;for(n=0;HiStatus()&&n<4000;n++)Yield();if(HiStatus()||HiDetach())return 1;KillSystemTimer(t);timer=0;}
 if(owner){StopSound();CloseSound();owner=0;}
 head=tail=count=end=0;unpinCode();return 0;
}
WORD TCAPI TCResume(void){WORD flags,result;
 if(!owns()||!timer||!active||userPaused)return 1;
 if(HiMetric(9)||HiStatus()||!HiMetric(8))return 1;
 HiPrepare(nextInterval(clockCycles));flags=SaveIRQ();restorePending=1;RestoreIRQ(flags);
 result=HiRestart();if(result)restorePending=0;return result;}
WORD TCAPI TCDebug(WORD i){WORD f,r=0;f=SaveIRQ();switch(i){case 0:r=count;break;case 1:r=active;break;case 2:r=sent;break;case 3:r=maxLate;break;case 4:r=maxGap;break;case 5:r=maxBatch;break;case 6:r=error;break;case 7:r=finished;break;case 8:r=calls;break;case 9:r=timer;break;case 10:r=owner;break;case 11:r=overflows;break;case 12:r=writes;break;case 13:r=probeCount;break;case 14:r=probeMin;break;case 15:r=probeMax;break;case 16:r=(WORD)(probeLast-probeFirst);break;case 17:r=underruns;break;case 18:r=allCalls;break;case 19:r=timer?HiMetric(8):0;break;case 20:r=timer?HiMetric(9):0;break;case 21:r=timer?HiStatus():0;break;}RestoreIRQ(f);return r;}
WORD TCAPI TCProbe(WORD period){WORD t;if(owner||probeTimer)return 1;if(GetWinFlags()&WF_PMODE)return 2;
 probeOwner=GetCurrentTask();probeCount=probeMax=0;probeMin=65535U;probeFirst=probeLast=0;
 if(pinCode()){probeOwner=0;return 2;}t=CreateSystemTimer(period,(FARPROC)TimerEntry);if(!t){probeOwner=0;unpinCode();return 2;}probeTimer=t;return 0;}
WORD TCAPI TCProbeOff(void){WORD t;if(owner)return 1;if(probeOwner && probeOwner!=GetCurrentTask())return 1;t=probeTimer;probeTimer=0;if(t)KillSystemTimer(t);probeOwner=0;unpinCode();return 0;}
DWORD TCAPI TCStamp(void){if(GetWinFlags()&WF_PMODE)return 0;return ReadStamp();}

DWORD TCAPI TCClock(void){WORD f;DWORD r;if(!owns())return 0;f=SaveIRQ();r=clockCycles;RestoreIRQ(f);return r;}
WORD TCAPI TCWideOpen(void){WORD r=TCOpen();if(!r)wideClock=1;return r;}
WORD TCAPI TCPause(void){WORD f;if(!owns()||!timer||!active||userPaused)return 1;
 if(HiMetric(9)&&HiStop())return 1;f=SaveIRQ();userPaused=1;mute();RestoreIRQ(f);return 0;}
WORD TCAPI TCUnpause(void){WORD r;if(!owns()||!userPaused)return 1;userPaused=0;r=TCResume();if(r)userPaused=1;return r;}

WORD TCAPI TCFlush(void){WORD flags,n;if(!owns())return 1;
 if(timer&&HiMetric(9)&&HiStop())return 1;
 for(n=0;timer&&HiStatus()&&n<4000;n++)Yield();if(timer&&HiStatus())return 1;
 flags=SaveIRQ();active=0;userPaused=0;restorePending=lastValid=0;head=tail=count=end=finished=error=0;published=lastDue=clockCycles=lastStamp=0;epoch=0xffffffffUL;sent=0;mute();RestoreIRQ(flags);return 0;}
WORD TCAPI TCRearm(void){WORD r;if(!owns()||!timer||active||!count)return 1;
 active=1;HiPrepare(nextInterval(0));epoch=0xffffffffUL;lastStamp=clockCycles=0;r=HiRestart();if(r)active=0;return r;}

WORD TCAPI TCMask(void){WORD f,r=0,i;if(!owns())return 0;f=SaveIRQ();if(active&&!userPaused&&lastValid)for(i=0;i<4;i++)if(((lastFrame.volume>>(i*4))&15)!=15)r|=1<<i;RestoreIRQ(f);return r;}

/* Owner-task complete-state replacement. No mute, detach, allocation or second
   clock owner. Old prepared frames are cancelled under the existing IRQ lock;
   fixed callback data never references client memory. Future frames must be
   published promptly by this task. First state is emitted in this call. */
static TCFRAME liveFirst; /* fixed DLL DS, never a near pointer into caller SS */
WORD TCAPI TCLiveApply(TCFRAME FAR *source){WORD flags,i;
 if(!owns()||!source)return 1;liveFirst=*source;
 if(liveFirst.at||liveFirst.mask!=15||liveFirst.noise>7)return 3;
 for(i=0;i<3;i++)if(liveFirst.p[i]>1023||(!liveFirst.p[i]&&((liveFirst.volume>>(i*4))&15)!=15))return 3;
 if(timer&&(!active||userPaused||!HiMetric(9)))return 1;
 flags=SaveIRQ();head=tail=count=end=finished=error=0;restorePending=0;
 if(timer)epoch=lastStamp=HiRead();else epoch=0xffffffffUL;
 clockCycles=published=lastDue=0;sent=1;frame(&liveFirst);RestoreIRQ(flags);return 0;
}
