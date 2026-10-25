#ifndef PITLINK_H
#define PITLINK_H
/* Additive local SOUND lease contract. Include windows.h first. */
#define TP_VERSION 0x0100
#define TP_FIXED 1
#define TP_EXCLUSIVE 2
#define TP_SUSPEND_SAFE 4
#define TP_MIN_DIVISOR 151U
#define TP_MAX_DIVISOR 65535U
#define TP_QUERY 0
#define TP_CLAIM 1
#define TP_RELEASE 2
#define TP_MUTE 0
#define TP_SET 1
#define TP_SUSPEND 2
#define TP_REENTER 3
typedef int (FAR PASCAL *TPFAST)(WORD,WORD,WORD);
typedef struct {
    WORD size,version,features,minDivisor,maxDivisor,token;
    TPFAST fast;
} TPINFO;
typedef int (FAR PASCAL *TPGET)(WORD,TPINFO FAR *,WORD,WORD);
#endif
