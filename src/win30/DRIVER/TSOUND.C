/* Bounded Windows 3.0 / 8086 PSG sound driver. */
#define WINVER 0x0300
#define NOSOUND
#include <windows.h>
#define SAPI FAR PASCAL _loadds
#define MAXQ 64
#define DVNA (-1)
#define OFM (-2)
#define ACTIVE (-3)
#define QFULL (-4)
#define BADNOTE (-5)
#define BADLEN (-6)
#define BADDOTS (-7)
#define BADTEMPO (-8)
#define BADMODE (-10)
#define BADSHAPE (-11)
#define BADPITCH (-12)
#define BADFREQ (-13)
#define BADDUR (-14)
#define BADSOURCE (-15)
#define BADSTATE (-16)
#ifdef SOUND_TEST
#define BEEPLOG 32
#endif
#define BEEP_PERIOD1 125
#define BEEP_PERIOD2 149
#define BEEP_VOLUME 4
#define BEEP_WAIT_TICKS 20UL
#define BEEP_BIOS_DAY 0x001800b0UL
#define BEEP_STALL_POLLS 65535U
extern unsigned FAR PASCAL CreateSystemTimer(unsigned, FARPROC);
extern void FAR PASCAL KillSystemTimer(unsigned);
extern void FAR PASCAL TimerEntry(void);
extern unsigned _cdecl SaveIRQ(void);
extern void _cdecl RestoreIRQ(unsigned);
extern void _cdecl PsgByte(unsigned);
typedef struct { unsigned tone, total, sounded; } NOTE;
typedef struct {
    NOTE notes[MAXQ];
    unsigned head, tail, count, capacity, threshold;
    unsigned tempo, mode, pitch, volume;
    unsigned active, silenced, clocked;
    unsigned long elapsed;
} VOICE;
static VOICE voices[3];
static HTASK owner;
static unsigned rawPsg, queueManaged;
#include "PITCORE.H"
static unsigned timer;
static volatile unsigned running;
static unsigned timerEpoch;
static unsigned timerStarting, timerStopping;
static unsigned present;
static unsigned events;

static volatile unsigned beepActive, beepArmed, beepPhase;
static volatile unsigned beepDone, beepResult, beepVoice;
static volatile unsigned beepCallActive;
#ifdef SOUND_TEST
static volatile unsigned beepRejects;
static unsigned failTimerNext;
static unsigned psgLogHead, psgLogCount;
static unsigned char psgLog[BEEPLOG];
static unsigned psgLogTick[BEEPLOG];
static volatile unsigned testHoldBeepReturn, testBeepReturnWaiting;
static unsigned testHoldTicks;
static volatile unsigned testHoldBeepActive;
static unsigned testHoldBeepActiveTicks;
#endif
/* Independent resident counter permits callback lifecycle smoke tests. */
volatile unsigned TimerCount;
static unsigned freqtab[12] = {
    4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902
};
/* Called with interrupts disabled by mute() or toneout(). */
static void psgbyte(unsigned value) {
#ifdef SOUND_TEST
    unsigned slot;
    if (psgLogCount<BEEPLOG) {
        slot=(psgLogHead+psgLogCount)&(BEEPLOG-1);
        ++psgLogCount;
    } else {
        slot=psgLogHead;
        psgLogHead=(psgLogHead+1)&(BEEPLOG-1);
    }
    psgLog[slot]=(unsigned char)value;
    psgLogTick[slot]=TimerCount;
#endif
    PsgByte(value);
}
static void mute(unsigned v) {
    unsigned flags;
    flags=SaveIRQ();
    psgbyte(0x9f | (v << 5));
    RestoreIRQ(flags);
}
static void muteall(void) {
    unsigned i;
    if (present) for (i=0;i<4;++i)
        if (!(beepActive && beepArmed && i==beepVoice)) mute(i);
}
static void toneout(unsigned v,unsigned tone) {
    unsigned p,flags;
    flags=SaveIRQ(); p=tone & 1023;
    if (!p) psgbyte(0x9f | (v << 5));
    else {
        psgbyte(0x80 | (v<<5) | (p&15));
        psgbyte(p>>4);
        psgbyte(0x90 | (v<<5) | (tone>>12));
    }
    RestoreIRQ(flags);
}
static void musicmute(unsigned v) {
    if (beepActive && beepArmed && v==beepVoice) return;
    mute(v);
}
static void musictoneout(unsigned v,unsigned tone) {
    if (beepActive && beepArmed && v==beepVoice) return;
    toneout(v,tone);
}
static int owns(void) { return owner && owner==GetCurrentTask(); }
static int validvoice(int v) { return v>=1 && v<=3; }
static int psgready(void) {
#ifdef SOUND_HOST
    return !(GetWinFlags() & WF_PMODE);
#else
    if (GetWinFlags() & WF_PMODE) return 0;
    return *(unsigned char FAR *)0xfc000000UL == 0x21;
#endif
}
/* Independent BIOS clock, including early USER initialization. No USER
   entry point is called by the alert path. Save flags around the two words. */
