/* Compile the unchanged production PIT core; hardware port functions are mocks. */
#include <stdio.h>
#define _cdecl
#ifndef BEATS_PIT_SOURCE
#error HOSTTEST.PY must select the shared PIT core source
#endif
#include BEATS_PIT_SOURCE
static unsigned bits=160,divisor,programmed,checks;
unsigned PitRead(void){return bits;}
void PitGate(unsigned value){bits=value;}
void PitProgram(unsigned value){divisor=value;++programmed;}
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL PIT service:%u: %s\n",(unsigned)__LINE__,#x);return 1; \
} } while(0)
int main(void)
{
    unsigned hz,old,oldbits;
    CK(pitSet(1,36,1)==-13&&!pitActive&&bits==160&&!programmed);
    CK(pitSet(1,7903,1)==-13&&!pitActive&&bits==160&&!programmed);
    CK(pitSet(2,37,1)==-16);CK(pitSet(1,37,2)==-16);
    for(hz=37;hz<=7902;++hz){
        CK(pitSet(1,hz,1)==0);
        CK(divisor==(1193182UL+hz/2U)/hz&&divisor>=2&&divisor<=65535U);
        CK(bits==163&&pitActive);
    }
    CK(pitSet(1,0,0)==0&&bits==160&&!pitActive);
    old=programmed;bits=163;
    CK(pitSet(1,37,1)==-3&&bits==163&&!pitActive&&programmed==old);
    CK(pitSet(1,0,0)==0&&bits==163);
    bits=160;CK(pitSet(1,55,1)==0&&pitActive);bits=161;
    old=programmed;oldbits=bits;
    CK(pitSet(1,65,1)==-3&&!pitActive&&bits==oldbits&&programmed==old);
    CK(pitSet(1,0,0)==0&&bits==oldbits);
    printf("PASS actual shared PIT bounds/divisor/busy checks=%u frequencies=7866\n",checks);
    return 0;
}
