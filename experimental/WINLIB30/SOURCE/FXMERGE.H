/* SPDX-License-Identifier: MIT. Pure C89 PSG/PIT state merger. No IO/clock. */
#ifndef FXMERGE_H
#define FXMERGE_H
typedef struct {
    unsigned period[3],level[4],noise,pit;
} FXMSTATE;
/* Slots 0..2 are tones, 3 noise, 4 speaker. Inactive slots restore base.
   borrow2 is valid only for noise: it owns tone2's divider AND mutes tone2.
   It wins over slot2 until inactive, even during a gated/silent noise row.
   A speaker slot replaces base.pit. Retrigger/cursor ownership is caller's. */
typedef struct {
    unsigned active,period,level,noise,pit,borrow2;
} FXMLAYER;
void FXMMerge(FXMSTATE *,const FXMSTATE *,const FXMLAYER *);
/* Delta encoder: at most 11 bytes. Noise latch written only on mode change,
   explicit noise attack, or initial state; level-only rows never reset LFSR.
   initial is a whole-owner start, never a per-effect global mute prefix. */
unsigned FXMBytes(const FXMSTATE *,const FXMSTATE *,unsigned,unsigned,
                  unsigned char *);
#endif