static DWORD beepclock(void) {
#ifdef SOUND_HOST
    extern DWORD _cdecl ReadBeepClock(void);
    return ReadBeepClock();
#else
    unsigned flags;
    DWORD value;
    flags=SaveIRQ(); value=*(DWORD FAR *)0x0040006cUL;
    RestoreIRQ(flags); return value;
#endif
}
static unsigned choosebeepvoice(void) {
    unsigned i; VOICE *v;
    for (i=0;i<3;++i) {
        v=&voices[i];
        if (!v->active || v->silenced || !v->count ||
            !v->notes[v->head].tone) return i;
    }
    return 0;
}
static void restorebeepvoice(unsigned i) {
    VOICE *v=&voices[i];
    if (v->count && v->active && !v->silenced)
        toneout(i,v->notes[v->head].tone);
}
static void cancelbeep(void) {
    if (beepActive && beepArmed) mute(beepVoice);
    beepActive=0; beepArmed=0; beepPhase=0;
    if (beepCallActive && !beepDone) {
        beepDone=1; beepResult=0;
    }
#ifdef SOUND_TEST
    testHoldBeepActive=0; testHoldBeepActiveTicks=0;
#endif
}
static unsigned period(unsigned hz) {
    unsigned long p;
    if (!hz) return 0;
    p=(111861UL+(hz/2))/hz;
    if (p<1 || p>1023) return 0xffff;
    return (unsigned)p;
}
static int enqueue(int voice,unsigned p,unsigned total,unsigned sounded) {
    VOICE *v; unsigned flags;
    if (!owns() || !validvoice(voice)) return DVNA;
    if (!total) return BADDUR;
    if (rawPsg) return ACTIVE;
    v=&voices[voice-1];
    flags=SaveIRQ();
    if (v->count>=v->capacity) { RestoreIRQ(flags); return QFULL; }
    v->notes[v->tail].tone=p ? p | (v->volume<<12) : 0;
    v->notes[v->tail].total=total;
    v->notes[v->tail].sounded=sounded;
    v->tail=(v->tail+1)%MAXQ;
    ++v->count; queueManaged=1;
    RestoreIRQ(flags);
    return 0;
}
/* Called ONLY through register-saving fixed assembly shim. No Windows,
   DOS, allocation, or movable code is used by this routine. */
