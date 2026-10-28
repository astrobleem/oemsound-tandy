/* SPDX-License-Identifier: MIT */
#include "lt_sound.h"

static int owns(const lt_chip *c, lt_u8 o, lt_u8 ch)
{
    return o && ch < 4 && c->owner[ch] == o;
}

void lt_init(lt_chip *c, lt_emit emit, void *context)
{
    unsigned i;
    c->emit = emit;
    c->context = context;
    for (i = 0; i < 4; ++i) {
        c->owner[i] = 0;
        c->attenuation[i] = 15;
        if (i < 3) c->period[i] = 0;
    }
    c->noise = c->tone_valid = c->attenuation_valid = c->noise_valid = 0;
}

int lt_claim(lt_chip *c, lt_u8 o, lt_u8 mask)
{
    unsigned i;
    if (!o || !mask || (mask & 240) || !c->emit) return 0;
    for (i = 0; i < 4; ++i)
        if ((mask & (1 << i)) && c->owner[i] && c->owner[i] != o)
            return 0;
    for (i = 0; i < 4; ++i)
        if (mask & (1 << i)) c->owner[i] = o;
    return 1;
}

int lt_tone(lt_chip *c, lt_u8 o, lt_u8 ch, lt_u16 p, int force)
{
    lt_u8 bit;
    if (ch >= 3 || p > 1023 || !owns(c, o, ch)) return 0;
    bit = (lt_u8)(1 << ch);
    if (force || !(c->tone_valid & bit) || c->period[ch] != p) {
        c->emit(c->context, (lt_u8)(128 | (ch << 5) | (p & 15)));
        c->emit(c->context, (lt_u8)(p >> 4));
        c->period[ch] = p;
        c->tone_valid |= bit;
    }
    return 1;
}

int lt_attenuate(lt_chip *c, lt_u8 o, lt_u8 ch, lt_u8 a, int force)
{
    lt_u8 bit;
    if (a > 15 || !owns(c, o, ch)) return 0;
    bit = (lt_u8)(1 << ch);
    if (force || !(c->attenuation_valid & bit) || c->attenuation[ch] != a) {
        c->emit(c->context, (lt_u8)(144 | (ch << 5) | a));
        c->attenuation[ch] = a;
        c->attenuation_valid |= bit;
    }
    return 1;
}

int lt_noise(lt_chip *c, lt_u8 o, lt_u8 mode, int retrigger)
{
    if (mode > 7 || !owns(c, o, 3)) return 0;
    if ((mode & 3) == 3 && !owns(c, o, 2)) return 0;
    if (retrigger || !c->noise_valid || c->noise != mode) {
        c->emit(c->context, (lt_u8)(224 | mode));
        c->noise = mode;
        c->noise_valid = 1;
    }
    return 1;
}

int lt_stop(lt_chip *c, lt_u8 o, lt_u8 mask)
{
    unsigned i;
    if (!o || !mask || (mask & 240)) return 0;
    if (c->noise_valid && (c->noise & 3) == 3 &&
        c->owner[3] == o && (mask & LT_TONE2_NOISE))
        mask |= LT_TONE2_NOISE;
    for (i = 0; i < 4; ++i)
        if ((mask & (1 << i)) && !owns(c, o, (lt_u8)i)) return 0;
    for (i = 0; i < 4; ++i)
        if (mask & (1 << i)) {
            lt_attenuate(c, o, (lt_u8)i, 15, 1);
            c->owner[i] = 0;
            c->tone_valid &= (lt_u8)~(1 << i);
            if (i == 3) c->noise_valid = 0;
        }
    return 1;
}
