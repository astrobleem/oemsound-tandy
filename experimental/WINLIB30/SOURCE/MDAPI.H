#ifndef MD_API_H
#define MD_API_H
#include <windows.h>
#define MD_VERSION 0x0100
#define MD_CAPACITY 1024
#define MD_MIN_MS 8
#define MD_OK 0
#define MD_BUSY 1
#define MD_FULL 2
#define MD_BAD 3
#define MD_STALE 4
#define MD_SPACING 5
#define MD_IO 6
#define MD_LATE 7
#define MD_EOF 1
#define MD_OPEN 1
#define MD_RUNNING 2
#define MD_PAUSED 3
#define MD_DONE 4
#define MD_FAILED 5
/* Win16 natural2-byte alignment; size/ABI is returned in fields. */
typedef struct {WORD size,version,state,error,capacity,queued;DWORD generation,position,accepted,delivered;WORD minSpacingMs,psgTones,noiseVoices,flags;DWORD coalesced;} MDINFO;
#ifdef MD_IMPLEMENTATION
#define MDAPI FAR PASCAL _loadds
#else
#define MDAPI FAR PASCAL
#endif
WORD MDAPI MDOpen(WORD FAR *);
WORD MDAPI MDPush(WORD,DWORD,DWORD,DWORD);
WORD MDAPI MDCommit(WORD,DWORD,DWORD,WORD);
WORD MDAPI MDStart(WORD);
WORD MDAPI MDPump(WORD);
WORD MDAPI MDPause(WORD);
WORD MDAPI MDResume(WORD);
WORD MDAPI MDReset(WORD,DWORD);
WORD MDAPI MDClose(WORD);
WORD MDAPI MDStatus(WORD,MDINFO FAR *,WORD);
WORD MDAPI MDSeed(WORD,DWORD,DWORD,DWORD);
WORD MDAPI MDTiming(WORD,WORD);
DWORD MDAPI MDVersion(void);
WORD MDAPI MDClock(WORD,DWORD FAR *);
WORD MDAPI MDVoiceMode(WORD,WORD);
WORD MDAPI MDDebug(WORD,WORD);
/* Stage a live generation; MDStart commits owner-only no-mute replacement. */
WORD MDAPI MDBeginLive(WORD,DWORD);
#endif
