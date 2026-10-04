/* Standalone allocation-free core sanitizer stress. */
#include <assert.h>
#include "PIANO.H"
static unsigned long rng=8088UL;
static unsigned rnd(void){rng=(rng*1664525UL+1013904223UL)&0xffffffffUL;return (unsigned)(rng>>16);}
int main(void){PI_STATE s;PI_OUT o;unsigned i,j,k,op,a,b,held,count;pi_init(&s);
    for(i=0;i<200000;i++){op=rnd()%20;a=rnd()%17;b=rnd()%16;
        if(op<12)pi_press(&s,a,b,&o);else if(op<18)pi_release(&s,a,&o);else if(op==18)pi_panic(&s,&o);else pi_octave(&s,(int)(rnd()%13)-6,&o);
        assert(o.count<=PI_MAX_EVENTS);assert(s.octave>=3&&s.octave<=6);held=0;
        for(j=0;j<PI_SOURCES;j++){if(s.source[j]){held++;assert(s.source[j]>=48&&s.source[j]<=96);}}
        assert(held==s.held);
        for(j=1;j<128;j++){count=0;for(k=0;k<PI_SOURCES;k++)if(s.source[k]==j)count++;assert(count==s.refs[j]);}
        for(j=0;j<o.count;j++){assert(o.event[j].status==128||o.event[j].status==144);assert(o.event[j].note>=48&&o.event[j].note<=96);}
    }
    pi_panic(&s,&o);assert(s.held==0);return 0;
}
