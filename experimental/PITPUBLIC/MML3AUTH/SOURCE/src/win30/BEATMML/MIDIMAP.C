/* Experimental completion of the repository's midiMessage handler.
   Direct task-context Win3.0 harness; not a registered MMSYSTEM device. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include "PERIODS.H"
#include "INSTR.H"
#define MAPI FAR PASCAL _loadds
extern void _cdecl PsgByte(unsigned);
extern void _cdecl PitSilence(void);
static HTASK owner;
static WORD notesOn[3],channelsOn[3],periodOn[3],volumeOn[3],age[3];
static WORD noiseNote,noiseVolume,counter,steals,writes;
static int hasSound,pitEnabled;
static WORD pitNote;
static PITPROC pitVoice;
static unsigned pitHz[52]={110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,2093};
static int pitOutput(unsigned hz,unsigned level)
{
    if (!pitVoice) return MMSYSERR_NOTSUPPORTED;
    return pitVoice(1,hz,level) ? MMSYSERR_ALLOCATED : 0;
}

static WORD synthFlags,programs[16],voicePreset[4];
static WORD synthPasses,synthMissed,synthMaxGap,synthMaxWrites;
static DWORD synthClock,synthTick;
static TVOICE voiceState[4];
static void output(unsigned value) { PsgByte(value); ++writes; }
static void mute(unsigned voice)
{
    output(0x9f|(voice<<5));
    if(voice<3) {notesOn[voice]=0; periodOn[voice]=0; volumeOn[voice]=15;}
    else {noiseNote=0;noiseVolume=15;}
}
static void reset(void)
{
    unsigned i;for(i=0;i<4;++i)mute(i);
    if(pitNote)pitOutput(0,0);pitNote=0;
    counter=0; for(i=0;i<3;++i)age[i]=0;
    for(i=0;i<16;++i)programs[i]=0;
    synthClock=0;synthTick=GetTickCount();
}
static int owns(DWORD cookie)
{
    return owner && owner==GetCurrentTask() && cookie==MIDI_COOKIE;
}
static void release(void)
{
    if(!owner)return;
    reset();
    if(hasSound) {StopSound();CloseSound();hasSound=0;}
    owner=0;synthFlags=0;pitEnabled=0;pitVoice=0;
}
static unsigned attenuation(unsigned velocity)
{
    /* 1..127 maps to attenuations 14..0. No sounding velocity maps to mute. */
    return 14-((velocity-1)*14)/126;
}
/* The owning application supplies WM_TIMER service. No driver callbacks,
   interrupt hooks, allocations, catch-up replay or Windows calls in WEP. */
