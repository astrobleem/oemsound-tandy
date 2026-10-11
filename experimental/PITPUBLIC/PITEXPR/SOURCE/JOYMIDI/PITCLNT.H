/* Optional PIT2 v1 client. Include windows.h first. C89 /G0 friendly.
 * Caller owns the SOUND lease through OpenSound or its MIDI adapter.
 * Off before reset/close. Forget after losing ownership. Any failure
 * requires caller panic/reset/close cleanup; never fall back to ports.
 * This is binary on/off: no normal volume or independent speech sharing.
 */
#ifndef TANDY_PITCLIENT_H
#define TANDY_PITCLIENT_H
#define PITCLIENT_VERSION 1
#define PITCLIENT_UNAVAILABLE (-16)
#define PITCLIENT_NOT_OWNED (-1)
#define PITCLIENT_BAD_NOTE (-13)
typedef int (FAR PASCAL *PITCLIENTPROC)(unsigned,unsigned,unsigned);
typedef struct { PITCLIENTPROC proc; unsigned note; } PIT_CLIENT;
static const unsigned pitClientHz[52]={
    110,117,123,131,139,147,156,165,175,185,
    196,208,220,233,247,262,277,294,311,330,
    349,370,392,415,440,466,494,523,554,587,
    622,659,698,740,784,831,880,932,988,1047,
    1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,
    1976,2093
};
static void PitClientForget(PIT_CLIENT *p) { p->note=0; }
static int PitClientDiscover(PIT_CLIENT *p)
{
    HMODULE sound;
    /* Discovery must happen while no client note is active. */
    if(p->note)return PITCLIENT_NOT_OWNED;
    sound=GetModuleHandle("SOUND");
    p->proc=sound?(PITCLIENTPROC)GetProcAddress(sound,"TANDYPITVOICE"):0;
    return p->proc?0:PITCLIENT_UNAVAILABLE;
}
static int PitClientOff(PIT_CLIENT *p,int owned)
{
    int result;
    if(!owned){PitClientForget(p);return PITCLIENT_NOT_OWNED;}
    if(!p->proc){PitClientForget(p);return PITCLIENT_UNAVAILABLE;}
    result=p->proc(PITCLIENT_VERSION,0,0);
    /* A failed call makes the cache unusable; caller must release. */
    PitClientForget(p);return result;
}
static int PitClientNote(PIT_CLIENT *p,int owned,unsigned note)
{
    int result;
    if(!owned){PitClientForget(p);return PITCLIENT_NOT_OWNED;}
    if(!p->proc){PitClientForget(p);return PITCLIENT_UNAVAILABLE;}
    if(note<45 || note>96){
        PitClientForget(p);return PITCLIENT_BAD_NOTE;
    }
    if(p->note==note)return 0;
    result=p->proc(PITCLIENT_VERSION,pitClientHz[note-45],1);
    p->note=result?0:note;return result;
}
#endif
