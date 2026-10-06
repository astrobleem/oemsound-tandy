/* Original 55 ms attenuation curves; no game data or interrupt code.
   Indexed snapshots make each update O(1), even after a long GUI stall. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include "INSTR.H"
static BYTE curves[11][16]={
 {0,0,1,2,3,4,5,6,7,8,8,8,8,8,8,8}, /* keys */
 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, /* organ */
 {0,0,1,1,2,2,3,3,4,4,5,5,5,5,5,5}, /* bass */
 {12,9,6,4,2,1,0,0,1,1,1,1,1,1,1,1}, /* pad */
 {4,2,1,0,0,1,1,1,2,2,2,2,2,2,2,2}, /* reed */
 {1,0,0,1,1,1,1,1,2,2,2,2,2,2,2,2}, /* lead */
 {0,2,4,5,6,7,8,9,10,11,12,13,14,15,15,15}, /* bell */
 {0,2,4,6,8,10,12,14,15,15,15,15,15,15,15,15}, /* hit */
 {1,2,4,7,10,13,15,15,15,15,15,15,15,15,15,15}, /* kick */
 {0,2,5,8,11,15,15,15,15,15,15,15,15,15,15,15}, /* snare */
 {2,6,10,15,15,15,15,15,15,15,15,15,15,15,15,15} /* hat */
};
static signed char motion[8]={0,1,2,1,0,-1,-2,-1};
void InstStart(TVOICE *v,unsigned preset,unsigned volume,
               unsigned period,DWORD step)
{
    v->began=step;v->period=period;
    v->velocity=(BYTE)volume;v->preset=(BYTE)preset;
}
int InstRead(TVOICE *v,DWORD step,WORD flags,WORD *period,WORD *volume)
{
    DWORD elapsed;unsigned n,a,depth;int delta,p;
    elapsed=step-v->began;n=elapsed>15UL?15:(unsigned)elapsed;
    a=curves[v->preset][n];
    if(a==15){*volume=15;*period=v->period;return 0;}
    a+=v->velocity;*volume=a>15?15:a;p=v->period;
    if((flags&SYNTH_VIBRATO) && elapsed>=8UL &&
       (v->preset==3 || v->preset==4 || v->preset==5)) {
        /* A shallow divider-proportional triangle, 8 ticks per cycle.
           Below divider 256 motion is zero, avoiding coarse high-note
           warble. Integer quantization makes exact cents note-dependent. */
        depth=v->period>>8;
        delta=motion[(unsigned)elapsed&7];
        if(delta<0)p-=(-delta==2)?depth<<1:depth;
        else p+=(delta==2)?depth<<1:delta?depth:0;
    }
    if(p<1)p=1;if(p>1023)p=1023;*period=(WORD)p;return 1;
}
