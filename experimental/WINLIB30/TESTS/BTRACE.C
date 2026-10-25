/* Default-bank regression driver for the libtandy PSG MIDI synth.
   Prints one FNV-1a 32-bit hash over observable state only. Pass 1 adds
   mid-stream MSBank(0) calls when built with -DWITH_BANK. Argument "-v"
   prints a per-event and per-step text trace. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MIDISYN.H"

/* Baseline (libtandy-base, no MSBank) hash; all builds must match. */
#define EXPECTED_HASH 0x70D11B5DUL
#define FNV_OFFSET 2166136261UL
#define FNV_PRIME 16777619UL
#define MASK32 0xFFFFFFFFUL
#define MAX_EV 4096
#define RENDER_STEP 10UL
#define STALL_AT 1000UL
#define STALL_JUMP 5000UL
#define PASSES 2

#define SUITE_GROUPS 0
#define SUITE_CONTROL 1
#define SUITE_STALL 2
#define SUITE_SPEAKER1 3
#define SUITE_SPEAKER2 4
#define SUITE_LOW 5

#define MSG2(st,a) (((LTP_U32)(st))|(((LTP_U32)(a))<<8))
#define MSG3(st,a,b) (MSG2(st,a)|(((LTP_U32)(b))<<16))

typedef struct {
    LTP_U32 ms;
    LTP_U32 msg;
} EVENT;

static EVENT events[MAX_EV];
static unsigned event_count;
static unsigned long hash_state = FNV_OFFSET;
static unsigned long total_events;
static unsigned long total_steps;
static int verbose;
static MSYN synth;

static void die(const char *text)
{
    fprintf(stderr, "trace_default: %s\n", text);
    exit(2);
}

static void hash_byte(unsigned long b)
{
    hash_state = ((hash_state ^ (b & 0xFFUL)) * FNV_PRIME) & MASK32;
}

/* Observables are hashed as 16-bit little-endian values. */
static void hash16(unsigned long v)
{
    hash_byte(v);
    hash_byte(v >> 8);
}

static void put(LTP_U32 ms, LTP_U32 msg)
{
    if (event_count >= MAX_EV)
        die("event buffer full");
    events[event_count].ms = ms;
    events[event_count].msg = msg;
    event_count++;
}

/* Stable insertion sort by time, so same-time events keep build order. */
static void sort_events(void)
{
    unsigned i, j;
    EVENT t;
    for (i = 1; i < event_count; i++) {
        t = events[i];
        j = i;
        while (j > 0 && events[j - 1].ms > t.ms) {
            events[j] = events[j - 1];
            j--;
        }
        events[j] = t;
    }
}

/* Program groups 0..7, notes 45..100 at velocities 1/64/127, drums on
   channel 9, plus out-of-range rejections (34 melodic, 82 percussion). */
static LTP_U32 build_groups(void)
{
    static const unsigned vel[3] = {1, 64, 127};
    static const unsigned drum[8] = {35, 36, 38, 40, 42, 44, 46, 81};
    unsigned g, k, j, base, n;
    for (g = 0; g < 8; g++) {
        base = g * 1400;
        put(base, MSG2(0xC0u, g * 16));
        put(base + 30, MSG3(0x90u, 34, 64));
        put(base + 40, MSG3(0x99u, 82, 64));
        for (k = 0; k < 56; k++) {
            n = 45 + k;
            put(base + 20 + k * 20, MSG3(0x90u, n, vel[k % 3]));
            put(base + 120 + k * 20, MSG3(0x80u, n, 0));
        }
        for (j = 0; j < 8; j++) {
            put(base + 50 + j * 150, MSG3(0x99u, drum[j], vel[j % 3]));
            put(base + 90 + j * 150, MSG3(0x89u, drum[j], 0));
        }
    }
    return 8 * 1400;
}

/* Controllers 1/7/10/11/0/64/91/93/120/121/123, pitch bend, sustain
   pedal with note-offs, voice stealing and a drum on channel 9. */
