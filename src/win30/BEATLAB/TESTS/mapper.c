/* Current or archived mapper with hardware/SOUND mocks; no ROM/emulator run. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "BEATPAT.H"
#ifndef BEATS_MAPPER_SOURCE
#error HOSTTEST.PY must select the current or archived mapper source
#endif
#include BEATS_MAPPER_SOURCE
#ifndef BEATS_PIT_SOURCE
#error HOSTTEST.PY must select the unchanged shared PIT service core
#endif
#include BEATS_PIT_SOURCE
static DWORD now;
static unsigned latch,period[3],level[4]={15,15,15,15},noiseSetting;
static unsigned speakerHz,speakerLevel,closed,checks;
static unsigned pitBits=160,pitDivisor,serviceAvailable=1,task=1;
static const unsigned channel[5]={0,1,2,9,15};
static const unsigned expectedHz[71]={
    37,39,41,44,46,49,52,55,58,62,65,69,73,78,82,87,92,98,104,
    110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,
    262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,
    622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,
    1319,1397,1480,1568,1661,1760,1865,1976,2093
};
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL mapper:%u: %s\n",(unsigned)__LINE__,#x); return 1; \
} } while(0)
WORD GetWinFlags(void){return 0;}
HTASK GetCurrentTask(void){return (HTASK)task;}
int OpenSound(void){return 3;}
int StopSound(void){return 0;}
void CloseSound(void){++closed;}
DWORD GetTickCount(void){return now;}
HMODULE GetModuleHandle(char *name){return !strcmp(name,"SOUND")?1U:0U;}
static int mock_pit(unsigned voice,unsigned hz,unsigned value)
{
    if(pitSet(voice,hz,value))return 1;
    speakerHz=value?hz:0;speakerLevel=value;return 0;
}
unsigned PitRead(void){return pitBits;}
void PitGate(unsigned bits){pitBits=bits;}
void PitProgram(unsigned divisor){pitDivisor=divisor;}
PITMOCKPROC GetProcAddress(HMODULE module,char *name)
{
    return serviceAvailable&&module==1&&!strcmp(name,"TANDYPITVOICE")?mock_pit:0;
}
void PitSilence(void){speakerHz=speakerLevel=0;pitBits&=252U;pitActive=0;}
void PsgByte(unsigned value)
{
    unsigned voice;
    if(value>255){fprintf(stderr,"PSG byte overflow\n");exit(2);}
    if(value&128){
        latch=(value>>4)&7;voice=latch>>1;
        if(latch&1)level[voice]=value&15;
        else if(voice==3)noiseSetting=value&7;
        else period[voice]=(period[voice]&1008U)|(value&15U);
    }else if(!(latch&1)&&(latch>>1)<3)
        period[latch>>1]=(period[latch>>1]&15U)|((value&63U)<<4);
}
static DWORD send(unsigned status,unsigned a,unsigned b)
{
    return midiMessage(0,MODM_DATA,MIDI_COOKIE,
        (DWORD)status|((DWORD)a<<8)|((DWORD)b<<16),0);
}
static int release_all(unsigned held[5])
{
    unsigned r;
    for(r=0;r<5;++r)if(held[r]){
        CK(send(0x80|channel[r],held[r],0)==0);held[r]=0;
    }
    return 0;
}
typedef struct {
    WORD notes[3],channels[3],periods[3],volumes[3],program[16];
    TVOICE voices[4];WORD noise,nvolume,pit,flags,writeCount,sequence;
    unsigned registers[3],levels[4],nsetting,spkhz,spklevel,bits,divisor,active;
} SNAP;
static void snapshot(SNAP *s)
{
    memset(s,0,sizeof(*s));memcpy(s->notes,notesOn,sizeof(notesOn));
    memcpy(s->channels,channelsOn,sizeof(channelsOn));memcpy(s->periods,periodOn,sizeof(periodOn));
    memcpy(s->volumes,volumeOn,sizeof(volumeOn));memcpy(s->program,programs,sizeof(programs));
    memcpy(s->voices,voiceState,sizeof(voiceState));s->noise=noiseNote;s->nvolume=noiseVolume;
    s->pit=pitNote;s->flags=synthFlags;s->writeCount=writes;s->sequence=counter;
    memcpy(s->registers,period,sizeof(period));memcpy(s->levels,level,sizeof(level));
    s->nsetting=noiseSetting;s->spkhz=speakerHz;s->spklevel=speakerLevel;
    s->bits=pitBits;s->divisor=pitDivisor;s->active=pitActive;
}
static int setup(int enabled)
{
    task=1;if(owner)CK(midiMessage(0,MODM_CLOSE,MIDI_COOKIE,0,0)==0);
    owner=1;hasSound=1;now=0;serviceAvailable=1;pitBits=160;pitActive=0;pitDivisor=0;
    speakerHz=speakerLevel=0;
    CK(midiMessage(0,MODM_SYNTHCONFIG,MIDI_COOKIE,3,0)==0);
    if(enabled)CK(midiMessage(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1,1)==0);
    return 0;
}
static int rejected(unsigned ch,unsigned note)
{
    SNAP before,after;snapshot(&before);
    CK(send(0x90|ch,note,96)==MMSYSERR_INVALPARAM);snapshot(&after);
    CK(!memcmp(&before,&after,sizeof(before)));return 0;
}
static int boundaries(void)
{
    SNAP before,after;unsigned ch,n,low;
#ifdef BEATS_EXPECT_LEGACY
    low=45;
#else
    low=33;
#endif
    CK(setup(0)==0);CK(send(0x90,60,96)==0);
    CK(rejected(15,26)==0);CK(rejected(0,44)==0);
    CK(send(0x9f,45,96)==0&&speakerHz==0);CK(send(0x8f,45,0)==0);
    CK(setup(1)==0);CK(send(0x90,60,96)==0);
    for(ch=0;ch<15;++ch)CK(rejected(ch,26)==0);
    CK(rejected(15,25)==0);CK(rejected(15,97)==0);
    CK(rejected(9,34)==0);CK(rejected(9,82)==0);
#ifdef BEATS_EXPECT_LEGACY
    CK(rejected(15,26)==0);CK(rejected(15,33)==0);
#else
    for(n=26;n<45;++n){
        CK(send(0x9f,n,96)==0);CK(pitNote==n&&speakerHz==expectedHz[n-26]);
        CK(pitDivisor==(1193182UL+speakerHz/2U)/speakerHz);
        CK(notesOn[0]==60&&channelsOn[0]==0);
        CK(send(0x8f,n,0)==0&&speakerHz==0&&(pitBits&3)==0);
    }
#endif
    CK(send(0x9f,low,96)==0);CK(send(0x9f,low+3,96)==0);
    snapshot(&before);CK(send(0x8f,low,0)==0);snapshot(&after);
    CK(!memcmp(&before,&after,sizeof(before)));CK(pitNote==low+3);
    CK(send(0x8f,low+3,0)==0&&speakerHz==0);
    CK(setup(0)==0);CK(send(0x90,60,96)==0);serviceAvailable=0;pitVoice=0;
    snapshot(&before);CK(midiMessage(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1,1)==MMSYSERR_NOTSUPPORTED);
    snapshot(&after);CK(!memcmp(&before,&after,sizeof(before)));serviceAvailable=1;
    CK(setup(1)==0);pitBits=163;pitActive=0;snapshot(&before);
    CK(send(0x9f,low,96)==MMSYSERR_ALLOCATED);snapshot(&after);
    CK(!memcmp(&before,&after,sizeof(before)));
    CK(setup(1)==0);CK(send(0x90,60,96)==0);CK(send(0x99,42,76)==0);CK(send(0x9f,low,96)==0);
    task=2;snapshot(&before);CK(send(0x9f,low+3,96)==MMSYSERR_ALLOCATED);
    CK(midiMessage(0,MODM_RESET,MIDI_COOKIE,0,0)==MMSYSERR_ALLOCATED);
    snapshot(&after);CK(!memcmp(&before,&after,sizeof(before)));task=1;
    CK(midiMessage(0,MODM_RESET,MIDI_COOKIE,0,0)==0&&DebugState(0)==0);
    CK(!speakerHz&&pitBits==160);for(n=0;n<4;++n)CK(level[n]==15);
    CK(midiMessage(0,MODM_CLOSE,MIDI_COOKIE,0,0)==0&&!owner&&!hasSound);
    return 0;
}
static int compatibility(void)
{
    FILE *f;unsigned n,prog,t,ch;f=fopen("old-range.csv","w");CK(f!=0);
    CK(setup(1)==0);
    for(n=45;n<=96;++n){
        CK(send(0x9f,n,96)==0&&speakerHz==expectedHz[n-26]);
        CK(pitDivisor==(1193182UL+speakerHz/2U)/speakerHz);
        fprintf(f,"P,%u,%u,%u\n",n,speakerHz,pitDivisor);
        CK(send(0x8f,n,0)==0&&speakerHz==0);
    }
    for(ch=0;ch<3;++ch)for(prog=0;prog<128;prog+=16)for(n=45;n<=96;++n){
        now=0;CK(midiMessage(0,MODM_SYNTHCONFIG,MIDI_COOKIE,3,0)==0);
        CK(send(0xc0|ch,prog,0)==0&&send(0x90|ch,n,96)==0);
        for(t=0;t<=495;t+=55){now=t;CK(midiMessage(0,MODM_SYNTHUPDATE,MIDI_COOKIE,0,0)==0);
            fprintf(f,"T,%u,%u,%u,%u,%u,%u\n",ch,prog,n,t,period[0],level[0]);}
        CK(send(0x80|ch,n,0)==0);
    }
    for(n=35;n<=81;++n){
        now=0;CK(midiMessage(0,MODM_SYNTHCONFIG,MIDI_COOKIE,3,0)==0);CK(send(0x99,n,76)==0);
        for(t=0;t<=330;t+=55){now=t;CK(midiMessage(0,MODM_SYNTHUPDATE,MIDI_COOKIE,0,0)==0);
            fprintf(f,"N,%u,%u,%u,%u\n",n,t,noiseSetting,level[3]);}
        CK(send(0x89,n,0)==0);
    }
    CK(midiMessage(0,MODM_CLOSE,MIDI_COOKIE,0,0)==0);CK(!fclose(f));return 0;
}
static int demos_run(void)
{
    BPAT p;unsigned d,t,r,s,slot,duration,gate,total,held[5],active[5],step;
    char filename[40];FILE *trace,*attacks,*metadata;const char *name;
    attacks=fopen("attacks.csv","w");CK(attacks!=0);
    fprintf(attacks,"demo,lane,step,note,start_ms,end_ms,slot,expected_hz,program,pit_divisor\n");
    metadata=fopen("demos.csv","w");CK(metadata!=0);
    fprintf(metadata,"demo,name,tempo,pit,duration_ms\n");
    for(d=0;d<BP_DEMOS;++d){
        name=BPDemo(&p,d);CK(BPValid(&p));
        duration=30000U/p.tempo;gate=(duration*3U)/4U;total=duration*16U+200U;
        fprintf(metadata,"%u,%s,%u,%u,%u\n",d,name,p.tempo,(unsigned)p.pit,total);
        sprintf(filename,"demo-%u.csv",d);trace=fopen(filename,"w");CK(trace!=0);
        fprintf(trace,"ms,p0,p1,p2,v0,v1,v2,v3,noise,pit_hz,pit_divisor\n");
        memset(held,0,sizeof(held));owner=1;hasSound=1;now=0;
        CK(midiMessage(0,MODM_SYNTHCONFIG,MIDI_COOKIE,3,0)==0);
        if(p.pit)CK(midiMessage(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1,1)==0);
        for(r=0;r<3;++r)CK(send(0xc0|channel[r],p.program[r],0)==0);
        for(t=0;t<total;++t){
            now=t;
            if(t<duration*16U&&t%duration==0){
                CK(release_all(held)==0);step=(t/duration)%8U;
                memset(active,0,sizeof(active));
                for(r=0;r<5;++r)if(p.note[r][step]&&(r!=4||p.pit)){
                    unsigned hz,prog=0;
                    CK(send(0x90|channel[r],p.note[r][step],r==3?76:96)==0);
                    held[r]=p.note[r][step];active[r]=1;slot=5;
                    if(r<3){
                        for(s=0;s<3;++s)if(notesOn[s]==held[r]&&channelsOn[s]==channel[r])slot=s;
                        CK(slot<3);CK(periodOn[slot]==periods[held[r]-45]);
                        CK(voicePreset[slot]==(p.program[r]>>4));
                        hz=(unsigned)(3579545UL/(32UL*periodOn[slot]));prog=p.program[r];
                    }else if(r==3){slot=3;CK(noiseNote==held[r]);hz=0;}
                    else {slot=4;CK(pitNote==held[r]);CK(speakerHz==expectedHz[held[r]-26]);hz=speakerHz;
                        CK(pitDivisor==(1193182UL+hz/2U)/hz);}
                    fprintf(attacks,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                        d,r,t/duration,held[r],t,t+gate,slot,hz,prog,r==4?pitDivisor:0);
                }
                if(d==3){
                    if(step&1)CK(DebugState(0)==16);
                    if(step==0||step==4)CK(DebugState(0)==31);
                }
                for(r=0;r<5;++r)if(active[r]){
                    if(r==3)CK(noiseNote==held[r]);
                    else if(r==4)CK(pitNote==held[r]);
                    else {slot=5;for(s=0;s<3;++s)
                        if(notesOn[s]==held[r]&&channelsOn[s]==channel[r])slot=s;CK(slot<3);}
                }
            }
            if(t%duration==gate){CK(release_all(held)==0);CK(DebugState(0)==0);}
            CK(midiMessage(0,MODM_SYNTHUPDATE,MIDI_COOKIE,0,0)==0);
            fprintf(trace,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",t,
                period[0],period[1],period[2],level[0],level[1],level[2],level[3],noiseSetting,speakerHz,speakerHz?pitDivisor:0);
        }
        CK(release_all(held)==0);CK(midiMessage(0,MODM_RESET,MIDI_COOKIE,0,0)==0);
        CK(DebugState(0)==0&&speakerHz==0);
        for(r=0;r<4;++r)CK(level[r]==15);
        CK(midiMessage(0,MODM_CLOSE,MIDI_COOKIE,0,0)==0);
        CK(!owner&&!hasSound&&closed==d+1);CK(!fclose(trace));
    }
    CK(!fclose(attacks));CK(!fclose(metadata));
    printf("PASS current app-local mapper routing/reset/close checks=%u\n",checks);
    return 0;
}
int main(void)
{
    CK(boundaries()==0);CK(compatibility()==0);closed=0;
#ifdef BEATS_EXPECT_LEGACY
    printf("PASS archived mapper rejection/old-range checks=%u\n",checks);return 0;
#else
    return demos_run();
#endif
}
