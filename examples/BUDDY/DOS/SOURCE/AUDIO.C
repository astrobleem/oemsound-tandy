/* Buddy hardware sinks. Serialized by the application's libtandy clock. */
#include <conio.h>
#include <string.h>
#include "AUDIO.H"
#include "PERIODS.H"
#include "SPEAKER.H"
#include "lt_sound.h"
extern void __cdecl PsgByte(unsigned);
static lt_chip chip;
static STATE previous;
static unsigned speaker_note;
unsigned long audio_writes;
unsigned audio_mask;
static void emit(void *context, lt_u8 value)
{
    (void)context;
    PsgByte(value);
    ++audio_writes;
}
static void psg_mute(void)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        lt_attenuate(&chip, 1, (lt_u8)i, 15, 1);
    memset(&previous, 0, sizeof(previous));
    audio_mask = 0;
}
static void psg_open(void)
{
    lt_init(&chip, emit, 0);
    lt_claim(&chip, 1, 15);
    psg_mute();
}
static void psg_state(const STATE *s)
{
    unsigned i, n, attenuation, noise;
    for (i = 0; i < 4; ++i) {
        if (previous.note[i] == s->note[i] &&
            previous.velocity[i] == s->velocity[i] &&
            !(s->retrigger & (1U << i))) continue;
        lt_attenuate(&chip, 1, (lt_u8)i, 15, 1);
        audio_mask &= ~(1U << i);
        n = s->note[i];
        if (!n) continue;
        attenuation = 14 - ((s->velocity[i] - 1U) * 14) / 126;
        if (i < 3)
            lt_tone(&chip, 1, (lt_u8)i, periods[n - 45], 1);
        else {
            noise = 6;
            if (n == 35 || n == 36) noise = 2;
            else if (n == 38 || n == 40) noise = 5;
            else if (n == 42 || n == 44 || n == 46) noise = 4;
            lt_noise(&chip, 1, (lt_u8)noise, 1);
        }
        lt_attenuate(&chip, 1, (lt_u8)i, (lt_u8)attenuation, 1);
        audio_mask |= 1U << i;
    }
    previous = *s;
}
static void speaker_mute(void)
{
    outp(0x61, inp(0x61) & 0xFC);
    speaker_note = audio_mask = 0;
}
static void speaker_state(const STATE *s)
{
    unsigned n = s->note[1], divisor;
    if (n == speaker_note && !(s->retrigger & 2)) return;
    speaker_mute();
    if (!n) return;
    divisor = speaker_periods[n];
    outp(0x43, 0xB6);
    outp(0x42, divisor & 255);
    outp(0x42, divisor >> 8);
    outp(0x61, inp(0x61) | 3);
    speaker_note = n;
    audio_mask = 2;
    ++audio_writes;
}
static void silent_open(void) { audio_mask = 0; }
static void silent_state(const STATE *s) { (void)s; }
const AUDIO buddy_psg = { "Tandy PSG", psg_open, psg_state, psg_mute };
const AUDIO buddy_speaker = {
    "PC speaker", speaker_mute, speaker_state, speaker_mute
};
const AUDIO buddy_silent = {
    "Silent", silent_open, silent_state, silent_open
};