static LTP_U32 build_control(void)
{
    static const unsigned cc7[4] = {0, 40, 100, 127};
    static const unsigned cc11[4] = {127, 90, 30, 1};
    static const unsigned cc1[3] = {0, 64, 127};
    static const unsigned bend[5] = {0, 8192, 16383, 4096, 12288};
    unsigned b, t, v;
    for (b = 0; b < 20; b++) {
        t = b * 400;
        put(t, MSG3(0xB0u, 7, cc7[b % 4]));
        put(t, MSG3(0xB0u, 11, cc11[b % 4]));
        put(t + 10, MSG3(0xB0u, 1, cc1[b % 3]));
        put(t + 10, MSG3(0xB0u, 64, 127));
        put(t + 20, MSG3(0x90u, 60, 64));
        put(t + 30, MSG3(0xB0u, 10, (b * 9) & 127));
        put(t + 40, MSG3(0x90u, 64, 127));
        put(t + 50, MSG3(0x99u, 36, 100));
        put(t + 60, MSG3(0x90u, 67, 1));
        put(t + 80, MSG3(0x90u, 72, 100));
        put(t + 200, MSG3(0x80u, 60, 0));
        put(t + 210, MSG3(0x80u, 64, 0));
        put(t + 220, MSG3(0x80u, 67, 0));
        put(t + 230, MSG3(0x80u, 72, 0));
        put(t + 250, MSG3(0xB0u, 64, 0));
        put(t + 250, MSG3(0x89u, 36, 0));
        put(t + 260, MSG3(0xB0u, 91, 40));
        put(t + 270, MSG3(0xB0u, 0, 0));
        put(t + 280, MSG3(0xB0u, 0, 1));
        put(t + 60, MSG3(0x91u, 48, 100));
        v = bend[b % 5];
        put(t + 80, MSG3(0xE1u, v & 127, (v >> 7) & 127));
        v = bend[(b + 2) % 5];
        put(t + 160, MSG3(0xE1u, v & 127, (v >> 7) & 127));
        put(t + 170, MSG3(0xB1u, 1, cc1[(b + 1) % 3]));
        put(t + 180, MSG3(0xB1u, 7, cc7[(b + 1) % 4]));
        put(t + 300, MSG3(0x81u, 48, 0));
        put(t + 100, MSG3(0x92u, 72, 127));
        put(t + 150, MSG3(0xB2u, 121, 0));
        put(t + 350, MSG3(0x82u, 72, 0));
        put(t + 120, MSG3(0x93u, 55, 90));
        put(t + 130, MSG3(0x93u, 52, 90));
        put(t + 140, MSG3(0x93u, 59, 90));
        put(t + 320, MSG3(0xB3u, 123, 0));
    }
    return 19 * 400 + 400;
}

/* A long GUI stall: no events between STALL_AT and STALL_AT+5000. */
static LTP_U32 build_stall(void)
{
    put(0, MSG2(0xC0u, 16));
    put(100, MSG3(0x90u, 60, 100));
    put(300, MSG3(0x90u, 64, 100));
    put(500, MSG3(0x90u, 67, 100));
    put(700, MSG3(0x90u, 71, 100));
    put(900, MSG3(0x80u, 60, 0));
    put(950, MSG3(0x80u, 64, 0));
    put(6000, MSG3(0x80u, 67, 0));
    put(6010, MSG3(0x80u, 71, 0));
    put(6100, MSG3(0x90u, 48, 90));
    put(6200, MSG3(0xE0u, 0, 96));
    put(6400, MSG3(0x80u, 48, 0));
    put(6500, MSG3(0x90u, 45, 127));
    put(6900, MSG3(0x80u, 45, 0));
    put(7000, MSG3(0xB0u, 123, 0));
    return 7100;
}

/* Speaker mode 1: channel 15 and low notes route to the PIT; CC11 0 and
   127 drive the gain-15 gate. */
static LTP_U32 build_speaker1(void)
{
    unsigned k, n, ch, t;
    for (k = 0; k < 48; k++) {
        t = k * 100;
        n = 30 + (k * 7) % 71;
        ch = (k & 1) ? 15 : 0;
        if (k % 6 == 0)
            put(t, MSG3(0xB0u | ch, 11, 0));
        put(t + 10, MSG3(0x90u | ch, n, 100));
        put(t + 70, MSG3(0x80u | ch, n, 0));
        if (k % 6 == 0)
            put(t + 80, MSG3(0xB0u | ch, 11, 127));
    }
    return 4800;
}

/* Speaker mode 2: low MIDI octave-folds into the PSG, with bends. */
static LTP_U32 build_speaker2(void)
{
    unsigned k, n, ch, t;
    for (k = 0; k < 60; k++) {
        t = k * 80;
        n = 24 + (k * 11) % 77;
        ch = k % 4;
        put(t, MSG3(0x90u | ch, n, (k * 17) % 127 + 1));
        put(t + 40, MSG3(0x80u | ch, n, 0));
        if (k % 5 == 0)
            put(t + 20, MSG3(0xE0u | ch, (k * 3) & 127, k & 127));
    }
    return 4800;
}

/* Speaker mode 0: notes 24..44 are rejected (MSMessage returns 1). */
static LTP_U32 build_low(void)
{
    unsigned k, n, t;
    for (k = 0; k < 40; k++) {
        t = k * 50;
        n = 24 + k;
        put(t, MSG3(0x90u, n, 80));
        put(t + 30, MSG3(0x80u, n, 0));
        if (k % 4 == 0)
            put(t + 40, MSG3(0x90u, n, 0));
    }
    return 2000;
}

