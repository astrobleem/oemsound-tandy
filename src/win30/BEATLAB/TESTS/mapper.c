/* Actual shipped mapper with hardware/SOUND mocks, never a ROM/emulator run. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "BEATPAT.H"
#ifndef BEATS_MAPPER_SOURCE
#error HOSTTEST.PY must select the unchanged shipped mapper source
#endif
#include BEATS_MAPPER_SOURCE
static DWORD now;
static unsigned latch,period[3],level[4]={15,15,15,15},noiseSetting;
static unsigned speakerHz,speakerLevel,closed,checks;
static const unsigned channel[5]={0,1,2,9,15};
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL mapper:%u: %s\n",(unsigned)__LINE__,#x); return 1; \
} } while(0)
WORD GetWinFlags(void){return 0;}
HTASK GetCurrentTask(void){return 1;}
int OpenSound(void){return 3;}
int StopSound(void){return 0;}
void CloseSound(void){++closed;}
DWORD GetTickCount(void){return now;}
HMODULE GetModuleHandle(char *name){return !strcmp(name,"SOUND")?1U:0U;}
static int mock_pit(unsigned voice,unsigned hz,unsigned value)
{
    if(voice!=1)return 1;
    speakerHz=value?hz:0;speakerLevel=value;return 0;
}
PITMOCKPROC GetProcAddress(HMODULE module,char *name)
{
    return module==1&&!strcmp(name,"TANDYPITVOICE")?mock_pit:0;
}
void PitSilence(void){speakerHz=speakerLevel=0;}
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
int main(void)
{
    BPAT p;unsigned d,t,r,s,slot,duration,gate,total,held[5],active[5],step;
    char filename[40];FILE *trace,*attacks,*metadata;const char *name;
    attacks=fopen("attacks.csv","w");CK(attacks!=0);
    fprintf(attacks,"demo,lane,step,note,start_ms,end_ms,slot,expected_hz,program\n");
    metadata=fopen("demos.csv","w");CK(metadata!=0);
    fprintf(metadata,"demo,name,tempo,pit,duration_ms\n");
    for(d=0;d<BP_DEMOS;++d){
        name=BPDemo(&p,d);CK(BPValid(&p));
        duration=30000U/p.tempo;gate=(duration*3U)/4U;total=duration*16U+200U;
        fprintf(metadata,"%u,%s,%u,%u,%u\n",d,name,p.tempo,(unsigned)p.pit,total);
        sprintf(filename,"demo-%u.csv",d);trace=fopen(filename,"w");CK(trace!=0);
        fprintf(trace,"ms,p0,p1,p2,v0,v1,v2,v3,noise,pit_hz\n");
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
                    else {slot=4;CK(pitNote==held[r]);CK(speakerHz==pitHz[held[r]-45]);hz=speakerHz;}
                    fprintf(attacks,"%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                        d,r,t/duration,held[r],t,t+gate,slot,hz,prog);
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
            fprintf(trace,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",t,
                period[0],period[1],period[2],level[0],level[1],level[2],level[3],noiseSetting,speakerHz);
        }
        CK(release_all(held)==0);CK(midiMessage(0,MODM_RESET,MIDI_COOKIE,0,0)==0);
        CK(DebugState(0)==0&&speakerHz==0);
        for(r=0;r<4;++r)CK(level[r]==15);
        CK(midiMessage(0,MODM_CLOSE,MIDI_COOKIE,0,0)==0);
        CK(!owner&&!hasSound&&closed==d+1);CK(!fclose(trace));
    }
    CK(!fclose(attacks));CK(!fclose(metadata));
    printf("PASS actual shipped mapper routing/reset/close checks=%u\n",checks);
    return 0;
}
