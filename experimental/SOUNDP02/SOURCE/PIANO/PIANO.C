/* Mini Piano ownership core. GPL-3.0-or-later. */
#include "PIANO.H"
static void event(PI_OUT *o,unsigned st,unsigned n,unsigned v){
    if(o->count<PI_MAX_EVENTS){o->event[o->count].status=(unsigned char)st;o->event[o->count].note=(unsigned char)n;o->event[o->count].velocity=(unsigned char)v;o->count++;}
}
static void release(PI_STATE *s,unsigned src,PI_OUT *o){unsigned n;
    n=s->source[src];if(!n)return;s->source[src]=0;s->held--;
    if(s->refs[n]){s->refs[n]--;if(!s->refs[n])event(o,128,n,0);}
}
void pi_init(PI_STATE *s){unsigned i;s->octave=4;s->velocity=100;s->held=0;for(i=0;i<PI_SOURCES;i++)s->source[i]=0;for(i=0;i<128;i++)s->refs[i]=0;}
void pi_press(PI_STATE *s,unsigned src,unsigned off,PI_OUT *o){unsigned n;o->count=0;if(src>=PI_SOURCES||off>=PI_KEYS)return;n=(s->octave+1u)*12u+off;if(n<45||n>96)return;if((unsigned)s->source[src]==n)return;if(s->source[src])release(s,src,o);s->source[src]=(unsigned char)n;s->held++;if(!s->refs[n])event(o,144,n,s->velocity);s->refs[n]++;}
void pi_release(PI_STATE *s,unsigned src,PI_OUT *o){o->count=0;if(src<PI_SOURCES)release(s,src,o);}
void pi_panic(PI_STATE *s,PI_OUT *o){unsigned i;o->count=0;for(i=0;i<PI_SOURCES;i++)release(s,i,o);}
void pi_octave(PI_STATE *s,int d,PI_OUT *o){int n=(int)s->octave+d;pi_panic(s,o);if(n<PI_MIN_OCTAVE)n=PI_MIN_OCTAVE;if(n>PI_MAX_OCTAVE)n=PI_MAX_OCTAVE;s->octave=(unsigned char)n;}
int pi_scan(unsigned scan){static unsigned char codes[PI_KEYS]={0x1e,0x11,0x1f,0x12,0x20,0x21,0x14,0x22,0x15,0x23,0x16,0x24,0x25};unsigned i;for(i=0;i<PI_KEYS;i++)if((unsigned)codes[i]==scan)return (int)i;return -1;}
