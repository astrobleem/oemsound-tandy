#ifndef TCXBASS_H
#define TCXBASS_H
/* Additive local candidate; TCFRAME and TCBURST v1 are unchanged. */
#include "PITLINK.H"
#define TCX_VERSION 0x0100
#define TCX_AVAILABLE 0
#define TCX_DISABLED 1
#define TCX_UNAVAILABLE 2
#define TCX_BUSY 3
#define TCX_SUSPENDED 4
typedef struct {
    WORD size,version,status,minDivisor,maxDivisor,reserved;
} TCXINFO;
typedef struct {
    DWORD at;
    WORD count;
    BYTE data[TCB_WRITES];
    WORD pitDivisor;
} TCXBURST;
WORD TCAPI TCXGetCaps(WORD,TCXINFO FAR *,WORD);
WORD TCAPI TCXOpen(WORD);
WORD TCAPI TCXPush(TCXBURST FAR *);
WORD TCAPI TCXSetBass(WORD);
WORD TCAPI TCXStatus(void);
/* Additive complete-frame variant. Configure only before any frame queues. */
typedef struct {TCFRAME frame;WORD pitDivisor;} TCXFRAME;
WORD TCAPI TCXFrameMode(WORD);
WORD TCAPI TCXFramePush(TCXFRAME FAR *);
/* Paired complete tonal states. Percussion noise is shared by both states.
   TCFRAME and the older TCX interfaces retain their original layout. */
#define TCXP_VERSION 0x0100
#define TCXP_SHARED_NOISE 1
#define TCXP_ATOMIC_LIVE 2
#define TCXP_FALLBACK 4
#define TCXP_FORCE_FOLD 1

typedef struct {
    TCFRAME frame;
    WORD fallbackP[3],fallbackVolume,pitDivisor,flags;
} TCXPAIR;

typedef struct {
    WORD size,version,features,frameBytes,minDivisor,maxDivisor,reserved;
} TCXPAIRINFO;

WORD TCAPI TCXPairCaps(WORD,TCXPAIRINFO FAR *,WORD);
WORD TCAPI TCXPairPush(TCXPAIR FAR *);
WORD TCAPI TCXPairLive(TCXPAIR FAR *);
/* Configure TCXFrameMode before the first pair. Mask must be15; both states
   share frame.noise. FORCE_FOLD requires divisor0. Push returns0/1/2/3;
   live returns0/1/3 and requires at0. Mixed paired/unpaired queues refuse. */
#endif
