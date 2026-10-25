#ifndef TCBURST_H
#define TCBURST_H
/* Additive RAWCLK v1. TCFRAME and ordinals 1..19 remain unchanged. */
#define TCB_VERSION 0x0100
#define TCB_CAPACITY 512
#define TCB_WRITES 16
#define TCB_MIN_CYCLES 9546UL /* ceil(8ms * 3579545/3 Hz) */
#define TCB_ORDERED 1
#define TCB_RAW_NOISE 2
typedef struct {WORD size,version,features,capacity,maxWrites,reserved;
 DWORD minCycles,clockNumerator,clockDenominator;} TCBINFO;
typedef struct {DWORD at;WORD count;BYTE data[TCB_WRITES];} TCBURST;
WORD TCAPI TCBGetCaps(WORD,TCBINFO FAR *,WORD);
WORD TCAPI TCBOpen(void);
WORD TCAPI TCBPush(TCBURST FAR *);
WORD TCAPI TCBEnd(void);
/* 0 success,1 ownership/lifecycle,2 FULL,3 invalid,4 spacing/horizon. */
#endif
