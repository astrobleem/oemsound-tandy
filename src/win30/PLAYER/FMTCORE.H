#ifndef FMTCORE_H
#define FMTCORE_H
#include "VGMGRP.H"
#include "MODPLAY.H"
#define FX_VGM 3
#define FX_MOD 4
#define FX_LIMIT 65535UL
#define FX_MIN 9546UL
/* TCPush refuses timestamps above 2^31-1 PIT cycles (about 30 minutes), so
   VGM groups spaced >= FX_MIN can never exceed this count. */
#define FX_VGM_GROUPS (2147483647UL/FX_MIN+2UL)
typedef struct {unsigned long at;unsigned short count;unsigned char data[16];} FXBURST;
typedef struct {
 VGM_BURST_READER vgm;MOD_Module mod;MOD_Player player;MOD_Error modError;
 unsigned short kind,error,done,terminal,looped;
 unsigned long nextAt,lastAt,groups,sourceOffset,loopOffset,loopAt;
 unsigned short phase,period[3];unsigned char lane[4],volume[3];
} FXCORE;
/* Foreground-only bounded source logic; no OS, allocation, ownership or clock. */
int FXOpen(FXCORE *,VGM_READ_AT,void *,unsigned long,unsigned short);
int FXNext(FXCORE *,FXBURST *); /*1 group,0 complete,-1 refusal*/
int FXPreflight(FXCORE *,VGM_READ_AT,void *,unsigned long,unsigned short);
const char *FXReason(FXCORE *);
#endif
