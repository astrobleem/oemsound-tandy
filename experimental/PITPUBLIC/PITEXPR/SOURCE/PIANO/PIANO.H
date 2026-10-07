/* Mini Piano ownership core. GPL-3.0-or-later. No OS or allocation. */
#ifndef PIANO_H
#define PIANO_H
#define PI_KEYS 13
#define PI_SOURCES 14
#define PI_MOUSE 13
#define PI_MAX_EVENTS 14
#define PI_MIN_OCTAVE 3
#define PI_MAX_OCTAVE 6
typedef struct { unsigned char status,note,velocity; } PI_EVENT;
typedef struct { unsigned char count; PI_EVENT event[PI_MAX_EVENTS]; } PI_OUT;
typedef struct {
    unsigned char octave,velocity;
    unsigned char source[PI_SOURCES];
    unsigned char refs[128];
    unsigned char held;
} PI_STATE;
void pi_init(PI_STATE *s);
void pi_press(PI_STATE *s,unsigned source,unsigned offset,PI_OUT *out);
void pi_release(PI_STATE *s,unsigned source,PI_OUT *out);
void pi_panic(PI_STATE *s,PI_OUT *out);
void pi_octave(PI_STATE *s,int delta,PI_OUT *out);
int pi_scan(unsigned scan);
#endif
