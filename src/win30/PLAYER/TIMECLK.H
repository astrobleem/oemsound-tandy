#define TC_CAP 1024
#ifdef TC_DRIVER
#define TCAPI FAR PASCAL _loadds
#else
#define TCAPI FAR PASCAL
#endif
/* Absolute PIT-cycle timestamp plus complete PSG state, producer precomputed. */
typedef struct {DWORD at;WORD p[3],volume,noise,mask;} TCFRAME;
WORD TCAPI TCOpen(void);
WORD TCAPI TCPush(TCFRAME FAR *);
WORD TCAPI TCStart(void);
WORD TCAPI TCEnd(void);
WORD TCAPI TCClose(void);
WORD TCAPI TCDebug(WORD);
WORD TCAPI TCProbe(WORD);
WORD TCAPI TCProbeOff(void);
DWORD TCAPI TCStamp(void);

WORD TCAPI TCResume(void);

DWORD TCAPI TCClock(void);
WORD TCAPI TCWideOpen(void);
WORD TCAPI TCPause(void);
WORD TCAPI TCUnpause(void);

WORD TCAPI TCFlush(void);
WORD TCAPI TCRearm(void);
WORD TCAPI TCMask(void);

WORD TCAPI TCLiveApply(TCFRAME FAR *);