static void renderVoice(unsigned slot)
{
    /* DLL DS differs from the caller's SS: near result pointers must
       point into our DGROUP, never at stack-local WORDs. No yield here. */
    static WORD period,volume;
    if(!InstRead(&voiceState[slot],synthClock,synthFlags,&period,&volume)) {
        mute(slot);return;
    }
    if(slot<3) {
        if(period!=periodOn[slot]) {
            output(0x80|(slot<<5)|(period&15));output(period>>4);
            periodOn[slot]=period;
        }
        if(volume!=volumeOn[slot]) {
            output(0x90|(slot<<5)|volume);volumeOn[slot]=volume;
        }
    } else if(volume!=noiseVolume) {
        output(0xf0|volume);noiseVolume=volume;
    }
}
static void service(void)
{
    DWORD now,elapsed,steps;WORD before;unsigned i;
    if(!(synthFlags&SYNTH_ENVELOPES))return;
    now=GetTickCount();elapsed=now-synthTick;
    if(elapsed<SYNTH_QUANTUM)return;
    steps=elapsed/SYNTH_QUANTUM;
    synthTick+=steps*SYNTH_QUANTUM;synthClock+=steps;
    if(elapsed>65535UL)synthMaxGap=65535;
    else if((WORD)elapsed>synthMaxGap)synthMaxGap=(WORD)elapsed;
    if(steps>1) {
        if(steps-1>65535UL-synthMissed)synthMissed=65535;
        else synthMissed+=(WORD)(steps-1);
    }
    if(synthPasses<65535)++synthPasses;
    before=writes;
    for(i=0;i<3;++i)if(notesOn[i])renderVoice(i);
    if(noiseNote)renderVoice(3);
    if((WORD)(writes-before)>synthMaxWrites)
        synthMaxWrites=(WORD)(writes-before);
}
static void startVoice(unsigned slot,unsigned preset,unsigned volume,
                       unsigned period)
{
    voicePreset[slot]=preset;
    InstStart(&voiceState[slot],preset,volume,period,synthClock);
    renderVoice(slot);
}
static DWORD noteOn(unsigned channel,unsigned note,unsigned velocity)
{
    unsigned i,j,slot=3,oldest=65535,noise,ranks[3];
    if(channel==9) {
        if(note<35 || note>81)return MMSYSERR_INVALPARAM;
        noise=0xe6;
        if(note==35 || note==36)noise=0xe2;
        else if(note==38 || note==40)noise=0xe5;
        else if(note==42 || note==44 || note==46)noise=0xe4;
        noiseNote=note;noiseVolume=attenuation(velocity);
        output(noise);
        if(synthFlags&SYNTH_ENVELOPES) {
            i=(note==35 || note==36)?8:
              (note==42 || note==44 || note==46)?10:9;
            j=noiseVolume;noiseVolume=16;startVoice(3,i,j,1);
        } else output(0xf0|noiseVolume);
        return 0L;
    }
    if(note<45 || note>96)return MMSYSERR_INVALPARAM;
    if(pitEnabled && channel==15) {
        if(pitOutput(pitHz[note-45],1))return MMSYSERR_ALLOCATED;
        pitNote=note;return 0L;
    }
    /* Repeated pitch/channel retriggers the same voice. */
    for(i=0;i<3;++i) if(notesOn[i]==note && channelsOn[i]==channel)slot=i;
    if(slot==3) for(i=0;i<3;++i) if(!notesOn[i]) {slot=i;break;}
    if(slot==3) {
        for(i=0;i<3;++i) if(age[i]<oldest) {slot=i;oldest=age[i];}
        ++steals;
    }
    /* Rebase ages before wrap; at most three tiny fixed comparisons. */
    if(counter==65535) {
        for(i=0;i<3;++i) {
            ranks[i]=1;
            for(j=0;j<3;++j) if(age[j]<age[i])++ranks[i];
        }
        for(i=0;i<3;++i)age[i]=ranks[i];
        counter=3;
    }
    notesOn[slot]=note;channelsOn[slot]=channel;age[slot]=++counter;
    periodOn[slot]=periods[note-45];volumeOn[slot]=attenuation(velocity);
    output(0x80|(slot<<5)|(periodOn[slot]&15));
    output(periodOn[slot]>>4);
    if(synthFlags&SYNTH_ENVELOPES) {
        i=volumeOn[slot];volumeOn[slot]=16;
        startVoice(slot,programs[channel]>>4,i,periodOn[slot]);
    } else output(0x90|(slot<<5)|volumeOn[slot]);
    return 0L;
}
static DWORD noteOff(unsigned channel,unsigned note)
{
    unsigned i;
    if(pitEnabled && channel==15) {
        if(pitNote==note) {pitOutput(0,0);pitNote=0;}
        return 0L;
    }
    if(channel==9) {if(noiseNote==note)mute(3);return 0L;}
    for(i=0;i<3;++i) if(notesOn[i]==note && channelsOn[i]==channel)mute(i);
    return 0L;
}
static DWORD shortData(DWORD value)
{
    unsigned status,channel,a,b,i;
    if(value&0xff000000UL)return MMSYSERR_INVALPARAM;
    status=(unsigned)value&255;channel=status&15;
    a=((unsigned)value>>8)&255;b=(unsigned)(value>>16)&255;
    if(a>127 || b>127)return MMSYSERR_INVALPARAM;
    switch(status&0xf0) {
    case 0x80: return noteOff(channel,a);
    case 0x90: return b?noteOn(channel,a,b):noteOff(channel,a);
    case 0xc0:
        /* Programs are remembered per MIDI channel; sounding notes keep
           the descriptor copied at note-on. Channel 10 stays percussion. */
        programs[channel]=a;return 0L;
    case 0xb0:
        if(a!=120 && a!=123)return MMSYSERR_NOTSUPPORTED;
        if(pitEnabled && channel==15){pitOutput(0,0);pitNote=0;}
        else if(channel==9)mute(3);
        else for(i=0;i<3;++i) if(notesOn[i] && channelsOn[i]==channel)mute(i);
        return 0L;
    default:return MMSYSERR_NOTSUPPORTED;
    }
}
DWORD MAPI midiMessage(WORD id,WORD msg,DWORD user,DWORD p1,DWORD p2)
{
    static TCAPS caps;TOPEN FAR *desc;BYTE FAR *dest;BYTE *source;unsigned i,n;
    int count;
    if(id)return MMSYSERR_BADDEVICEID;
    switch(msg) {
    case MODM_GETNUMDEVS:return 1L;
    case MODM_GETDEVCAPS:
        if(!p1 || p2>65535UL)return MMSYSERR_INVALPARAM;
        caps.wMid=0;caps.wPid=0;caps.vDriverVersion=0x0102;
        for(i=0;i<32;++i)caps.szPname[i]=0;
        lstrcpy(caps.szPname,"Tandy PSG experimental MIDI");
        caps.wTechnology=2;caps.wVoices=3;caps.wNotes=3;
        caps.wChannelMask=0xffff;caps.dwSupport=0L;
        n=(unsigned)p2;if(n>sizeof(caps))n=sizeof(caps);
        source=(BYTE *)&caps;dest=(BYTE FAR *)p1;
        for(i=0;i<n;++i)dest[i]=source[i];return 0L;
    case MODM_OPEN:
        if(owner)return MMSYSERR_ALLOCATED;
        if(!user || !p1 || p2)return MMSYSERR_INVALPARAM;
        desc=(TOPEN FAR *)p1;
        if(desc->dwCallback)return MMSYSERR_NOTSUPPORTED;
        if((GetWinFlags()&WF_PMODE) ||
            *(BYTE FAR *)0xfc000000UL!=0x21)return MMSYSERR_ERROR;
        /* Cooperate with existing SOUND users before touching global PSG. */
        count=OpenSound();if(count<0)return MMSYSERR_ALLOCATED;
        hasSound=1;owner=GetCurrentTask();
        if(!owner) {CloseSound();hasSound=0;return MMSYSERR_ERROR;}
        StopSound();synthFlags=0;
        synthPasses=synthMissed=synthMaxGap=synthMaxWrites=0;
        reset();*(DWORD FAR *)user=MIDI_COOKIE;return 0L;
    case TANDY_PIT_CONFIG:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        if(p1>1 || p2!=1)return MMSYSERR_INVALPARAM;
        if(!pitVoice)pitVoice=(PITPROC)GetProcAddress(
            GetModuleHandle("SOUND"),"TANDYPITVOICE");
        if(!pitVoice)return MMSYSERR_NOTSUPPORTED;
        if(pitOutput(0,0))return MMSYSERR_ALLOCATED;
        reset();pitEnabled=(int)p1;return 0L;
    case MODM_CLOSE:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        release();return 0L;
    case MODM_RESET:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        reset();return 0L;
    case MODM_DATA:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        service();return shortData(p1);
    case MODM_SYNTHCONFIG:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        if(p2 || (p1&~3UL) || p1==SYNTH_VIBRATO)
            return MMSYSERR_INVALPARAM;
        /* Configuration changes panic first, so no stale voice can resume. */
        reset();synthFlags=(WORD)p1;return 0L;
    case MODM_SYNTHUPDATE:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        if(p1 || p2)return MMSYSERR_INVALPARAM;
        service();return 0L;
    case MODM_LONGDATA:return MMSYSERR_NOTSUPPORTED;
    default:return MMSYSERR_NOTSUPPORTED;
    }
}
DWORD MAPI modMessage(WORD id,WORD msg,DWORD user,DWORD p1,DWORD p2)
{
    return midiMessage(id,msg,user,p1,p2);
}
LONG MAPI DriverProc(DWORD id,WORD handle,WORD msg,LONG p1,LONG p2)
{
    /* Lifecycle probe only. No install/configure or multimedia dispatcher. */
    if(msg==1 || msg==2 || msg==3 || msg==4)return 1L;
    if(msg==5 || msg==6) {if(!owner || owner==GetCurrentTask())release();return 1L;}
    return 0L;
}
/* WEP may not call other DLLs. Normal clients close before unloading. */
int MAPI WEP(int reason)
{
    unsigned i;
    if(pitNote)PitSilence();
    pitNote=0;pitVoice=0;pitEnabled=0;
    if(owner)for(i=0;i<4;++i)mute(i);
    owner=0;hasSound=0;synthFlags=0;return 1;
}
WORD MAPI DebugState(WORD query)
{
    WORD mask=0;unsigned i;
    for(i=0;i<3;++i)if(notesOn[i])mask|=1<<i;
    if(noiseNote)mask|=8;
    if(pitNote)mask|=16;
    if(query==0)return mask;
    if(query==1)return owner?1:0;
    if(query==2)return writes;
    if(query==3)return steals;
    if(query>=4 && query<=6)return notesOn[query-4];
    if(query==7)return noiseNote;
    if(query>=8 && query<=10)return periodOn[query-8];
    if(query==11)return noiseVolume;
    if(query>=12 && query<=14)return volumeOn[query-12];
    if(query==15)return synthFlags;
    if(query==16)return synthPasses;
    if(query==17)return synthMissed;
    if(query==18)return synthMaxGap;
    if(query==19)return synthMaxWrites;
    if(query>=32 && query<48)return programs[query-32];
    if(query>=48 && query<52)return voicePreset[query-48];
    return 0xffff;
}

