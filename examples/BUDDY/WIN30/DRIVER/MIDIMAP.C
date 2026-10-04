/* Experimental completion of the repository's midiMessage handler.
   Direct task-context Win3.0 harness; not a registered MMSYSTEM device. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include "PERIODS.H"
#define MAPI FAR PASCAL _loadds
extern void _cdecl PsgByte(unsigned);
static HTASK owner;
static WORD notesOn[3],channelsOn[3],periodOn[3],volumeOn[3],age[3];
static WORD noiseNote,noiseVolume,counter,steals,writes;
static int hasSound;
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
    counter=0; for(i=0;i<3;++i)age[i]=0;
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
    owner=0;
}
static unsigned attenuation(unsigned velocity)
{
    /* 1..127 maps to attenuations 14..0. No sounding velocity maps to mute. */
    return 14-((velocity-1)*14)/126;
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
        output(noise);output(0xf0|noiseVolume);return 0L;
    }
    if(note<45 || note>96)return MMSYSERR_INVALPARAM;
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
    output(periodOn[slot]>>4); output(0x90|(slot<<5)|volumeOn[slot]);
    return 0L;
}
static DWORD noteOff(unsigned channel,unsigned note)
{
    unsigned i;
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
    case 0xb0:
        if(a!=120 && a!=123)return MMSYSERR_NOTSUPPORTED;
        if(channel==9)mute(3);
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
        caps.wMid=0;caps.wPid=0;caps.vDriverVersion=0x0101;
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
        StopSound();reset();*(DWORD FAR *)user=MIDI_COOKIE;return 0L;
    case MODM_CLOSE:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        release();return 0L;
    case MODM_RESET:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        reset();return 0L;
    case MODM_DATA:
        if(!owns(user))return MMSYSERR_ALLOCATED;
        return shortData(p1);
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
    if(owner)reset();
    owner=0;hasSound=0;return 1;
}
WORD MAPI DebugState(WORD query)
{
    WORD mask=0;unsigned i;
    for(i=0;i<3;++i)if(notesOn[i])mask|=1<<i;
    if(noiseNote)mask|=8;
    if(query==0)return mask;
    if(query==1)return owner?1:0;
    if(query==2)return writes;
    if(query==3)return steals;
    if(query>=4 && query<=6)return notesOn[query-4];
    if(query==7)return noiseNote;
    if(query>=8 && query<=10)return periodOn[query-8];
    if(query==11)return noiseVolume;
    if(query>=12 && query<=14)return volumeOn[query-12];
    return 0xffff;
}