void _cdecl Tick(void) {
    unsigned i,budget,any=0,flags; VOICE *v; NOTE *n;
    ++TimerCount;
    flags=SaveIRQ();
#ifdef SOUND_TEST
    if (beepDone && testHoldBeepReturn && testHoldTicks &&
        !--testHoldTicks) testHoldBeepReturn=0;
#endif
    if (beepActive) {
#ifdef SOUND_TEST
        if (testHoldBeepActive) {
            if (testHoldBeepActiveTicks &&
                !--testHoldBeepActiveTicks) testHoldBeepActive=0;
        } else {
#endif
        if (beepPhase==0) {
            toneout(beepVoice,
                    BEEP_PERIOD1 | (BEEP_VOLUME<<12));
            beepPhase=1; beepArmed=1;
        } else if (beepPhase==1) {
            toneout(beepVoice,
                    BEEP_PERIOD2 | (BEEP_VOLUME<<12));
            beepPhase=2;
        } else {
            mute(beepVoice); restorebeepvoice(beepVoice);
            beepActive=0; beepArmed=0; beepPhase=0;
            beepDone=1; beepResult=1;
        }
#ifdef SOUND_TEST
        }
#endif
    }
    RestoreIRQ(flags);
    if (!running) return;
    for (i=0;i<3;++i) {
        v=&voices[i];
        if (v->clocked) v->elapsed+=21970UL; /* 54.925 ms / 2.5 ms */
        budget=4; /* bounded catch-up for very short notes */
        while (v->count && budget--) {
            n=&v->notes[v->head];
            if (!v->active) {
                musictoneout(i,n->tone);
                v->active=1; v->silenced=0; v->clocked=1;
            }
            if (!v->silenced && v->elapsed >= (unsigned long)n->sounded*1000UL) {
                musicmute(i); v->silenced=1;
            }
            if (v->elapsed < (unsigned long)n->total*1000UL) break;
            v->elapsed-=(unsigned long)n->total*1000UL;
            musicmute(i); v->head=(v->head+1)%MAXQ; --v->count;
            v->active=0;
            if (v->threshold && v->count==v->threshold-1) events|=1<<i;
        }
        if (v->count) any=1;
        else { v->active=0; v->clocked=0; v->elapsed=0; }
    }
    if (!any) { running=0; muteall(); }
}
#include "PITLEASE.H"
static void stopinternal(void) {
    unsigned i,flags,t,release;
    flags=SaveIRQ(); running=0; t=timer; timer=0;
    ++timerEpoch; timerStarting=0; release=0;
    if (t && !timerStopping) { timerStopping=1; release=1; }
    cancelbeep();
    for (i=0;i<3;++i) {
        voices[i].head=voices[i].tail=voices[i].count=0;
        voices[i].active=voices[i].silenced=voices[i].clocked=0;
        voices[i].elapsed=0;
    }
    events=0; rawPsg=queueManaged=0;
    leaseRevoke(); pitStop(); muteall(); RestoreIRQ(flags);
    if (t && release) {
        KillSystemTimer(t);
        flags=SaveIRQ(); timerStopping=0; RestoreIRQ(flags);
    }
}
static unsigned ensuretimer(void) {
    unsigned flags,epoch,fail,t,installed,kill;
    flags=SaveIRQ();
    if (timer && !timerStopping) {
        t=timer; RestoreIRQ(flags); return t;
    }
    if (timerStarting || timerStopping) {
        RestoreIRQ(flags); return 0;
    }
    timerStarting=1; epoch=timerEpoch;
#ifdef SOUND_TEST
    fail=failTimerNext; failTimerNext=0;
#else
    fail=0;
#endif
    RestoreIRQ(flags);
    t=fail ? 0 : CreateSystemTimer(55,(FARPROC)TimerEntry);
    flags=SaveIRQ();
    if (timerEpoch==epoch) {
        timerStarting=0;
        if (t && !timer && !timerStopping) timer=t;
    }
    installed=(timerEpoch==epoch && !timerStopping) ? timer : 0;
    kill=t && (timerEpoch!=epoch || timer!=t);
    RestoreIRQ(flags);
    if (kill) KillSystemTimer(t);
    return installed;
}
static void retiretimer(unsigned expected) {
    unsigned flags,t;
    flags=SaveIRQ();
    if (!expected || timer!=expected || running || beepActive ||
        timerStarting || timerStopping) {
        RestoreIRQ(flags); return;
    }
    t=timer; timer=0; timerStopping=1; ++timerEpoch;
    RestoreIRQ(flags);
    KillSystemTimer(t);
    flags=SaveIRQ(); timerStopping=0; RestoreIRQ(flags);
}
int SAPI OpenSound(void) {
    unsigned i;
    if (owner || !psgready()) return DVNA;
    owner=GetCurrentTask(); if (!owner) return DVNA;
    present=1; stopinternal();
    for (i=0;i<3;++i) {
        voices[i].capacity=32; voices[i].threshold=0;
        voices[i].tempo=120; voices[i].mode=0;
        voices[i].pitch=0; voices[i].volume=4;
    }
    return 3;
}
void SAPI CloseSound(void) {
    if (!owns()) return;
    stopinternal(); owner=0;
}
int SAPI SetVoiceQueueSize(int voice,int bytes) {
    VOICE *v; unsigned flags;
    if (!owns() || !validvoice(voice)) return DVNA;
    if (bytes<0 || bytes>MAXQ*6) return OFM;
    v=&voices[voice-1]; flags=SaveIRQ();
    if (v->count || v->active) { RestoreIRQ(flags); return ACTIVE; }
    v->capacity=(unsigned)bytes/6; RestoreIRQ(flags); return 0;
}
int SAPI SetVoiceNote(int voice,int note,int length,int dots) {
    unsigned total,sounded,hz,p,n,oct; unsigned long d,part; VOICE *v;
    if (!owns() || !validvoice(voice)) return DVNA;
    if (note<0 || note>84) return BADNOTE;
    if (length<1 || length>64) return BADLEN;
    if (dots<0 || dots>15) return BADDOTS;
    v=&voices[voice-1];
    d=96000UL/((unsigned)length*v->tempo); part=d;
    while (dots--) { part>>=1; d+=part; }
    if (!d || d>65535UL) return BADLEN;
    total=(unsigned)d; sounded=total; p=0;
    if (note) {
        n=((unsigned)note-1+v->pitch)%84; oct=n/12;
        hz=freqtab[n%12];
        if (oct<6) { n=6-oct; hz=(hz+(1<<(n-1)))>>n; }
        p=period(hz); if (p==0xffff) return BADFREQ;
        if (v->mode==0) sounded=(unsigned)((d*7)/8);
        else if (v->mode==2) sounded=(unsigned)((d*3)/4);
        if (!sounded) sounded=1;
    } else sounded=0;
    return enqueue(voice,p,total,sounded);
}
int SAPI SetVoiceSound(int voice,DWORD frequency,int duration) {
    unsigned p,hz;
    if (!owns() || !validvoice(voice)) return DVNA;
    if (duration<=0) return BADDUR;
    /* Restricted subset accepts whole-Hz inputs, rather than discarding
       fractional pitch while returning success. */
    if (LOWORD(frequency)) return BADFREQ;
    hz=HIWORD(frequency); p=period(hz);
    if (p==0xffff) return BADFREQ;
    return enqueue(voice,p,(unsigned)duration,hz ? (unsigned)duration : 0);
}
int SAPI SetVoiceAccent(int voice,int tempo,int volume,int mode,int pitch) {
    VOICE *v;
    if (!owns() || !validvoice(voice)) return DVNA;
    if (tempo<32 || tempo>255) return BADTEMPO;
    if (mode<0 || mode>2) return BADMODE;
    if (pitch<0 || pitch>83) return BADPITCH;
    v=&voices[voice-1];
    v->tempo=tempo; v->mode=mode; v->pitch=pitch;
    v->volume=(unsigned)volume==65535U ? 4 : 15-((unsigned)volume>>12);
    return 0;
}
int SAPI StartSound(void) {
    unsigned flags,i,any=0,t;
    if (!owns()) return DVNA;
    flags=SaveIRQ();
    if (rawPsg) { RestoreIRQ(flags); return ACTIVE; }
    if (running) { RestoreIRQ(flags); return 0; }
    for (i=0;i<3;++i) if (voices[i].count) any=1;
    RestoreIRQ(flags);
    if (!any) return 0;
    t=ensuretimer();
    if (!t) { muteall(); return OFM; }
    flags=SaveIRQ();
    if (timer!=t || timerStopping) {
        RestoreIRQ(flags); return OFM;
    }
    running=1; RestoreIRQ(flags); return 0;
}
int SAPI StopSound(void) {
    if (!owns()) return DVNA;
    stopinternal(); return 0;
}
int SAPI CountVoiceNotes(int voice) {
    if (!owns() || !validvoice(voice)) return 0;
    return voices[voice-1].count;
}
int SAPI SetVoiceThreshold(int voice,int count) {
    if (!owns() || !validvoice(voice)) return DVNA;
    if (count<0 || count>MAXQ) return BADSTATE;
    voices[voice-1].threshold=count; return 0;
}
int FAR * SAPI GetThresholdEvent(void) {
    return owns() ? (int FAR *)&events : (int FAR *)0;
}
int SAPI GetThresholdStatus(void) {
    unsigned flags,value;
    if (!owns()) return 0;
    flags=SaveIRQ(); value=events; events=0; RestoreIRQ(flags); return value;
}
int SAPI WaitSoundState(int state) {
    unsigned i,all,any,n,flags,playing;
    if (!owns()) return DVNA;
    if (state<0 || state>2) return BADSTATE;
    for (;;) {
        flags=SaveIRQ(); all=1; any=0; n=0;
        for (i=0;i<3;++i) {
            n+=voices[i].count;
            if (voices[i].count<voices[i].threshold) any=i+1;
            else all=0;
        }
        playing=running; RestoreIRQ(flags);
        if (state==0 && !n) return 0;
        if (state==1 && any) return any;
        if (state==2 && all) return 0;
        if (!playing) return BADSTATE;
        Yield();
        if (!owns()) return DVNA;
    }
}
int SAPI SyncAllVoices(void) { return BADSTATE; }
int SAPI SetVoiceEnvelope(int v,int shape,int repeat) { return BADSHAPE; }
int SAPI SetSoundNoise(int source,int duration) { return BADSOURCE; }
/* DDK ordinal 17 has one cleanup word, ignores it, and returns AX 0/1. */
int SAPI DoBeep(unsigned ignored) {
    unsigned flags,voice,t,done,result,armed,stalled=0,timedout;
    DWORD started,now,last,elapsed;
    (void)ignored;
    /* Yield needs a real task; early taskless initialization has none. */
    if (!GetCurrentTask()) return 0;
    if (!present) {
        if (!psgready()) return 0;
        present=1;
    }
    flags=SaveIRQ();
    /* A SYSTEM callback cannot run while the caller masks interrupts.
       Never enable them on the caller's behalf or wait for that callback. */
    if (!(flags & 0x0200U)) { RestoreIRQ(flags); return 0; }
    if (beepCallActive) {
#ifdef SOUND_TEST
        ++beepRejects;
#endif
        RestoreIRQ(flags); return 0;
    }
    /* OpenSound also protects clients writing the PSG directly. Only
       explicitly published legacy queues permit voice borrowing. */
    if (rawPsg || (owner && !queueManaged) ||
        timerStarting || timerStopping) {
        RestoreIRQ(flags); return 0;
    }
    voice=choosebeepvoice();
    beepVoice=voice; beepPhase=0; beepArmed=0;
    beepCallActive=1; beepDone=0; beepResult=0; beepActive=1;
#ifdef SOUND_TEST
    testBeepReturnWaiting=0;
#endif
    RestoreIRQ(flags);
    started=last=beepclock();
    t=ensuretimer();
    if (!t) {
        flags=SaveIRQ();
        if (beepActive) cancelbeep();
        beepCallActive=0; beepDone=0; beepResult=0;
        RestoreIRQ(flags); return 0;
    }
    flags=SaveIRQ();
    if (timer!=t || timerStopping) {
        if (beepActive) cancelbeep();
        beepCallActive=0; beepDone=0; beepResult=0;
        RestoreIRQ(flags); return 0;
    }
    RestoreIRQ(flags);
    for (;;) {
        now=beepclock();
        if (now!=last) { last=now; stalled=0; }
        else if (stalled<BEEP_STALL_POLLS) ++stalled;
        elapsed=now>=started ? now-started : BEEP_BIOS_DAY-started+now;
        timedout=elapsed>=BEEP_WAIT_TICKS ||
                 stalled==BEEP_STALL_POLLS;
        flags=SaveIRQ(); done=beepDone; result=beepResult;
        if (!done && timedout) {
            /* Only this alert is cancelled. Queued music, the task owner,
               and separate PIT leases keep their existing state. */
            voice=beepVoice; armed=beepActive && beepArmed;
            cancelbeep();
            if (armed && queueManaged) restorebeepvoice(voice);
            done=beepDone; result=beepResult;
        }
#ifdef SOUND_TEST
        if (done && testHoldBeepReturn && !timedout) {
            testBeepReturnWaiting=1;
            RestoreIRQ(flags); Yield(); continue;
        }
        if (timedout) testHoldBeepReturn=0;
        testBeepReturnWaiting=0;
#endif
        if (done) {
            beepCallActive=0; beepDone=0; beepResult=0;
        }
        RestoreIRQ(flags);
        if (done) break;
        Yield();
    }
    retiretimer(t);
    return (int)result;
}
/* Private raw PSG lease. OpenSound is still the cross-task authority.
   Repeated CLAIM is a side-effect-free ownership check for timed clients. */
