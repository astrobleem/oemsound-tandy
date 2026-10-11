/* Host-only mocked I/O tests of the exact core used by SOUND. */
#include <stdio.h>
#define _cdecl
static unsigned bits,writes,programs,lastDivisor;
unsigned PitRead(void) { return bits; }
void PitGate(unsigned value) { bits=value; ++writes; }
void PitProgram(unsigned divisor) { lastDivisor=divisor; ++programs; }
#include "PITCORE.H"
#define CHECK(x) do { ++tests; if(!(x)) { \
    printf("FAIL line %u\n",__LINE__);return 1; } } while(0)
int main(void)
{
    unsigned i,tests=0,n;
    bits=0xa0;
    CHECK(pitSet(1,0,0)==0 && writes==0 && programs==0);
    CHECK(pitSet(2,440,1)==-16 && writes==0);
    CHECK(pitSet(1,440,2)==-16 && writes==0);
    CHECK(pitSet(1,36,1)==-13 && writes==0);
    CHECK(pitSet(1,7903,1)==-13 && writes==0);
    for(i=0;i<256;i+=4) {
        bits=i;pitActive=0;
        CHECK(pitSet(1,440,1)==0 && bits==(i|3));
        CHECK(lastDivisor==2712);
        /* Unrelated bits may change during a note; stop preserves them. */
        bits^=0x90;
        CHECK(pitSet(1,0,0)==0 && bits==(i^0x90));
        n=writes;
        CHECK(pitSet(1,0,0)==0 && writes==n);
    }
    for(i=1;i<=3;++i) {
        bits=0xa0|i;pitActive=0;n=writes;
        CHECK(pitSet(1,440,1)==-3 && bits==(0xa0|i) && writes==n);
    }
    bits=0;
    CHECK(pitSet(1,37,1)==0 && lastDivisor==32248);
    CHECK(pitSet(1,7902,1)==0 && lastDivisor==151);
    pitStop();CHECK(!pitActive && bits==0);
    printf("PASS %u PIT source checks; mocked I/O only\n",tests);
    return 0;
}
