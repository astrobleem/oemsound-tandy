/* Reusable fixed-burst EX joystick decoding. No Windows or I/O dependency. */
#ifndef JOYCORE_H
#define JOYCORE_H
#define JOY_SAMPLES 512
#define JOY_TIMEOUT JOY_SAMPLES
#define JOY_AXIS_COUNT 4
/* Raw order: right X, right Y, left X, left Y. */
typedef struct {
 unsigned axis[4];
 unsigned char timed;   /* bit set for observed high-to-low transition */
 unsigned char early;   /* low at first sample: zero endpoint OR unknown */
 unsigned char timeout; /* never low during burst */
 unsigned char buttons;/* bits 0..3 pressed: R1,R2,L1,L2 */
 unsigned char last;
} JOY_SAMPLE;
typedef struct {
 unsigned low[4], high[4], center[4];
 unsigned char seen, centered;
} JOY_CAL;
void joy_decode(const unsigned char *raw, unsigned count, JOY_SAMPLE *out);
void joy_reset(JOY_CAL *cal);
void joy_track(JOY_CAL *cal, const JOY_SAMPLE *s);
void joy_center(JOY_CAL *cal, const JOY_SAMPLE *s);
/* Returns -1 unless manual center AND measured range straddle center. */
int joy_normal(const JOY_CAL *cal, unsigned axis, unsigned raw);
/* Production assembler routine: exactly JOY_SAMPLES INs, one OUT to 0201h. */
void joy_capture(unsigned char *raw);
#endif