int SAPI TandyPsgLease(unsigned version,unsigned operation) {
    unsigned flags,i;
    int result=0;
    if (version!=1 || operation>2) return BADSTATE;
    if (operation && !owns()) return DVNA;
    flags=SaveIRQ();
    if (!operation) result=(owner || beepCallActive) ? ACTIVE : 0;
    else if (operation==1) {
        if (!rawPsg) {
            if (running || beepCallActive || timerStarting || timerStopping)
                result=ACTIVE;
            for (i=0;i<3;++i) if (voices[i].count) result=ACTIVE;
            if (!result) { rawPsg=1; queueManaged=0; }
        }
    } else if (!rawPsg) result=DVNA;
    else { rawPsg=0; queueManaged=0; muteall(); }
    RestoreIRQ(flags);
    return result;
}
/* Optional PIT2 voice shares the same task owner; it never steals a fast
   prepared lease. PSG legacy queues and this separate voice may coexist. */
int SAPI TandyPitVoice(unsigned version,unsigned hz,unsigned level) {
    unsigned flags;
    int result;
    if (!owns()) return DVNA;
    flags=SaveIRQ();
    result=leaseToken ? ACTIVE : pitSet(version,hz,level);
    RestoreIRQ(flags);
    return result;
}
int SAPI MyOpenSound(void) { return OpenSound(); }
int SAPI WEP(int reason) { stopinternal(); owner=0; return 1; }
/* These two counters are part of the existing resident diagnostic ABI. */
unsigned SAPI DebugTicks(void) { return TimerCount; }
unsigned SAPI DebugTimer(void) { return timer; }
#ifdef SOUND_TEST
unsigned SAPI DebugPsgCount(void) {
    unsigned flags,value;
    flags=SaveIRQ(); value=psgLogCount; RestoreIRQ(flags); return value;
}
unsigned SAPI DebugPsgRead(int index) {
    unsigned flags,value;
    flags=SaveIRQ(); value=0xffffU;
    if (index>=0 && (unsigned)index<psgLogCount)
        value=psgLog[(psgLogHead+(unsigned)index)&(BEEPLOG-1)];
    RestoreIRQ(flags); return value;
}
unsigned SAPI DebugPsgTick(int index) {
    unsigned flags,value;
    flags=SaveIRQ(); value=0xffffU;
    if (index>=0 && (unsigned)index<psgLogCount)
        value=psgLogTick[(psgLogHead+(unsigned)index)&(BEEPLOG-1)];
    RestoreIRQ(flags); return value;
}
void SAPI DebugPsgClear(void) {
    unsigned flags;
    flags=SaveIRQ(); psgLogHead=psgLogCount=0; RestoreIRQ(flags);
}
unsigned SAPI DebugBeepActive(void) {
    unsigned flags,value;
    flags=SaveIRQ(); value=beepActive; RestoreIRQ(flags); return value;
}
unsigned SAPI DebugBeepRejects(void) { return beepRejects; }
unsigned long SAPI DebugVoiceElapsed(int voice) {
    unsigned flags; unsigned long value;
    if (!validvoice(voice)) return 0;
    flags=SaveIRQ(); value=voices[voice-1].elapsed;
    RestoreIRQ(flags); return value;
}
void SAPI DebugHoldBeepReturn(void) {
    unsigned flags;
    flags=SaveIRQ(); testHoldBeepReturn=1;
    testBeepReturnWaiting=0; testHoldTicks=40;
    RestoreIRQ(flags);
}
void SAPI DebugHoldBeepActive(void) {
    unsigned flags;
    flags=SaveIRQ(); testHoldBeepActive=1;
    testHoldBeepActiveTicks=20; RestoreIRQ(flags);
}
unsigned SAPI DebugBeepReturnWaiting(void) {
    unsigned flags,value;
    flags=SaveIRQ(); value=testBeepReturnWaiting;
    RestoreIRQ(flags); return value;
}
void SAPI DebugReleaseBeepReturn(void) {
    unsigned flags;
    flags=SaveIRQ(); testHoldBeepReturn=0; testHoldTicks=0;
    RestoreIRQ(flags);
}
void SAPI DebugFailNextTimer(void) {
    unsigned flags;
    flags=SaveIRQ(); failTimerNext=1; RestoreIRQ(flags);
}
unsigned SAPI DebugTestBeginBeep(void) {
    unsigned flags;
    flags=SaveIRQ();
    if (beepActive || beepCallActive || timerStarting || timerStopping) {
        RestoreIRQ(flags); return 0;
    }
    beepVoice=choosebeepvoice(); beepPhase=0; beepArmed=0;
    beepDone=0; beepResult=0; beepActive=1;
    RestoreIRQ(flags); return 1;
}
void SAPI DebugTestTick(void) { Tick(); }
void SAPI DebugTestEndBeep(void) {
    unsigned flags;
    flags=SaveIRQ(); cancelbeep();
    beepCallActive=0; beepDone=0; beepResult=0;
    RestoreIRQ(flags);
}
#endif
