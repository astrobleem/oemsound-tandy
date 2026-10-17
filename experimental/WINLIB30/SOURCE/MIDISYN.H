#ifndef MIDISYN_H
#define MIDISYN_H
#ifdef SYNTH_HOST
#include <stdint.h>
#define FAR
#define PASCAL
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t MSLONG;
typedef uint8_t BYTE;
#else
#include <windows.h>
typedef LONG MSLONG;
#endif
#include "INSTR.H"
typedef struct {WORD note[3],channel[3],age[3],program[16],counter,noiseNote,noise,flags;TVOICE voice[4];WORD p[3],volume[4];DWORD ms;} MSYN;
void MSReset(MSYN *);
WORD MSMessage(MSYN *,DWORD,DWORD);
void MSRender(MSYN *,DWORD);
DWORD MSCycles(DWORD);
#endif
