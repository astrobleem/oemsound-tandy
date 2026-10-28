#ifndef BUDDY_AUDIO_H
#define BUDDY_AUDIO_H
/* Only implemented backends appear here. Future devices implement this
 * full-state interface without changing the media parser or clock. */
typedef unsigned char BYTE;
typedef struct {
    unsigned long when;
    BYTE note[4], velocity[4], retrigger;
} STATE;
typedef struct {
    const char *name;
    void (*open)(void);
    void (*state)(const STATE *state);
    void (*mute)(void);
} AUDIO;
extern const AUDIO buddy_psg, buddy_speaker, buddy_silent;
extern unsigned long audio_writes;
extern unsigned audio_mask;
#endif
