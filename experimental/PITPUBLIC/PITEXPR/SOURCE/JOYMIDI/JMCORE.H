/* Pure deterministic joystick-to-MIDI state machine. No I/O or Windows. */
#ifndef JMCORE_H
#define JMCORE_H
#include "JOYCORE.H"
#define JM_MAX_MSG 3
#define JM_NOTE_MIN 60
#define JM_NOTE_MAX 72
#define JM_NOTE_GAP 165UL
#define JM_STALL 500UL
typedef struct { unsigned char status,a,b; } JM_MSG;
typedef struct { JM_MSG msg[JM_MAX_MSG]; unsigned count; int stopped; } JM_OUT;
typedef struct {
 int armed,releaseGate,sounding,note,velocity,x,y;
 unsigned pressCount,candidateCount,histNext;
 int candidate,hx[3],hy[3];
 unsigned long lastTick,lastNote;
} JM_STATE;
void jm_init(JM_STATE *s);
int jm_ready(const JOY_CAL *c,const JOY_SAMPLE *p,unsigned port,int *x,int *y);
void jm_arm(JM_STATE *s,int x,int y,unsigned long now);
void jm_stop(JM_STATE *s,JM_OUT *o);
/* Invalid input or a >500ms sampling gap mutes/disarms. Release never waits. */
void jm_tick(JM_STATE *s,int valid,int x,int y,unsigned buttons,unsigned long now,JM_OUT *o);
#endif
