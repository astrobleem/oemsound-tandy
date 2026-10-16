/* Internal C89 client contract. This is not a driver/export wire ABI. */
#ifndef BQSCHED_H
#define BQSCHED_H

#define BQ_OK 0
#define BQ_FULL 1
#define BQ_LATER 2
#define BQ_DONE 3
#define BQ_ERROR (-1)
#define BQ_MAXMSG 8
#define BQ_LOOKAHEAD 250UL
#define BQ_LEAD 55UL

typedef unsigned long BQTIME;
typedef struct {
    BQTIME when;
    unsigned char note[4], velocity[4], retrigger;
} BQSTATE;

/* Callbacks execute only in the foreground, under an existing MIDI owner.
 * now: monotonic modulo-2^32 milliseconds in the deadline epoch.
 * put: one packed MIDI message, absolute deadline; FULL has no side effect.
 * reset: synchronous acknowledgement of flush AND owned-output silence.
 * pending: nonzero until all accepted messages, including final offs, apply.
 * All callbacks return OK/error, except put may return FULL.
 */
typedef struct {
    void *context;
    int (*now)(void *,BQTIME *);
    int (*put)(void *,BQTIME,BQTIME);
    int (*reset)(void *);
    int (*pending)(void *,unsigned *);
} BQOPS;

typedef struct {
    BQOPS ops;
    BQTIME origin, base, end, position;
    BQTIME messages[BQ_MAXMSG];
    unsigned count, index;
    int active, seed, staged, eof;
    BQSTATE planned, offered;
} BQSCHED;

unsigned BqMessages(const BQSTATE *,const BQSTATE *,BQTIME *);
int BqStart(BQSCHED *,const BQOPS *,BQTIME,BQTIME,const BQSTATE *);
int BqPosition(BQSCHED *,BQTIME *);
int BqSeed(BQSCHED *);
int BqOffer(BQSCHED *,const BQSTATE *);
int BqEnd(BQSCHED *);
int BqPause(BQSCHED *,BQTIME *);
int BqStop(BQSCHED *);

#endif
