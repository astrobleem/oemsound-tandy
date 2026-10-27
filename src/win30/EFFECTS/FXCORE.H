/* SPDX-License-Identifier: MIT */
#ifndef FXCORE_H
#define FXCORE_H
#include <stdio.h>
#define FX_FIELDS 12
#define FX_TICKS 300
#define FX_TEXT 256
enum {FX_PITCH,FX_SLIDE,FX_DEPTH,FX_RATE,FX_ATTACK,FX_HOLD,FX_RELEASE,
      FX_LEVEL,FX_NOISE,FX_ON,FX_OFF,FX_TARGET};
typedef struct {int v[FX_FIELDS];unsigned seed;} FXPATCH;
/* PSG period, attenuation (15=mute), noise mode, PIT divisor (0=gate off). */
typedef struct {unsigned period,level,noise,pit;} FXROW;
typedef struct {unsigned count,target,mask;FXROW row[FX_TICKS+1];} FXCLIP;
int FXValid(const FXPATCH *p);
void FXPreset(FXPATCH *p,unsigned n);
void FXRandom(FXPATCH *p,int mutate);
int FXEnabled(const FXPATCH *p,unsigned field);
void FXAdjust(FXPATCH *p,unsigned field,int delta);
int FXCompile(const FXPATCH *p,FXCLIP *out);
unsigned FXBytes(const FXCLIP *c,unsigned tick,unsigned char *bytes);
unsigned FXEncode(const FXPATCH *p,char *text,unsigned capacity);
int FXDecode(FXPATCH *p,const char *text,unsigned bytes);
int FXName(const char *name);
int FXLoad(FXPATCH *p,const char *path);
/* 0 success, 1 invalid, 2 IO, 3 exists, 4 recovery in .BAK. */
int FXSave(const FXPATCH *p,const char *path,int replace);
int FXHeader(const FXCLIP *c,FILE *f);
extern const char *FXNames[FX_FIELDS];
extern const char *FXNoiseNames[9];
#endif
