/* Independently written bounded raw-score player. MIT; see LICENSE.TXT. */
#ifndef RAWFILE_H
#define RAWFILE_H
#ifdef _MSC_VER
#define RF_FAR _far
typedef unsigned long RF32;
#else
#include <stdint.h>
#define RF_FAR
typedef uint32_t RF32;
#endif
typedef unsigned char RF8;
typedef unsigned short RF16;
#define RF_MAX 32768U
#define RF_PATTERNS 16
#define RF_EVENTS (RF_PATTERNS*4*64*3)
/* Error values: 1 structure, 2 unsupported profile, 3 runtime bound. */
typedef struct {
 RF8 RF_FAR *data; RF8 RF_FAR *events; RF16 size,kind;
 RF16 start[4],end[4],cursor[4],left[4],done[4];
 RF16 period[4],volume[4],noise,dirty;
 RF16 order[255],length,patterns,loop,speed,hz,position,row,tick;
 RF16 freq[3],target[3],note[3],octave[3],fx[3],param[3],frac[3],arp[3];
 RF32 debt,ticks; RF16 error;
} RFSong;
RF16 RFLoad(RFSong *,RF8 RF_FAR *,RF16,RF8 RF_FAR *,RF16);
void RFReset(RFSong *);
RF16 RFStep(RFSong *);
RF16 RFAdvance(RFSong *,RF16);
#endif
