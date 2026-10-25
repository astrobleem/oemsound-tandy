#ifndef TANDY_INSTR_H
#define TANDY_INSTR_H
#include "LTPBASE.H"
/* Original compact PSG presets. */
typedef struct {
    LTP_U32 began;
    LTP_U16 period;
    LTP_U8 velocity,preset,modulation;
} TVOICE;
void InstStart(TVOICE *,unsigned,unsigned,unsigned,LTP_U32);
int InstRead(TVOICE *,LTP_U32,LTP_U16,LTP_U16 *,LTP_U16 *);

#endif
