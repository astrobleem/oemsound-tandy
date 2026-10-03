/* Host-only logic check of actual driver source. Not a Windows build. */
#include <stdio.h>
#include <stdlib.h>
#include "MIDIMAP.C"
WORD GetWinFlags(void){return 0;}
HTASK GetCurrentTask(void){return 1;}
int OpenSound(void){return 3;}
int StopSound(void){return 0;}
void CloseSound(void){}
void PsgByte(unsigned value){if(value>255){puts("bad PSG byte");exit(2);}}
#define TEST(x) do { ++tested; if(!(x)) {printf("FAIL line %u\n",__LINE__);return 1;} } while(0)
int main(void)
{
    unsigned i,tested=0;
    owner=1;reset();
    TEST(noteOn(0,60,127)==0 && periodOn[0]==428);
    TEST(noteOn(1,64,1)==0 && volumeOn[1]==14);
    TEST(noteOn(2,67,64)==0);
    TEST(noteOn(3,72,96)==0 && notesOn[0]==72);
    TEST(noteOff(0,60)==0 && notesOn[0]==72);
    TEST(noteOn(9,42,100)==0 && noiseNote==42);
    TEST(shortData(0x00002a99UL)==0 && noiseNote==0);
    TEST(shortData(0x00803c90UL)==11);
    reset();
    /* Force the non-slot-ordered ages that exposed wraparound regression. */
    counter=65534; notesOn[0]=60;notesOn[1]=64;notesOn[2]=67;
    channelsOn[0]=0;channelsOn[1]=1;channelsOn[2]=2;
    age[0]=3;age[1]=2;age[2]=65534;
    TEST(noteOn(2,67,100)==0 && counter==65535);
    TEST(noteOn(2,67,100)==0 && counter==4);
    TEST(noteOn(3,72,100)==0 && notesOn[1]==72);
    for(i=0;i<100000;++i) {
        TEST(noteOn(i%8,45+i%52,1+i%127)==0);
        TEST((notesOn[0]>=45 && notesOn[0]<=96) &&
             (notesOn[1]>=45 && notesOn[1]<=96) &&
             (notesOn[2]>=45 && notesOn[2]<=96));
    }
    reset();TEST(DebugState(0)==0);
    printf("PASS %u source-logic checks including counter rollover\n",tested);
    return 0;
}
