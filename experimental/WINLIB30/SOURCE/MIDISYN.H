#ifndef MIDISYN_H
#define MIDISYN_H
#include "LTPBASE.H"
#include "INSTR.H"
typedef struct {LTP_U16 note[3],channel[3],age[3],program[16],counter,noiseNote,noise,flags;TVOICE voice[4];LTP_U16 p[3],volume[4];LTP_U32 ms;LTP_U16 key[3],speakerMode,speakerNote,speakerChannel,
 speakerDivisor,speakerAttacks,bassFolded,pitchBend[16],bendFactor[16],modulation[16];
 LTP_U8 channelVolume[16],expression[16],pan[16],sustain[16],gain[16];
 LTP_U8 released[4],speakerReleased,baseVolume[4],renderDirty,renderValid,bank;LTP_U16 speakerBase,speakerFallback,renderFlags;LTP_U32 renderQuantum;} MSYN;
void MSReset(MSYN *);
/* Banks latch at melodic note-on. Reset restores Default; existing voices
   and percussion keep their own bank, and speaker routing is unchanged. */
#define MS_BANK_DEFAULT 0
#define MS_BANK_CHAMBER 1
LTP_U16 MSBank(MSYN *,LTP_U16);
/* 0 legacy; 1 prepared speaker; 2 octave-fold low MIDI into PSG. */
void MSSpeakerMode(MSYN *,LTP_U16);
LTP_U16 MSMessage(MSYN *,LTP_U32,LTP_U32);
/* Apply an event without rendering; bracket same-time groups with MSRender. */
LTP_U16 MSMessageDeferred(MSYN *,LTP_U32,LTP_U32);
/* Call after direct changes to voice/controller fields before MSRender.
   ms and render* are renderer-owned cache metadata; do not write them. */
void MSInvalidate(MSYN *);
void MSRender(MSYN *,LTP_U32);
LTP_U32 MSCycles(LTP_U32);
#endif
