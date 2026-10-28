/* SPDX-License-Identifier: MIT. Render ahead, using the existing presets.
   RAWCLK is the only playback owner; no MIDI mapper or FXBytes mute prefix.
   Preparation/merging/60 Hz trajectories run here, never in an IRQ callback. */
#define WINVER 0x0300
#include <windows.h>
#include <string.h>
#include "MIDIAPI.H"
#include "INSTR.H"
#include "PERIODS.H"
#include "BEATPLAY.H"
static const unsigned pitHz[71]={37,39,41,44,46,49,52,55,58,62,65,69,73,78,82,87,92,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,2093};
unsigned long BBStamp(unsigned long tick)
{
    return (tick/180UL)*3579545UL+(tick%180UL)*3579545UL/180UL;
}
unsigned BBStep(unsigned long tick,unsigned tempo)
{
    return (unsigned)((tick*tempo/1800UL)%8UL);
}
/* C4 (60) plays the imported pitch. Transpose the entire compiled sweep,
   vibrato and pitched-noise divider together. Fixed-rate noise ignores it. */
unsigned BBTranspose(unsigned value,unsigned note,unsigned low,unsigned high)
{
    static const unsigned ratio[12]={4096,3866,3649,3444,3251,3069,2896,2734,2580,2435,2299,2170};
    unsigned long v;int octave;
    if(!value)return 0;
    v=((unsigned long)value*ratio[note%12]+2048UL)/4096UL;
    octave=(int)(note/12)-5;
    if(octave>0)v=(v+(1UL<<(octave-1)))>>octave;
    else if(octave<0)v<<=-octave;
    if(v<low)v=low;if(v>high)v=high;return (unsigned)v;
}
int BBPrepare(BBPLAYER *p,const BPAT *song)
{
    unsigned r;
    if(!p||!BPValid(song))return 0;
    memset(p,0,sizeof(*p));p->song=*song;p->lastStep=8;
    for(r=0;r<5;++r){
        if(song->loaded[r]&&!FXCompile(&song->patch[r],&p->clip[r]))return 0;
        p->cursor[r]=p->clip[r].count;
    }
    return 1;
}
int BBNext(BBPLAYER *p,BBBURST *b)
{
    FXMSTATE base,out;FXMLAYER fx[5];TVOICE voice;
    unsigned r,n,s,phase,elapsed,preset,attack=0;WORD period,volume;
    const FXROW *row;
    if(!p||!b||p->tick>BB_LIMIT)return 0;
    memset(&base,0,sizeof(base));memset(fx,0,sizeof(fx));
    for(r=0;r<3;++r)base.period[r]=1;
    for(r=0;r<4;++r)base.level[r]=15;
    s=BBStep(p->tick,p->song.tempo);
    phase=(unsigned)((p->tick*p->song.tempo)%1800UL);
    elapsed=(unsigned)((unsigned long)phase*1000UL/((unsigned long)p->song.tempo*3300UL));
    for(r=0;r<5;++r){
        n=p->song.note[r][s];
        if(r==4&&!p->song.pit)continue;
        if(p->lastStep!=s&&p->song.trigger[r][s]){
            p->cursor[r]=0;p->pitch[r]=n;if(r==3)attack=1;
        }
        if(n&&!p->song.trigger[r][s]&&phase<1350){
            if(r==4)base.pit=(unsigned)((3579545UL+pitHz[n-26]*3UL/2UL)/(pitHz[n-26]*3UL));
            else {
                preset=r<3?p->song.program[r]>>4:(n==35||n==36)?8:(n==42||n==44||n==46)?10:9;
                InstStart(&voice,preset,r==3?6:4,r<3?periods[n-45]:1,0);
                InstRead(&voice,elapsed,3,&period,&volume);
                if(r<3)base.period[r]=period;
                base.level[r]=volume;
                if(r==3){base.noise=(n==35||n==36)?2:(n==38||n==40)?5:(n==42||n==44||n==46)?4:6;
                    if(p->lastStep!=s)attack=1;}
            }
        }
        if(p->cursor[r]<p->clip[r].count){
            row=&p->clip[r].row[p->cursor[r]++];fx[r].active=1;
            fx[r].period=BBTranspose(row->period,p->pitch[r],1,1023);
            fx[r].level=row->level;fx[r].noise=row->noise;
            fx[r].pit=BBTranspose(row->pit,p->pitch[r],151,65535U);
            fx[r].borrow2=p->clip[r].mask==12;
            /* Base drum attacks hidden by a patch must not restart its LFSR. */
            if(r==3)attack=p->cursor[r]==1;
        }
    }
    if(p->noiseFX&&!fx[3].active&&base.level[3]<15)attack=1;
    FXMMerge(&out,&base,fx);
    if(p->tick==BB_LIMIT){for(r=0;r<4;++r)out.level[r]=15;out.pit=0;}
    b->at=BBStamp(p->tick);b->pit=out.pit;
    b->count=FXMBytes(&out,&p->old,p->tick==0,attack,b->data);
    p->old=out;p->lastStep=s;p->noiseFX=fx[3].active;++p->tick;
    return 1;
}
