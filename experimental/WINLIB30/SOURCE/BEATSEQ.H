/* Foreground step producer. Published steps/gates are immutable; edits affect
   only upcoming unprepared steps. No flush or retrigger on a pattern edit. */
typedef struct {DWORD at,serial;WORD step,duration;} BSMARK;
static BSMARK bsMarks[32];static WORD bsHead,bsMarkCount;
static DWORD bsNext,bsGate,bsAt,bsSerial,bsEditAt;
static WORD bsDur,bsOutStep,bsPhase,bsCount,bsIndex;
static BYTE bsPattern[4][8],bsOutNotes[4];static DWORD bsMessages[4];
static void bsTrim(DWORD now){while(bsMarkCount>1&&(LONG)(now-bsMarks[(bsHead+1)&31].at)>=0){bsHead=(bsHead+1)&31;bsMarkCount--;}}

static DWORD bsMessage(WORD lane,WORD note,int on){return (on?0x90:0x80)|channels[lane]|((DWORD)note<<8)|((DWORD)(on?(lane==3?76:96):0)<<16);}
static void stop(void){playing=0;if(!ACClose())status="Stopped / muted";else{status="Close pending";failures++;}redraw();}
static void bsCopy(void){WORD r,c;for(r=0;r<4;r++)for(c=0;c<8;c++)bsPattern[r][c]=pattern[r][c];bsDur=(WORD)(30000UL/tempo);}
static void bsEdit(void){if(playing){bsCopy();bsEditAt=bsNext;status="Edit queued ahead";}}
static int bsPump(void){WORD r,bound=0,k;DWORD now,through;
 if(!acHandle||ACClock())return 0;now=acClockValue;bsTrim(now);
 while(bound<64){
  if(bsIndex==bsCount){bsIndex=bsCount=0;bsAt=bsPhase==0?bsGate:bsNext;if((LONG)(bsAt-now)>1000L)break;
   if(bsPhase==0){for(r=0;r<4;r++)if(bsOutNotes[r]){bsMessages[bsCount++]=bsMessage(r,bsOutNotes[r],0);bsOutNotes[r]=0;}bsPhase=1;}
   else{
    if(bsMarkCount==32)return 0;bsOutStep=(bsOutStep+1)%8;k=(bsHead+bsMarkCount)&31;
    bsMarks[k].at=bsNext;bsMarks[k].step=bsOutStep;bsMarks[k].duration=bsDur;bsMarks[k].serial=++bsSerial;bsMarkCount++;
    for(r=0;r<4;r++){bsOutNotes[r]=bsPattern[r][bsOutStep];if(bsOutNotes[r])bsMessages[bsCount++]=bsMessage(r,bsOutNotes[r],1);}
    bsGate=bsNext+(DWORD)(bsDur*3U/4U);bsNext+=bsDur;bsPhase=0;
   }
  }
  while(bsIndex<bsCount&&bound<64){r=ACPush(bsAt,bsMessages[bsIndex]);if(r==MD_FULL)goto publish;if(r)return 0;bsIndex++;bound++;}
  if(!bsCount)bound++;
 }
 publish:if(bsIndex<bsCount&&bsAt==0)return 1;through=bsIndex<bsCount?bsAt:(bsPhase==0?bsGate:bsNext);if(through)through--;
 if(ACPublish(through,0))return 0;
 bsTrim(now);if(bsMarkCount){step=bsMarks[bsHead].step;stepsPlayed=bsMarks[bsHead].serial>32767UL?32767:(int)bsMarks[bsHead].serial;}
 status=bsEditAt&&(LONG)(now-bsEditAt)<0?"Edit queued ahead":"Playing deadline MIDI";redraw();return 1;
}
static void play(void){WORD r;if(!opened||!timerReady)return;stop();step=0;stepsPlayed=1;
 if(ACOpen()){failures++;status="Sound busy; retry";return;}
 bsCopy();bsHead=bsMarkCount=bsCount=bsIndex=0;bsSerial=bsNext=bsGate=bsEditAt=0;bsOutStep=7;bsPhase=1;
 for(r=0;r<4;r++)bsOutNotes[r]=0;
 if(ACPush(0,0xc0UL)||!bsPump()){failures++;stop();status="Queue error";return;}playing=1;
}
