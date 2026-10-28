/* SPDX-License-Identifier: MIT */
#ifndef LT_SOUND_H
#define LT_SOUND_H

/* C89; unsigned short must be 16 bits, char 8 bits. No heap or OS state. */
#include <limits.h>
#if UCHAR_MAX != 255 || USHRT_MAX != 65535U
#error libtandy requires 8-bit char and 16-bit short
#endif
#ifndef LT_FAR
#define LT_FAR
#endif
typedef unsigned char lt_u8;
typedef unsigned short lt_u16;
typedef signed char lt_i8;
typedef void (*lt_emit)(void *context, lt_u8 byte);
#define LT_TONE2_NOISE 12

typedef struct {
    lt_emit emit;
    void *context;
    lt_u16 period[3];
    lt_u8 owner[4], attenuation[4], noise;
    lt_u8 tone_valid, attenuation_valid, noise_valid;
} lt_chip;

/* init is silent; adapter must exclude every other writer while claimed. */
void lt_init(lt_chip *chip, lt_emit emit, void *context);
int lt_claim(lt_chip *chip, lt_u8 owner, lt_u8 mask);
int lt_tone(lt_chip *chip, lt_u8 owner, lt_u8 channel,
            lt_u16 raw_period, int force);
int lt_attenuate(lt_chip *chip, lt_u8 owner, lt_u8 channel,
                 lt_u8 attenuation, int force);
int lt_noise(lt_chip *chip, lt_u8 owner, lt_u8 mode, int retrigger);
/* mute before unclaim; coupled noise expands a tone2/noise stop to both. */
int lt_stop(lt_chip *chip, lt_u8 owner, lt_u8 mask);
#endif
