/* SPDX-License-Identifier: MIT */
#include "FXMERGE.H"
void FXMMerge(FXMSTATE *out,const FXMSTATE *base,const FXMLAYER *fx)
{
    unsigned i;*out=*base;
    for(i=0;i<3;++i)if(fx[i].active){
        out->period[i]=fx[i].period;out->level[i]=fx[i].level;
    }
    if(fx[3].active){
        out->noise=fx[3].noise;out->level[3]=fx[3].level;
        if(fx[3].borrow2){out->period[2]=fx[3].period;out->level[2]=15;}
    }
    if(fx[4].active)out->pit=fx[4].pit;
}
unsigned FXMBytes(const FXMSTATE *s,const FXMSTATE *old,unsigned initial,
                  unsigned noiseAttack,unsigned char *b)
{
    unsigned i,n=0,p;
    for(i=0;i<3;++i){p=s->period[i];
        if(initial||p!=old->period[i]){
            b[n++]=(unsigned char)(0x80|(i<<5)|(p&15));
            b[n++]=(unsigned char)(p>>4);
        }
        if(initial||s->level[i]!=old->level[i])
            b[n++]=(unsigned char)(0x90|(i<<5)|s->level[i]);
    }
    if(initial||noiseAttack||s->noise!=old->noise)
        b[n++]=(unsigned char)(0xe0|s->noise);
    if(initial||s->level[3]!=old->level[3])
        b[n++]=(unsigned char)(0xf0|s->level[3]);
    return n;
}
