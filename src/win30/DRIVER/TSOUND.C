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
static unsigned timer;
static volatile unsigned running;
static unsigned present;
static unsigned events;
/* Independent resident counter permits callback lifecycle smoke tests. */
volatile unsigned TimerCount;
static unsigned freqtab[12] = {
    4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902
};
static void mute(unsigned v) { PsgByte(0x9f | (v << 5)); }
static void muteall(void) {
    unsigned i;
    if (present) for (i=0;i<4;++i) mute(i);
}
static void toneout(unsigned v,unsigned tone) {
    unsigned p=tone & 1023;
    if (!p) { mute(v); return; }
    PsgByte(0x80 | (v<<5) | (p&15));
    PsgByte(p>>4);
    PsgByte(0x90 | (v<<5) | (tone>>12));
}
static int owns(void) { return owner && owner==GetCurrentTask(); }
static int validvoice(int v) { return v>=1 && v<=3; }
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
    v=&voices[voice-1];
    flags=SaveIRQ();
    if (v->count>=v->capacity) { RestoreIRQ(flags); return QFULL; }
    v->notes[v->tail].tone=p ? p | (v->volume<<12) : 0;
    v->notes[v->tail].total=total;
    v->notes[v->tail].sounded=sounded;
    v->tail=(v->tail+1)%MAXQ;
    ++v->count;
    RestoreIRQ(flags);
    return 0;
}
/* Called ONLY through register-saving fixed assembly shim. No Windows,
   DOS, allocation, or movable code is used by this routine. */
void _cdecl Tick(void) {
    unsigned i,budget,any=0; VOICE *v; NOTE *n;
    ++TimerCount;
    if (!running) return;
    for (i=0;i<3;++i) {
        v=&voices[i];
        if (v->clocked) v->elapsed+=21970UL; /* 54.925 ms / 2.5 ms */
        budget=4; /* bounded catch-up for very short notes */
        while (v->count && budget--) {
            n=&v->notes[v->head];
            if (!v->active) {
                toneout(i,n->tone); v->active=1; v->silenced=0; v->clocked=1;
            }
            if (!v->silenced && v->elapsed >= (unsigned long)n->sounded*1000UL) {
                mute(i); v->silenced=1;
            }
            if (v->elapsed < (unsigned long)n->total*1000UL) break;
            v->elapsed-=(unsigned long)n->total*1000UL;
            mute(i); v->head=(v->head+1)%MAXQ; --v->count; v->active=0;
            if (v->threshold && v->count==v->threshold-1) events|=1<<i;
        }
        if (v->count) any=1;
        else { v->active=0; v->clocked=0; v->elapsed=0; }
    }
    if (!any) { running=0; muteall(); }
}
static void stopinternal(void) {
    unsigned i,flags,t;
    flags=SaveIRQ(); running=0; t=timer; timer=0;
    for (i=0;i<3;++i) {
        voices[i].head=voices[i].tail=voices[i].count=0;
        voices[i].active=voices[i].silenced=voices[i].clocked=0;
        voices[i].elapsed=0;
    }
    events=0; muteall(); RestoreIRQ(flags);
    if (t) KillSystemTimer(t);
}
int SAPI OpenSound(void) {
    unsigned i;
    if (owner || (GetWinFlags() & WF_PMODE)) return DVNA;
    if (*(unsigned char FAR *)0xfc000000UL != 0x21) return DVNA;
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
    if (running) return 0;
    for (i=0;i<3;++i) if (voices[i].count) any=1;
    if (!any) return 0;
    if (!timer) {
        t=CreateSystemTimer(55,(FARPROC)TimerEntry);
        if (!t) { muteall(); return OFM; }
        timer=t;
    }
    flags=SaveIRQ(); running=1; RestoreIRQ(flags); return 0;
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
int SAPI DoBeep(unsigned ignored) { return 0; /* unsupported, not success */ }
int SAPI MyOpenSound(void) { return OpenSound(); }
int SAPI WEP(int reason) { stopinternal(); owner=0; return 1; }
/* Test-only state queries; no hardware or scheduler side effects. */
unsigned SAPI DebugTicks(void) { return TimerCount; }
unsigned SAPI DebugTimer(void) { return timer; }
