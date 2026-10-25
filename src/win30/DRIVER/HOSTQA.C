/* Executes the production state machine with mocked Win16 and I/O calls. */
#define SOUND_HOST
#define SOUND_TEST
#include <stdio.h>
#include "TSOUND.C"
static HTASK task=1;
static unsigned winflags,bits=0xa0,portWrites,programs,psgWrites;
static unsigned timerCreates,timerKills,activeTimer;
static unsigned yieldMode,irqFlags=0x0200U;
static unsigned long yields;
static DWORD clockTicks;
HTASK GetCurrentTask(void) { return task; }
unsigned GetWinFlags(void) { return winflags; }
unsigned SaveIRQ(void) { return irqFlags; }
void RestoreIRQ(unsigned flags) { (void)flags; }
void PsgByte(unsigned value) { (void)value; ++psgWrites; }
unsigned PitRead(void) { return bits; }
void PitGate(unsigned value) { bits=value; ++portWrites; }
void PitProgram(unsigned divisor) { (void)divisor; ++programs; }
unsigned CreateSystemTimer(unsigned n,FARPROC proc) {
    (void)n; (void)proc; ++timerCreates; activeTimer=timerCreates;
    return activeTimer;
}
void KillSystemTimer(unsigned t) {
    if (t==activeTimer) activeTimer=0;
    ++timerKills;
}
void TimerEntry(void) { Tick(); }
DWORD ReadBeepClock(void) { return clockTicks; }
void Yield(void) {
    ++yields;
    if (yieldMode!=4) {
        ++clockTicks;
        if (clockTicks>=BEEP_BIOS_DAY) clockTicks=0;
    }
    if (yieldMode==1) { task=2; CloseSound(); task=1; }
    if (yieldMode==2) StopSound();
    if (yieldMode==3 || yieldMode==4 || (yieldMode==5 && yields>1)) return;
    Tick();
}
#define CHECK(x) do { ++tests; if (!(x)) { \
    printf("FAIL line %u: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void) {
    unsigned tests=0,before,pbefore,t,token;
    TPINFO info;
    CHECK(TandyPsgLease(1,0)==0);
    CHECK(TandyPsgLease(2,0)==-16);
    CHECK(TandyPsgLease(1,3)==-16);
    CHECK(TandyPsgLease(1,1)==-1);
    CHECK(TandyPitVoice(1,440,1)==-1);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==-1);
    winflags=WF_PMODE;
    CHECK(OpenSound()==-1 && !owner);
    winflags=0;
    CHECK(DoBeep(0)==1 && !activeTimer && !timer);
    CHECK(psgWrites==7 && timerCreates==1 && timerKills==1);
    CHECK(OpenSound()==3);
    CHECK(OpenSound()==-1 && owner==1);
    CHECK(TandyPsgLease(1,0)==-3);
    before=psgWrites;
    CHECK(DoBeep(0)==0 && psgWrites==before);
    CHECK(TandyPsgLease(1,1)==0 && rawPsg);
    before=psgWrites;
    CHECK(TandyPsgLease(1,1)==0 && psgWrites==before);
    CHECK(DoBeep(0)==0 && psgWrites==before);
    CHECK(SetVoiceSound(1,440UL<<16,40)==-3);
    CHECK(SetVoiceNote(1,25,4,0)==-3);
    CHECK(StartSound()==-3 && !timer && !running);
    CHECK(TandyPitVoice(1,440,1)==0 && bits==0xa3);
    CHECK(TandyPitVoice(1,523,1)==0 && programs==2);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==-3);
    pbefore=portWrites;
    task=2;
    CHECK(OpenSound()==-1);
    CHECK(TandyPsgLease(1,1)==-1);
    CHECK(TandyPsgLease(1,2)==-1);
    CHECK(TandyPitVoice(1,0,0)==-1 && portWrites==pbefore);
    CHECK(StopSound()==-1);
    CloseSound(); CHECK(owner==1 && rawPsg && pitActive);
    CHECK(DoBeep(0)==0 && psgWrites==before);
    task=1;
    CHECK(TandyPitVoice(1,0,0)==0 && bits==0xa0 && !pitActive);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_QUERY)==0);
    CHECK(info.version==TP_VERSION && !info.token && info.fast);
    CHECK(info.features==(TP_FIXED|TP_EXCLUSIVE|TP_SUSPEND_SAFE));
    CHECK(info.minDivisor==TP_MIN_DIVISOR && info.maxDivisor==65535U);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==0);
    token=info.token;
    CHECK(token && rawPsg && info.fast(token,100,TP_SET)==-13);
    CHECK(info.fast(token,1000,TP_SET)==0 && bits==0xa3);
    CHECK(TandyPitVoice(1,440,1)==-3);
    pbefore=portWrites;
    CHECK(info.fast(token,1000,TP_SET)==0 && portWrites==pbefore);
    CHECK(info.fast(token+1,1000,TP_SET)==-1);
    CHECK(info.fast(token,0,TP_SUSPEND)==0 && bits==0xa0);
    CHECK(info.fast(token,1000,TP_SET)==-3);
    bits=0xa1;
    CHECK(info.fast(token,0,TP_REENTER)==-3 && bits==0xa1);
    bits=0xa0;
    CHECK(info.fast(token,0,TP_REENTER)==0);
    CHECK(info.fast(token,1000,TP_SET)==0 && bits==0xa3);
    CHECK(info.fast(token,0,TP_MUTE)==0 && bits==0xa0);
    task=2;
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_RELEASE)==-1);
    CHECK(leaseToken==token);
    task=1;
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_RELEASE)==0);
    CHECK(!info.token && rawPsg && owner==1);
    CHECK(info.fast(token,1000,TP_SET)==-1);
    before=psgWrites;
    CHECK(TandyPsgLease(1,2)==0 && !rawPsg && psgWrites==before+4);
    CHECK(TandyPsgLease(1,2)==-1);
    CHECK(SetVoiceSound(1,440UL<<16,40)==0 && queueManaged);
    CHECK(TandyPsgLease(1,1)==-3);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==-3);
    CHECK(StartSound()==0 && timer);
    yieldMode=1;
    CHECK(DoBeep(0)==1 && owner==1);
    yieldMode=0;
    CHECK(WaitSoundState(0)==0);
    CHECK(StopSound()==0 && !timer && !rawPsg && !queueManaged);
    CHECK(TandyPsgLease(1,1)==0);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==0);
    token=info.token;
    CHECK(info.fast(token,1000,TP_SET)==0);
    CloseSound();
    CHECK(!owner && !rawPsg && !leaseToken && bits==0xa0 && !timer);
    CHECK(info.fast(token,1000,TP_SET)==-1);
    CHECK(OpenSound()==3);
    CHECK(TandyPsgLease(1,1)==0);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_CLAIM)==0);
    CHECK(info.token && info.token!=token);
    CHECK(info.fast(token,1000,TP_SET)==-1);
    CHECK(TandyPitLease(TP_VERSION,&info,sizeof(info),TP_RELEASE)==0);
    CHECK(StopSound()==0);
    CHECK(TandyPitVoice(1,37,1)==0);
    bits=0xa1; pbefore=portWrites;
    CHECK(TandyPitVoice(1,440,1)==-3 && !pitActive);
    CHECK(portWrites==pbefore && bits==0xa1);
    CloseSound(); CHECK(bits==0xa1);
    bits=0xa0;
    CHECK(OpenSound()==3);
    CHECK(TandyPsgLease(1,1)==0);
    CHECK(TandyPitVoice(1,440,1)==0);
    CHECK(WEP(0)==1 && !owner && !rawPsg && !pitActive && bits==0xa0);
    CHECK(OpenSound()==3);
    CHECK(SetVoiceSound(1,440UL<<16,400)==0);
    CHECK(StartSound()==0);
    yieldMode=2;
    CHECK(DoBeep(0)==0 && !timer && !beepActive && !running);
    yieldMode=0; CloseSound();
    before=psgWrites; DebugFailNextTimer();
    CHECK(DoBeep(0)==0 && psgWrites==before && !timer);
    CHECK(DoBeep(0)==1 && !timer);
    t=timerCreates;
    CHECK(TandyPsgLease(1,0)==0 && timerCreates==t);
    /* Timer exists but never calls back: alert must unwind independently. */
    yieldMode=3; yields=0; before=psgWrites;
    CHECK(DoBeep(0)==0);
    CHECK(yields>0 && yields<=20 && psgWrites==before);
    CHECK(!timer && !activeTimer && !beepActive && !beepCallActive);
    CHECK(TandyPsgLease(1,0)==0);
    /* No callback and frozen BIOS clock still have a finite wait. */
    yieldMode=4; yields=0; before=psgWrites;
    CHECK(DoBeep(0)==0 && yields<BEEP_STALL_POLLS);
    CHECK(psgWrites==before && !timer && !activeTimer);
    CHECK(!beepActive && !beepCallActive && !owner);
    /* Lost callback after first tone must silence the sounded voice. */
    yieldMode=5; yields=0; DebugPsgClear();
    CHECK(DoBeep(0)==0 && DebugPsgCount()==4);
    CHECK(DebugPsgRead(3)==0x9f && !timer && !activeTimer);
    CHECK(!beepActive && !beepCallActive);
    /* Timeout restores borrowed queued PSG without touching owner/PIT. */
    CHECK(OpenSound()==3);
    CHECK(SetVoiceSound(1,440UL<<16,400)==0);
    CHECK(StartSound()==0); t=timer;
    CHECK(TandyPitVoice(1,440,1)==0); pbefore=portWrites;
    yieldMode=5; yields=0; DebugPsgClear();
    CHECK(DoBeep(0)==0);
    CHECK(owner==1 && running && queueManaged && !rawPsg);
    CHECK(timer==t && activeTimer==t && CountVoiceNotes(1)==1);
    CHECK(pitActive && bits==0xa3 && portWrites==pbefore);
    CHECK(DebugPsgRead(DebugPsgCount()-3)==0x8e);
    CHECK(DebugPsgRead(DebugPsgCount()-2)==0x0f);
    CHECK(DebugPsgRead(DebugPsgCount()-1)==0x94);
    CHECK(!beepActive && !beepCallActive);
    CHECK(StopSound()==0); CloseSound();
    /* The BIOS daily clock reset preserves the same finite deadline. */
    clockTicks=BEEP_BIOS_DAY-2; yieldMode=3; yields=0;
    CHECK(DoBeep(0)==0 && yields>0 && yields<=20);
    CHECK(!timer && !activeTimer && !owner);
    /* IF=0 cannot dispatch the callback. Refuse before timer or PSG I/O. */
    irqFlags=0; before=psgWrites; t=timerCreates; yields=0;
    CHECK(DoBeep(0)==0 && psgWrites==before && timerCreates==t && !yields);
    CHECK(!timer && !beepActive && !beepCallActive);
    irqFlags=0x0200U; yieldMode=0;
    task=0; before=psgWrites; t=timerCreates; yields=0;
    CHECK(DoBeep(0)==0 && psgWrites==before && timerCreates==t && !yields);
    CHECK(!timer && !beepActive && !beepCallActive && !owner);
    task=1;
    CHECK(DoBeep(0)==1 && !timer && !activeTimer);
    printf("PASS %u unified driver checks (mocked Win16 and ports)\n",tests);
    return 0;
}
