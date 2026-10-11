/* Host tests execute real TSOUND ownership and optional PIT entry points. */
#include <stdio.h>
#include "TSOUND.C"
static HTASK task=1;
static unsigned bits,io,programs,psg;
unsigned PitRead(void){return bits;}
void PitGate(unsigned value){bits=value;++io;}
void PitProgram(unsigned divisor){++programs;}
unsigned SaveIRQ(void){return 0;}
void RestoreIRQ(unsigned f){}
void PsgByte(unsigned b){++psg;}
unsigned CreateSystemTimer(unsigned n,FARPROC f){return 1;}
void KillSystemTimer(unsigned n){}
void TimerEntry(void){}
HTASK GetCurrentTask(void){return task;}
unsigned GetWinFlags(void){return 0;}
void Yield(void){}
#define CHECK(x) do { ++tests; if(!(x)) { \
    printf("FAIL line %u\n",__LINE__);return 1; } } while(0)
int main(void)
{
    unsigned tests=0,n;
    CHECK(TandyPitVoice(1,440,1)==-1 && !io);
    owner=1;present=1;bits=0xa0;
    CHECK(StopSound()==0 && !io);
    CHECK(TandyPitVoice(1,440,1)==0 && bits==0xa3 && programs==1);
    CHECK(SetVoiceQueueSize(1,192)==0);
    CHECK(SetVoiceSound(1,440UL<<16,40)==0);
    CHECK(StartSound()==0);
    Tick();CHECK(psg && pitActive && bits==0xa3);
    task=2;n=io;
    CHECK(TandyPitVoice(1,0,0)==-1 && io==n && pitActive);
    CHECK(StopSound()==-1 && pitActive);
    CloseSound();CHECK(owner==1 && pitActive);
    task=1;
    CHECK(StopSound()==0 && !pitActive && bits==0xa0);
    CHECK(TandyPitVoice(1,523,1)==0);
    CloseSound();CHECK(!owner && !pitActive && bits==0xa0);
    owner=2;task=1;n=io;
    CHECK(TandyPitVoice(1,440,1)==-1 && io==n);
    task=2;CHECK(TandyPitVoice(1,440,1)==0);
    WEP(0);CHECK(!owner && !pitActive && bits==0xa0);
    printf("PASS %u shared-driver lease/coexistence checks; mocked Windows/I/O\n",tests);
    return 0;
}