static LTP_U32 build_suite(unsigned suite)
{
    LTP_U32 end;
    event_count = 0;
    switch (suite) {
    case SUITE_GROUPS:
        end = build_groups();
        break;
    case SUITE_CONTROL:
        end = build_control();
        break;
    case SUITE_STALL:
        end = build_stall();
        break;
    case SUITE_SPEAKER1:
        end = build_speaker1();
        break;
    case SUITE_SPEAKER2:
        end = build_speaker2();
        break;
    case SUITE_LOW:
        end = build_low();
        break;
    default:
        die("unknown suite");
        return 0;
    }
    sort_events();
    return end;
}

#ifdef WITH_BANK
static void mid_bank(unsigned pass)
{
    if (pass == 1 && MSBank(&synth, 0) != 0)
        die("mid-stream MSBank(0) rejected");
}
#else
static void mid_bank(unsigned pass)
{
    (void)pass;
}
#endif

static void observe(unsigned pass, LTP_U32 now)
{
    unsigned i;
    for (i = 0; i < 3; i++)
        hash16(synth.p[i]);
    for (i = 0; i < 4; i++)
        hash16(synth.volume[i]);
    hash16(synth.noise);
    hash16(synth.noiseNote);
    for (i = 0; i < 3; i++)
        hash16(synth.note[i]);
    for (i = 0; i < 4; i++)
        hash16(synth.released[i]);
    hash16(synth.speakerDivisor);
    hash16(synth.speakerNote);
    if (verbose) {
        printf("S %u %lu p", pass, (unsigned long)now);
        for (i = 0; i < 3; i++)
            printf(" %u", (unsigned)synth.p[i]);
        printf(" v");
        for (i = 0; i < 4; i++)
            printf(" %u", (unsigned)synth.volume[i]);
        printf(" n %u %u nt", (unsigned)synth.noise,
               (unsigned)synth.noiseNote);
        for (i = 0; i < 3; i++)
            printf(" %u", (unsigned)synth.note[i]);
        printf(" r");
        for (i = 0; i < 4; i++)
            printf(" %u", (unsigned)synth.released[i]);
        printf(" sd %u sn %u\n", (unsigned)synth.speakerDivisor,
               (unsigned)synth.speakerNote);
    }
}

static void run_suite(unsigned suite, unsigned pass, unsigned flags,
                      unsigned mode, int stall)
{
    LTP_U32 end, now;
    unsigned i, rc;
    end = build_suite(suite);
    MSReset(&synth);
#ifdef WITH_BANK
    if (MSBank(&synth, 0) != 0)
        die("MSBank(0) rejected after MSReset");
#endif
    synth.flags = (LTP_U16)flags;
    MSSpeakerMode(&synth, (LTP_U16)mode);
    if (verbose)
        printf("RUN suite %u pass %u flags %u mode %u\n", suite, pass,
               flags, mode);
    i = 0;
    now = 0;
    for (;;) {
        while (i < event_count && events[i].ms <= now) {
            if (events[i].ms != now)
                die("event not on render grid");
            mid_bank(pass);
            rc = MSMessage(&synth, now, events[i].msg);
            hash16(rc);
            if (verbose)
                printf("E %lu %08lX rc %u\n", (unsigned long)now,
                       (unsigned long)events[i].msg, rc);
            i++;
        }
        mid_bank(pass);
        MSRender(&synth, now);
        observe(pass, now);
        total_steps++;
        if (now >= end)
            break;
        now += (stall && now == STALL_AT) ? STALL_JUMP : RENDER_STEP;
        if (now > end)
            now = end;
    }
    if (i != event_count)
        die("events remain after end");
    total_events += event_count;
}

int main(int argc, char **argv)
{
    unsigned pass;
    verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);
    for (pass = 0; pass < PASSES; pass++) {
        run_suite(SUITE_GROUPS, pass, 3, 0, 0);
        run_suite(SUITE_CONTROL, pass, 3, 0, 0);
        run_suite(SUITE_CONTROL, pass, 0, 0, 0);
        run_suite(SUITE_CONTROL, pass, 1, 0, 0);
        run_suite(SUITE_STALL, pass, 3, 0, 1);
        run_suite(SUITE_SPEAKER1, pass, 3, 1, 0);
        run_suite(SUITE_SPEAKER2, pass, 3, 2, 0);
        run_suite(SUITE_LOW, pass, 3, 0, 0);
    }
    printf("hash %08lX events %lu steps %lu\n", hash_state,
           total_events, total_steps);
#ifdef EXPECTED_HASH
    if (hash_state != EXPECTED_HASH) {
        printf("MISMATCH expected %08lX\n", (unsigned long)EXPECTED_HASH);
        return 1;
    }
#endif
    return 0;
}
