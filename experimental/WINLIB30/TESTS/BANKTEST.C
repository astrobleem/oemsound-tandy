/* SPDX-License-Identifier: MIT; PSG instrument bank checks for the core. */
#include <stdio.h>
#include "MIDISYN.H"

#define STEPS 20
#define STEP_MS 55UL
#define SWITCH_K 5
#define MAX_EVENTS 10
#define SNAP_WIDTH 12
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { \
    printf("FAIL %u: %s\n", (unsigned)__LINE__, #x); ++failures; } } while (0)

typedef struct {
    unsigned char vol[STEPS];
    unsigned per[STEPS];
    unsigned char live[STEPS];
} TRACE;

typedef struct {
    LTP_U32 ms;
    unsigned status, a, b;
} EVENT;

typedef struct {
    unsigned vol[4], p[3], note[3], base[4];
} SNAP;

static LTP_U16 emit(MSYN *s, LTP_U32 ms, unsigned status, unsigned a,
                    unsigned b)
{
    return MSMessage(s, ms, (LTP_U32)status | ((LTP_U32)a << 8) |
                     ((LTP_U32)b << 16));
}

static void voice_on(MSYN *s, unsigned ch, unsigned prog, unsigned note,
                     unsigned vel)
{
    emit(s, 0, 0xc0 | ch, prog, 0);
    emit(s, 0, 0x90 | ch, note, vel);
}

static unsigned find_slot(const MSYN *s, unsigned ch, unsigned note)
{
    unsigned i;
    for (i = 0; i < 3; ++i)
        if (s->note[i] && s->channel[i] == ch && s->note[i] == note)
            return i;
    return 3;
}

/* Renders absolute step k and records the voice's volume and period.
   A voice that is not sounding is recorded as volume 15, not live. */
static void capture(MSYN *s, unsigned ch, unsigned note, unsigned k,
                    TRACE *t)
{
    unsigned i;
    MSRender(s, (LTP_U32)k * STEP_MS);
    if (ch == 9) {
        i = (note && s->noiseNote == note) ? 3 : 4;
    } else {
        i = find_slot(s, ch, note);
        if (i == 3)
            i = 4;
    }
    if (i < 4) {
        t->vol[k] = (unsigned char)s->volume[i];
        t->per[k] = i < 3 ? s->p[i] : 0;
        t->live[k] = 1;
    } else {
        t->vol[k] = 15;
        t->per[k] = 0;
        t->live[k] = 0;
    }
}

static void run(MSYN *s, unsigned ch, unsigned note, TRACE *t)
{
    unsigned k;
    for (k = 0; k < STEPS; ++k)
        capture(s, ch, note, k, t);
}

static unsigned same_n(const TRACE *a, const TRACE *b, unsigned n)
{
    unsigned k;
    for (k = 0; k < n; ++k)
        if (a->vol[k] != b->vol[k] || a->per[k] != b->per[k] ||
            a->live[k] != b->live[k])
            return 0;
    return 1;
}

static unsigned same_trace(const TRACE *a, const TRACE *b)
{
    return same_n(a, b, STEPS);
}

/* Fresh synth, bank selected, channel 0 note-on at ms 0, STEPS captured. */
static void play(MSYN *s, LTP_U16 bank, unsigned prog, unsigned note,
                 TRACE *t)
{
    MSReset(s);
    MSBank(s, bank);
    voice_on(s, 0, prog, note, 100);
    run(s, 0, note, t);
}

/* Note-on on channel ch at SWITCH_K with no earlier voices. */
static void late_on(MSYN *s, LTP_U16 bank, unsigned ch, unsigned note,
                    TRACE *t)
{
    unsigned k;
    MSReset(s);
    MSBank(s, bank);
    for (k = 0; k < STEPS; ++k) {
        if (k == SWITCH_K)
            emit(s, (LTP_U32)SWITCH_K * STEP_MS, 0x90 | ch, note, 100);
        capture(s, ch, note, k, t);
    }
}

static void take(const MSYN *s, SNAP *d)
{
    unsigned i;
    for (i = 0; i < 4; ++i) {
        d->vol[i] = s->volume[i];
        d->base[i] = s->baseVolume[i];
    }
    for (i = 0; i < 3; ++i) {
        d->p[i] = s->p[i];
        d->note[i] = s->note[i];
    }
}

static unsigned same_snap(const SNAP *a, const SNAP *b)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        if (a->vol[i] != b->vol[i] || a->base[i] != b->base[i])
            return 0;
    for (i = 0; i < 3; ++i)
        if (a->p[i] != b->p[i] || a->note[i] != b->note[i])
            return 0;
    return 1;
}

static void test_groups(void)
{
    MSYN s;
    TRACE def, def2, cha, cha2;
    unsigned g, k, ok;
    for (g = 0; g < 8; ++g) {
        play(&s, MS_BANK_DEFAULT, g * 16, 60, &def);
        play(&s, MS_BANK_DEFAULT, g * 16, 60, &def2);
        play(&s, MS_BANK_CHAMBER, g * 16, 60, &cha);
        play(&s, MS_BANK_CHAMBER, g * 16, 60, &cha2);
        CHECK(def.live[0] && cha.live[0]);
        CHECK(!same_trace(&def, &cha));
        CHECK(same_trace(&def, &def2));
        CHECK(same_trace(&cha, &cha2));
        ok = 1;
        for (k = 0; k < STEPS; ++k) {
            if (def.vol[k] > 15 || cha.vol[k] > 15)
                ok = 0;
            if (cha.live[k] && (cha.per[k] < 1 || cha.per[k] > 1023))
                ok = 0;
            if (def.live[k] && (def.per[k] < 1 || def.per[k] > 1023))
                ok = 0;
        }
        CHECK(ok);
    }
}

static void test_invalid(void)
{
    static const LTP_U16 bad[3] = {2, 255, 65535U};
    MSYN s;
    TRACE ref, got;
    unsigned base, i;
    for (base = 0; base < 2; ++base) {
        play(&s, (LTP_U16)base, 0, 60, &ref);
        for (i = 0; i < 3; ++i) {
            MSReset(&s);
            MSBank(&s, (LTP_U16)base);
            CHECK(MSBank(&s, bad[i]) != 0);
            CHECK(s.bank == base);
            voice_on(&s, 0, 0, 60, 100);
            run(&s, 0, 60, &got);
            CHECK(same_trace(&got, &ref));
        }
    }
}

static void test_repeat(void)
{
    MSYN s;
    TRACE ref, got;
    unsigned i;
    play(&s, MS_BANK_CHAMBER, 0, 60, &ref);
    MSReset(&s);
    for (i = 0; i < 3; ++i) {
        CHECK(MSBank(&s, MS_BANK_CHAMBER) == 0);
        CHECK(s.bank == MS_BANK_CHAMBER);
    }
    voice_on(&s, 0, 0, 60, 100);
    run(&s, 0, 60, &got);
    CHECK(same_trace(&got, &ref));
    for (i = 0; i < 3; ++i) {
        CHECK(MSBank(&s, MS_BANK_DEFAULT) == 0);
        CHECK(s.bank == MS_BANK_DEFAULT);
    }
}

static void test_switch(void)
{
    MSYN s;
    TRACE def, cha, held, late, late_def, late_cha;
    unsigned k;

    play(&s, MS_BANK_DEFAULT, 0, 60, &def);
    play(&s, MS_BANK_CHAMBER, 0, 60, &cha);

    MSReset(&s);
    voice_on(&s, 0, 0, 60, 100);
    for (k = 0; k < STEPS; ++k) {
        if (k == SWITCH_K)
            MSBank(&s, MS_BANK_CHAMBER);
        capture(&s, 0, 60, k, &held);
    }
    CHECK(same_trace(&held, &def));
    CHECK(s.bank == MS_BANK_CHAMBER);

    MSReset(&s);
    MSBank(&s, MS_BANK_CHAMBER);
    voice_on(&s, 0, 0, 60, 100);
    for (k = 0; k < STEPS; ++k) {
        if (k == SWITCH_K)
            MSBank(&s, MS_BANK_DEFAULT);
        capture(&s, 0, 60, k, &held);
    }
    CHECK(same_trace(&held, &cha));
    CHECK(s.bank == MS_BANK_DEFAULT);

    /* A note started after the switch takes the new bank, and the held
       Default voice is unaffected by it. */
    late_on(&s, MS_BANK_DEFAULT, 1, 64, &late_def);
    late_on(&s, MS_BANK_CHAMBER, 1, 64, &late_cha);
    CHECK(!same_trace(&late_def, &late_cha));
    MSReset(&s);
    voice_on(&s, 0, 0, 60, 100);
    for (k = 0; k < STEPS; ++k) {
        if (k == SWITCH_K) {
            MSBank(&s, MS_BANK_CHAMBER);
            emit(&s, (LTP_U32)SWITCH_K * STEP_MS, 0x91, 64, 100);
        }
        capture(&s, 1, 64, k, &late);
        capture(&s, 0, 60, k, &held);
    }
    CHECK(same_trace(&late, &late_cha));
    CHECK(same_trace(&held, &def));
}

static void test_pedal(void)
{
    MSYN s;
    TRACE def, rel, fresh, ref62;
    unsigned k;

    play(&s, MS_BANK_DEFAULT, 0, 60, &def);
    late_on(&s, MS_BANK_CHAMBER, 0, 62, &ref62);

    MSReset(&s);
    voice_on(&s, 0, 0, 60, 100);
    emit(&s, 0, 0xb0, 64, 127);
    for (k = 0; k < STEPS; ++k) {
        if (k == 2)
            emit(&s, 2 * STEP_MS, 0x80, 60, 0);
        if (k == SWITCH_K) {
            MSBank(&s, MS_BANK_CHAMBER);
            emit(&s, (LTP_U32)SWITCH_K * STEP_MS, 0x90, 62, 100);
        }
        if (k == 10)
            emit(&s, 10 * STEP_MS, 0xb0, 64, 0);
        capture(&s, 0, 60, k, &rel);
        capture(&s, 0, 62, k, &fresh);
    }
    /* Pedal-released voice keeps the Default curve until the lift. */
    CHECK(same_n(&rel, &def, 10));
    CHECK(rel.live[9] && !rel.live[10]);
    CHECK(!rel.live[STEPS - 1]);
    /* Notes started under the pedal after the switch use Chamber. */
    CHECK(same_trace(&fresh, &ref62));
    CHECK(fresh.live[STEPS - 1]);
}

static void test_reset(void)
{
    MSYN s;
    TRACE def, cha, got;

    play(&s, MS_BANK_DEFAULT, 0, 60, &def);
    play(&s, MS_BANK_CHAMBER, 0, 60, &cha);

    MSReset(&s);
    CHECK(MSBank(&s, MS_BANK_CHAMBER) == 0);
    CHECK(s.bank == MS_BANK_CHAMBER);
    MSReset(&s);
    CHECK(s.bank == MS_BANK_DEFAULT);
    voice_on(&s, 0, 0, 60, 100);
    run(&s, 0, 60, &got);
    CHECK(same_trace(&got, &def));

    MSReset(&s);
    MSBank(&s, MS_BANK_CHAMBER);
    emit(&s, 0, 0xb0, 121, 0);
    CHECK(s.bank == MS_BANK_CHAMBER);
    emit(&s, 0, 0xb0, 123, 0);
    CHECK(s.bank == MS_BANK_CHAMBER);
    voice_on(&s, 0, 0, 60, 100);
    run(&s, 0, 60, &got);
    CHECK(same_trace(&got, &cha));
}

static void test_cache(void)
{
    MSYN s;
    SNAP def_snap, first, second, third;

    MSReset(&s);
    voice_on(&s, 0, 0, 60, 100);
    MSRender(&s, (LTP_U32)SWITCH_K * STEP_MS);
    take(&s, &def_snap);
    MSBank(&s, MS_BANK_CHAMBER);
    MSRender(&s, (LTP_U32)SWITCH_K * STEP_MS);
    take(&s, &first);
    CHECK(same_snap(&def_snap, &first));

    MSReset(&s);
    MSBank(&s, MS_BANK_CHAMBER);
    voice_on(&s, 0, 0, 60, 100);
    MSRender(&s, (LTP_U32)SWITCH_K * STEP_MS);
    take(&s, &first);
    MSRender(&s, (LTP_U32)SWITCH_K * STEP_MS);
    take(&s, &second);
    CHECK(same_snap(&first, &second));
    MSBank(&s, MS_BANK_CHAMBER);
    MSRender(&s, (LTP_U32)SWITCH_K * STEP_MS);
    take(&s, &third);
    CHECK(same_snap(&first, &third));
}

static void test_percussion(void)
{
    static const unsigned notes[3] = {35, 38, 42};
    MSYN s;
    TRACE def, cha;
    unsigned i;
    for (i = 0; i < 3; ++i) {
        MSReset(&s);
        emit(&s, 0, 0x99, notes[i], 100);
        run(&s, 9, notes[i], &def);
        MSReset(&s);
        MSBank(&s, MS_BANK_CHAMBER);
        emit(&s, 0, 0x99, notes[i], 100);
        run(&s, 9, notes[i], &cha);
        CHECK(def.live[0] && cha.live[0]);
        CHECK(same_trace(&def, &cha));
    }
}

static const EVENT phrase_a[] = {
    {0, 0x90, 60, 100}, {55, 0x90, 64, 100}, {110, 0x90, 67, 100},
    {165, 0x80, 60, 0}, {220, 0x91, 72, 100}, {275, 0x80, 64, 0},
    {330, 0x90, 62, 100}, {385, 0x80, 67, 0}, {440, 0x80, 62, 0},
    {495, 0x81, 72, 0}
};

static const EVENT phrase_b[] = {
    {0, 0x90, 60, 100}, {55, 0x90, 62, 100}, {110, 0x90, 64, 100},
    {165, 0x90, 65, 100}, {220, 0x90, 67, 100},
    {275, 0x80, 64, 0}, {330, 0x80, 65, 0}, {385, 0x80, 67, 0},
    {440, 0x80, 60, 0}, {495, 0x80, 62, 0}
};

/* Records note, channel, key and age of each slot after every event. */
static void phrase(MSYN *s, LTP_U16 bank, const EVENT *ev, unsigned n,
                   unsigned snap[][SNAP_WIDTH])
{
    unsigned e, i;
    MSReset(s);
    MSBank(s, bank);
    for (e = 0; e < n; ++e) {
        emit(s, ev[e].ms, ev[e].status, ev[e].a, ev[e].b);
        for (i = 0; i < 3; ++i) {
            snap[e][i] = s->note[i];
            snap[e][3 + i] = s->channel[i];
            snap[e][6 + i] = s->key[i];
            snap[e][9 + i] = s->age[i];
        }
    }
}

static unsigned same_slots(unsigned a[][SNAP_WIDTH],
                           unsigned b[][SNAP_WIDTH], unsigned n)
{
    unsigned e, j;
    for (e = 0; e < n; ++e)
        for (j = 0; j < SNAP_WIDTH; ++j)
            if (a[e][j] != b[e][j])
                return 0;
    return 1;
}

static void test_slots(void)
{
    MSYN s;
    unsigned a[MAX_EVENTS][SNAP_WIDTH], b[MAX_EVENTS][SNAP_WIDTH];
    unsigned occupied, i;

    phrase(&s, MS_BANK_DEFAULT, phrase_a, COUNT(phrase_a), a);
    phrase(&s, MS_BANK_CHAMBER, phrase_a, COUNT(phrase_a), b);
    CHECK(same_slots(a, b, COUNT(phrase_a)));
    CHECK(a[COUNT(phrase_a) - 1][0] == 0 && a[COUNT(phrase_a) - 1][1] == 0
          && a[COUNT(phrase_a) - 1][2] == 0);

    phrase(&s, MS_BANK_DEFAULT, phrase_b, COUNT(phrase_b), a);
    phrase(&s, MS_BANK_CHAMBER, phrase_b, COUNT(phrase_b), b);
    CHECK(same_slots(a, b, COUNT(phrase_b)));
    occupied = 0;
    for (i = 0; i < 3; ++i)
        if (a[4][i])
            ++occupied;
    CHECK(occupied == 3);
    CHECK(a[4][0] == 67 || a[4][1] == 67 || a[4][2] == 67);
    CHECK(a[COUNT(phrase_b) - 1][0] == 0 && a[COUNT(phrase_b) - 1][1] == 0
          && a[COUNT(phrase_b) - 1][2] == 0);
}

static void test_mute(void)
{
    MSYN s;
    TRACE t;
    unsigned b, k, before;
    for (b = 0; b < 2; ++b) {
        MSReset(&s);
        MSBank(&s, (LTP_U16)b);
        voice_on(&s, 0, 0, 60, 100);
        for (k = 0; k < 3; ++k)
            capture(&s, 0, 60, k, &t);
        before = s.volume[0];
        CHECK(before < 15 && s.note[0] == 60);
        emit(&s, 3 * STEP_MS, 0x80, 60, 0);
        CHECK(s.volume[0] == 15 && s.note[0] == 0);
        MSRender(&s, 4 * STEP_MS);
        CHECK(s.volume[0] == 15 && s.note[0] == 0);
    }
}

int main(void)
{
    test_groups();
    test_invalid();
    test_repeat();
    test_switch();
    test_pedal();
    test_reset();
    test_cache();
    test_percussion();
    test_slots();
    test_mute();
    printf("bank: %u checks, %u failures\n", checks, failures);
    return failures != 0;
}
