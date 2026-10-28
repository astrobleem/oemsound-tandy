/* Foreground producer. All times use the transport clock, never GUI time. */
#ifndef BEATPLAY_H
#define BEATPLAY_H
#include "BEATPAT.H"
#include "FXMERGE.H"
/* Safety horizon only. The caller rolls the origin before reaching this. */
#define BB_LIMIT 90000UL
#define BB_REBASE_TICKS 14400UL
#define BB_BATCH 16U
#define BB_PREFILL 512U
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
int BBRestart(BBPLAYER *,const BPAT *);
int BBRebase(BBPLAYER *,unsigned long);
unsigned BBStep(unsigned long,unsigned);
unsigned long BBStamp(unsigned long);
unsigned BBTranspose(unsigned,unsigned,unsigned,unsigned);
#endif
