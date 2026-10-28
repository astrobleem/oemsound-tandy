/* Foreground producer. All times use the transport clock, never GUI time. */
#ifndef BEATPLAY_H
#define BEATPLAY_H
#include "BEATPAT.H"
#include "FXMERGE.H"
/* RAWCLK's literal-burst horizon is <2^31 PIT cycles. 25 minutes/session. */
#define BB_LIMIT 90000UL
#define BB_BATCH 16U
#define BB_PREFILL 128U
typedef struct {
    unsigned long at;
    unsigned count,pit;
    unsigned char data[16];
} BBBURST;
typedef struct {
    BPAT song;
    FXCLIP clip[5];
    unsigned cursor[5],pitch[5],lastStep,noiseFX;
    unsigned long tick;
    FXMSTATE old;
} BBPLAYER;
int BBPrepare(BBPLAYER *,const BPAT *);
int BBNext(BBPLAYER *,BBBURST *);
unsigned BBStep(unsigned long,unsigned);
unsigned long BBStamp(unsigned long);
unsigned BBTranspose(unsigned,unsigned,unsigned,unsigned);
#endif
