/* psgaudit.c - developer A/B audition of the MIDI PSG instrument banks.

   Each scene's event list is built once and sent through the real synth
   (MSReset, MSBank, MSMessage, MSRender from sound/MIDISYN.C) once per
   bank. Only MSBank differs between the two takes.

   The PSG output stage below is a HOST APPROXIMATION of an SN76496: square
   tones, a 15-bit LFSR noise voice and 2 dB attenuation steps, with fixed
   per-voice gains and no normalization. It is NOT Tandy 1000 output.
   Host-only developer tool; no driver or foreground code uses it.

   Usage: psgaudit <outdir>  (outdir must already exist)
   The directory must already exist. Writes <scene>_default.wav,
   <scene>_chamber.wav, <scene>_ab.wav and report.txt. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MIDISYN.H"

#define AUD_RATE 44100UL
#define AUD_CLOCK 3579545.0
#define TONE_GAIN 0.2
#define NOISE_GAIN 0.2
#define TAIL_MS 1200UL
#define SILENCE_MS 500UL
#define UNIT_MS 150UL
#define GATE_TRIM 10UL
#define MAX_EVENTS 1024
#define SCENE_COUNT 4
#define BANK_COUNT 2
#define TONE_VOICES 3
#define PERC_CH 9
#define SC_ATTACKS 0
#define SC_HELD 1
#define SC_RELEASE 2
#define SC_PHRASE 3
#define MAX_MELODIC 3
#define PATH_LEN 1024

typedef struct {
    LTP_U32 ms;
    LTP_U32 msg;
} EVENT;

typedef struct {
    EVENT ev[MAX_EVENTS];
    unsigned count;
    unsigned errors;
} EVENT_LIST;

typedef struct {
    unsigned long noteOns;
    unsigned long noteOffs;
    unsigned long percussion;
    unsigned long controls;
    unsigned long bends;
    unsigned long programs;
    unsigned long maxMelodic;
    unsigned long checksum;
} SCENE_INFO;

typedef struct {
    short *data;
    unsigned long n;
    unsigned long rejected;
    unsigned long steals;
    unsigned long clipped;
    unsigned long peak;
    double rawPeak;
    double sumSq;
} RENDER;

static const char *const scene_names[SCENE_COUNT] = {
    "attacks", "held", "release", "phrase"
};

static const char *const scene_desc[SCENE_COUNT] = {
    "8 staccato notes, one per program group (program g*16), "
        "150 ms each, 100 ms gaps, channel 1.",
    "8 notes, one per program group, each held 1.2 s then off, "
        "played in sequence.",
    "CC64 pedal down in 3 windows (groups 0-2, 3-5, 6-7), each note "
        "held by the pedal, pedal lifted after each window.",
    "Original 8-bar phrase. Melody ch1 (programs 0,16,48,80,96), "
        "bass ch2 (program 32, pad at bar 5), chord ch3 (program 64, "
        "bars 4 and 8), kick/snare/hat on ch10 (notes 36,38,42). "
        "CC7/CC11/CC1 and one pitch-bend gesture."
};

static const char *const bank_names[BANK_COUNT] = {
    "default", "chamber"
};

static MSYN synth;
static EVENT_LIST list;
static SCENE_INFO infos[SCENE_COUNT];
static RENDER result[SCENE_COUNT][BANK_COUNT];
static unsigned long diffs[SCENE_COUNT];

static int IsNoteOn(LTP_U32 msg)
{
    return ((msg & 0xF0UL) == 0x90UL) && (((msg >> 16) & 0xFFUL) != 0UL);
}

static LTP_U32 MakeMsg(unsigned status, unsigned a, unsigned b)
{
    return (LTP_U32)status | ((LTP_U32)a << 8) | ((LTP_U32)b << 16);
}

/* Stable insertion by time. At equal times a note-on sorts after every
   other event, so note-offs and controllers always precede it. */
static void AddEvent(LTP_U32 ms, LTP_U32 msg)
{
    unsigned i;
    int rank = IsNoteOn(msg) ? 1 : 0;

    if (list.count >= MAX_EVENTS) {
        list.errors++;
        return;
    }
    i = list.count++;
    while (i > 0) {
        EVENT *prev = &list.ev[i - 1];
        int prevRank = IsNoteOn(prev->msg) ? 1 : 0;

        if (prev->ms < ms || (prev->ms == ms && prevRank <= rank))
            break;
        list.ev[i] = *prev;
        i--;
    }
    list.ev[i].ms = ms;
    list.ev[i].msg = msg;
}

static void AddNoteOn(LTP_U32 ms, unsigned ch, unsigned note, unsigned vel)
{
    AddEvent(ms, MakeMsg(0x90U | ch, note, vel));
}

static void AddNoteOff(LTP_U32 ms, unsigned ch, unsigned note)
{
    AddEvent(ms, MakeMsg(0x80U | ch, note, 0));
}

static void AddCc(LTP_U32 ms, unsigned ch, unsigned cc, unsigned val)
{
    AddEvent(ms, MakeMsg(0xB0U | ch, cc, val));
}

static void AddProgram(LTP_U32 ms, unsigned ch, unsigned prog)
{
    AddEvent(ms, MakeMsg(0xC0U | ch, prog, 0));
}

static void AddBend(LTP_U32 ms, unsigned ch, unsigned value)
{
    AddEvent(ms, MakeMsg(0xE0U | ch, value & 127U, (value >> 7) & 127U));
}

/* Scene 1: staccato attack on each program group, channel 0. */
static void BuildAttacks(void)
{
    unsigned g;

    for (g = 0; g < 8; g++) {
        LTP_U32 t = (LTP_U32)g * 250UL;

        AddProgram(t, 0, g * 16U);
        AddNoteOn(t, 0, 60, 100);
        AddNoteOff(t + 150UL, 0, 60);
    }
}

/* Scene 2: each group holds a note for 1.2 s, in sequence, channel 0. */
static void BuildHeld(void)
{
    unsigned g;

    for (g = 0; g < 8; g++) {
        LTP_U32 t = (LTP_U32)g * 1300UL;

        AddProgram(t, 0, g * 16U);
        AddNoteOn(t, 0, 60, 100);
        AddNoteOff(t + 1200UL, 0, 60);
    }
}

/* Scene 3: three pedal windows. At most three notes are held by the pedal
   in any window, so the synth never reclaims a pedal-held voice. */
static void BuildRelease(void)
{
    static const unsigned char first[3] = {0, 3, 6};
    static const unsigned char count[3] = {3, 3, 2};
    unsigned w, j;
    LTP_U32 base = 0UL;

    for (w = 0; w < 3; w++) {
        LTP_U32 lift;

        AddCc(base, 0, 64, 127);
        for (j = 0; j < count[w]; j++) {
            unsigned g = first[w] + j;
            LTP_U32 t = base + (LTP_U32)j * 300UL;

            AddProgram(t, 0, g * 16U);
            AddNoteOn(t, 0, 60, 100);
            AddNoteOff(t + 200UL, 0, 60);
        }
        lift = base + (LTP_U32)count[w] * 300UL + 100UL;
        AddCc(lift, 0, 64, 0);
        base = lift + 200UL;
    }
}

/* Scene 4: original 8-bar phrase, 64 units of 150 ms (9.6 s). Melody and
   bass never exceed three sounding melodic notes together; the chord
   bars drop the bass so melody + two chord tones make three. */
static void BuildPhrase(void)
{
    static const unsigned char melP[] = {
        72, 74, 72, 74, 72, 67, 69, 72, 71, 69, 67, 65, 64, 62, 64, 67,
        72, 76, 79, 76, 81, 79, 77, 76, 74, 72, 71, 69, 67, 72
    };
    static const unsigned char melU[] = {
        2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4
    };
    static const unsigned char bassP[] = {
        48, 55, 45, 52, 53, 48, 0, 55, 48, 45, 53, 50, 45, 0, 48
    };
    static const unsigned char bassU[] = {
        4, 4, 4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4
    };
    static const unsigned char percU[6] = {0, 12, 20, 26, 48, 60};
    static const unsigned char percN[6] = {36, 38, 36, 42, 36, 38};
    unsigned i, u = 0;
    unsigned melCount = sizeof(melP) / sizeof(melP[0]);
    unsigned bassCount = sizeof(bassP) / sizeof(bassP[0]);
    LTP_U32 t;

    if (melCount != sizeof(melU) / sizeof(melU[0]) ||
        bassCount != sizeof(bassU) / sizeof(bassU[0])) {
        list.errors++;
        return;
    }

    /* Melody, channel 0. */
    for (i = 0; i < melCount; i++) {
        t = (LTP_U32)u * UNIT_MS;
        AddNoteOn(t, 0, melP[i], 100);
        AddNoteOff(t + (LTP_U32)melU[i] * UNIT_MS - GATE_TRIM, 0, melP[i]);
        u += melU[i];
    }
    if (u != 64) list.errors++;

    /* Bass, channel 1. */
    u = 0;
    for (i = 0; i < bassCount; i++) {
        t = (LTP_U32)u * UNIT_MS;
        if (bassP[i]) {
            AddNoteOn(t, 1, bassP[i], 90);
            AddNoteOff(t + (LTP_U32)bassU[i] * UNIT_MS - GATE_TRIM,
                       1, bassP[i]);
        }
        u += bassU[i];
    }

    /* Chord, channel 2: two tones in bar 4 and bar 8. */
    t = 24UL * UNIT_MS;
    AddNoteOn(t, 2, 65, 70);
    AddNoteOn(t, 2, 69, 70);
    AddNoteOff(t + 4UL * UNIT_MS - GATE_TRIM, 2, 65);
    AddNoteOff(t + 4UL * UNIT_MS - GATE_TRIM, 2, 69);
    t = 56UL * UNIT_MS;
    AddNoteOn(t, 2, 60, 70);
    AddNoteOn(t, 2, 64, 70);
    AddNoteOff(t + 4UL * UNIT_MS - GATE_TRIM, 2, 60);
    AddNoteOff(t + 4UL * UNIT_MS - GATE_TRIM, 2, 64);

    /* Percussion, channel 10 (index 9). */
    for (i = 0; i < 6; i++) {
        unsigned vel = percN[i] == 42 ? 80 : (percN[i] == 38 ? 100 : 110);

        AddNoteOn((LTP_U32)percU[i] * UNIT_MS, PERC_CH, percN[i], vel);
    }

    /* Programs and controllers. */
    AddProgram(0UL, 0, 0);
    AddProgram(0UL, 1, 32);
    AddProgram(0UL, 2, 64);
    AddCc(0UL, 0, 7, 100);
    AddCc(0UL, 0, 11, 110);
    AddCc(0UL, 0, 1, 0);
    AddProgram(16UL * UNIT_MS, 0, 16);
    AddCc(16UL * UNIT_MS, 0, 11, 90);
    AddProgram(32UL * UNIT_MS, 0, 48);
    AddCc(32UL * UNIT_MS, 0, 1, 100);
    AddCc(32UL * UNIT_MS, 0, 7, 127);
    AddProgram(32UL * UNIT_MS, 1, 48);
    AddCc(32UL * UNIT_MS, 1, 1, 127);
    AddProgram(48UL * UNIT_MS, 0, 80);
    AddCc(48UL * UNIT_MS, 0, 1, 40);
    AddProgram(48UL * UNIT_MS, 1, 32);
    AddCc(48UL * UNIT_MS, 1, 1, 0);
    AddBend(50UL * UNIT_MS, 0, 12288UL);
    AddBend(52UL * UNIT_MS, 0, 8192UL);
    AddProgram(56UL * UNIT_MS, 0, 96);
    AddCc(56UL * UNIT_MS, 0, 11, 110);
}

static void AnalyzeList(SCENE_INFO *si)
{
    unsigned i, status, ch, vel;
    unsigned long active = 0UL;
    LTP_U32 m;

    memset(si, 0, sizeof(*si));
    for (i = 0; i < list.count; i++) {
        m = list.ev[i].msg;
        status = (unsigned)(m & 0xF0UL);
        ch = (unsigned)(m & 0x0FUL);
        vel = (unsigned)((m >> 16) & 0xFFUL);
        si->checksum = si->checksum * 31UL
            + (unsigned long)list.ev[i].ms * 7UL + (unsigned long)m;
        if (status == 0x90 && vel != 0 && ch != PERC_CH) {
            si->noteOns++;
            active++;
            if (active > si->maxMelodic) si->maxMelodic = active;
        } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
            if (ch != PERC_CH) {
                si->noteOffs++;
                if (active) active--;
            }
        } else if (status == 0x90) {
            si->percussion++;
        } else if (status == 0xB0) {
            si->controls++;
        } else if (status == 0xC0) {
            si->programs++;
        } else if (status == 0xE0) {
            si->bends++;
        }
    }
}

static int BuildScene(unsigned scene)
{
    list.count = 0;
    list.errors = 0;
    switch (scene) {
    case SC_ATTACKS:
        BuildAttacks();
        break;
    case SC_HELD:
        BuildHeld();
        break;
    case SC_RELEASE:
        BuildRelease();
        break;
    default:
        BuildPhrase();
        break;
    }
    if (list.errors || list.count == 0) return 1;
    AnalyzeList(&infos[scene]);
    return 0;
}

/* Attenuation: 2 dB per step, amplitude 10^(-0.1*v). 15 is silent. */
static double Amp(unsigned v)
{
    static double table[16];
    static int ready = 0;
    unsigned i;

    if (!ready) {
        table[0] = 1.0;
        for (i = 1; i < 15; i++)
            table[i] = table[i - 1] * 0.7943282347242815;
        table[15] = 0.0;
        ready = 1;
    }
    return table[v > 15 ? 15 : v];
}

static double SqrtD(double x)
{
    double g;
    unsigned i;

    if (x <= 0.0) return 0.0;
    g = x > 1.0 ? x : 1.0;
    for (i = 0; i < 100; i++) g = 0.5 * (g + x / g);
    return g;
}

static int PrepareRender(RENDER *r, unsigned long n)
{
    memset(r, 0, sizeof(*r));
    r->n = n;
    r->data = (short *)malloc(n * sizeof(short));
    return r->data ? 0 : 1;
}

/* Renders the current list through the real synth for one bank. The
   event delivery and MSRender calls are identical for both banks. */
static int RenderBank(unsigned bank, RENDER *r)
{
    LTP_U32 ms = 0UL, lastMs = 0xFFFFFFFFUL;
    unsigned long n, acc = 0UL;
    unsigned ei = 0, i, nz, ndiv;
    unsigned lfsr = 0x4000U, fb;
    double tp[TONE_VOICES], np = 0.0, sum, val, a;
    short s;

    MSReset(&synth);
    if (MSBank(&synth, (LTP_U16)bank) != 0) return 1;
    for (i = 0; i < TONE_VOICES; i++) tp[i] = 0.0;

    for (n = 0; n < r->n; n++) {
        while (ei < list.count && list.ev[ei].ms <= ms) {
            LTP_U32 m = list.ev[ei].msg;
            int melodicOn = IsNoteOn(m) && ((m & 0x0FUL) != PERC_CH);

            if (melodicOn && synth.note[0] && synth.note[1] &&
                synth.note[2])
                r->steals++;
            if (MSMessage(&synth, ms, m)) r->rejected++;
            ei++;
        }
        if (ms != lastMs) {
            MSRender(&synth, ms);
            lastMs = ms;
        }

        sum = 0.0;
        for (i = 0; i < TONE_VOICES; i++) {
            unsigned p = synth.p[i];

            if (synth.note[i] && p) {
                tp[i] += AUD_CLOCK / (32.0 * (double)p) / (double)AUD_RATE;
                while (tp[i] >= 1.0) tp[i] -= 1.0;
                sum += (tp[i] < 0.5 ? 1.0 : -1.0)
                    * Amp(synth.volume[i]) * TONE_GAIN;
            }
        }

        /* Noise: bit 2 selects white (1) or periodic (0), bits 0-1 the
           rate: div 16/32/64. Setting 3 (tone-2 clocked) is treated as
           div 64 here; MIDISYN.C never produces it. */
        nz = synth.noise & 7U;
        ndiv = (nz & 3U) == 3U ? 64U : (16U << (nz & 3U));
        np += AUD_CLOCK / (32.0 * (double)ndiv) / (double)AUD_RATE;
        while (np >= 1.0) {
            np -= 1.0;
            fb = (nz & 4U) ? ((lfsr ^ (lfsr >> 3)) & 1U) : (lfsr & 1U);
            lfsr = (lfsr >> 1) | (fb << 14);
        }
        if (synth.volume[3] < 15) {
            sum += ((lfsr & 1U) ? 1.0 : -1.0)
                * Amp(synth.volume[3]) * NOISE_GAIN;
        }

        a = sum < 0.0 ? -sum : sum;
        if (a > r->rawPeak) r->rawPeak = a;

        val = sum * 32767.0;
        if (val > 32767.0) {
            val = 32767.0;
            r->clipped++;
        } else if (val < -32768.0) {
            val = -32768.0;
            r->clipped++;
        }
        s = (short)(val < 0.0 ? val - 0.5 : val + 0.5);
        r->data[n] = s;
        {
            unsigned long mag = s < 0 ? (unsigned long)(-(long)s)
                                      : (unsigned long)s;

            if (mag > r->peak) r->peak = mag;
        }
        r->sumSq += (double)s * (double)s;

        acc += 1000UL;
        if (acc >= AUD_RATE) {
            acc -= AUD_RATE;
            ms++;
        }
    }
    return 0;
}

static void PutLe16(FILE *f, unsigned long v)
{
    fputc((int)(v & 0xFFUL), f);
    fputc((int)((v >> 8) & 0xFFUL), f);
}

static void PutLe32(FILE *f, unsigned long v)
{
    PutLe16(f, v & 0xFFFFUL);
    PutLe16(f, (v >> 16) & 0xFFFFUL);
}

/* Canonical 44-byte PCM WAV header, mono 16-bit 44100 Hz. */
static int WriteWav(const char *path, const short *data, unsigned long n)
{
    FILE *f;
    unsigned long i, bytes = n * 2UL;

    f = fopen(path, "wb");
    if (!f) return 1;
    fwrite("RIFF", 1, 4, f);
    PutLe32(f, 36UL + bytes);
    fwrite("WAVEfmt ", 1, 8, f);
    PutLe32(f, 16UL);
    PutLe16(f, 1UL);
    PutLe16(f, 1UL);
    PutLe32(f, AUD_RATE);
    PutLe32(f, AUD_RATE * 2UL);
    PutLe16(f, 2UL);
    PutLe16(f, 16UL);
    fwrite("data", 1, 4, f);
    PutLe32(f, bytes);
    for (i = 0; i < n; i++)
        PutLe16(f, (unsigned long)(unsigned short)data[i]);
    return fclose(f) != 0;
}

static int WriteScene(const char *dir, unsigned scene)
{
    char path[PATH_LEN];
    RENDER *d = &result[scene][0];
    RENDER *c = &result[scene][1];
    unsigned long silence = SILENCE_MS * AUD_RATE / 1000UL;
    unsigned long k;
    short *ab;

    sprintf(path, "%s/%s_default.wav", dir, scene_names[scene]);
    if (WriteWav(path, d->data, d->n)) return 1;
    sprintf(path, "%s/%s_chamber.wav", dir, scene_names[scene]);
    if (WriteWav(path, c->data, c->n)) return 1;

    ab = (short *)malloc((d->n + silence + c->n) * sizeof(short));
    if (!ab) return 1;
    memcpy(ab, d->data, d->n * sizeof(short));
    memset(ab + d->n, 0, silence * sizeof(short));
    memcpy(ab + d->n + silence, c->data, c->n * sizeof(short));
    sprintf(path, "%s/%s_ab.wav", dir, scene_names[scene]);
    k = WriteWav(path, ab, d->n + silence + c->n);
    free(ab);
    if (k) return 1;

    diffs[scene] = 0UL;
    for (k = 0; k < d->n; k++)
        if (d->data[k] != c->data[k]) diffs[scene]++;
    return 0;
}

static int WriteReport(const char *dir)
{
    char path[PATH_LEN];
    FILE *f;
    unsigned s, b;

    sprintf(path, "%s/report.txt", dir);
    f = fopen(path, "w");
    if (!f) return 1;

    fprintf(f, "psgaudit report\n\n");
    fprintf(f, "Host-rendered approximation, NOT real Tandy 1000 output.\n\n");
    fprintf(f, "Engine: real synth, sound/MIDISYN.C and sound/INSTR.C\n");
    fprintf(f, "  (MSReset, MSBank, MSMessage, MSRender). Banks: default =\n");
    fprintf(f, "  MS_BANK_DEFAULT (0), chamber = MS_BANK_CHAMBER (1).\n");
    fprintf(f, "  Only MSBank differs between the takes of a scene.\n\n");
    fprintf(f, "Output stage (host model of an SN76496, not hardware):\n");
    fprintf(f, "  3 square tones at 3579545/(32*period) Hz, 2 dB steps\n");
    fprintf(f, "  attenuation (15 = silent), 15-bit LFSR noise.\n");
    fprintf(f, "  Noise setting:\n");
    fprintf(f, "  bit 2 = white (1) or periodic (0), bits 0-1 = rate\n");
    fprintf(f, "  div 16/32/64; setting 3 is treated as div 64 (unused).\n");
    fprintf(f, "  Fixed gains, identical for both banks: tone %.2f, noise\n",
            TONE_GAIN);
    fprintf(f, "  %.2f. No normalization; saturating clamp to int16 only.\n",
            NOISE_GAIN);
    fprintf(f, "  Vibrato (presets 3-5) and percussion are bank-independent\n");
    fprintf(f, "  in the synth: the banks differ only in melodic "
            "envelopes.\n\n");
    fprintf(f, "Format: mono 16-bit PCM, 44100 Hz, 44-byte header.\n");
    fprintf(f, "Tail after last event: %lu ms. ab silence: %lu ms.\n\n",
            TAIL_MS, SILENCE_MS);

    fprintf(f, "Scenes:\n");
    for (s = 0; s < SCENE_COUNT; s++)
        fprintf(f, "  %s: %s\n", scene_names[s], scene_desc[s]);
    fprintf(f, "\n");

    fprintf(f, "Event lists (identical for both banks):\n");
    for (s = 0; s < SCENE_COUNT; s++) {
        SCENE_INFO *si = &infos[s];

        fprintf(f, "  %-8s noteOn %lu noteOff %lu perc %lu cc %lu "
                "bend %lu prog %lu maxMelodic %lu checksum %08lX\n",
                scene_names[s], si->noteOns, si->noteOffs, si->percussion,
                si->controls, si->bends, si->programs, si->maxMelodic,
                si->checksum);
    }
    fprintf(f, "\n");

    fprintf(f, "Results (int16 units; full scale 32767):\n");
    fprintf(f, "%-8s %-8s %8s %6s %8s %10s %8s %8s %6s\n",
            "scene", "bank", "samples", "peak", "rms", "preclamp",
            "clipped", "rejected", "steal");
    for (s = 0; s < SCENE_COUNT; s++) {
        for (b = 0; b < BANK_COUNT; b++) {
            RENDER *r = &result[s][b];

            fprintf(f, "%-8s %-8s %8lu %6lu %8.1f %10.1f %8lu %8lu %6lu\n",
                    scene_names[s], bank_names[b], r->n, r->peak,
                    SqrtD(r->sumSq / (double)r->n),
                    r->rawPeak * 32767.0, r->clipped, r->rejected,
                    r->steals);
        }
    }
    fprintf(f, "\n");
    fprintf(f, "Samples that differ, default vs chamber:\n");
    for (s = 0; s < SCENE_COUNT; s++)
        fprintf(f, "  %-8s %lu\n", scene_names[s], diffs[s]);
    fprintf(f, "\nFiles (in this directory):\n");
    for (s = 0; s < SCENE_COUNT; s++)
        fprintf(f, "  %s_default.wav %s_chamber.wav %s_ab.wav\n",
                scene_names[s], scene_names[s], scene_names[s]);
    fprintf(f, "  report.txt\n");
    return fclose(f) != 0;
}

int main(int argc, char **argv)
{
    unsigned scene, bank;

    if (argc != 2) {
        fprintf(stderr, "usage: psgaudit <outdir>\n");
        return 2;
    }
    for (scene = 0; scene < SCENE_COUNT; scene++) {
        unsigned long n, lastMs;

        if (BuildScene(scene)) {
            fprintf(stderr, "scene %s: bad event list\n", scene_names[scene]);
            return 1;
        }
        if (scene == SC_PHRASE && infos[scene].maxMelodic > MAX_MELODIC) {
            fprintf(stderr, "phrase: more than %d melodic notes at once\n",
                    MAX_MELODIC);
            return 1;
        }
        lastMs = list.ev[list.count - 1].ms;
        n = ((lastMs + TAIL_MS) * AUD_RATE) / 1000UL;
        for (bank = 0; bank < BANK_COUNT; bank++) {
            RENDER *r = &result[scene][bank];

            if (PrepareRender(r, n) || RenderBank(bank, r)) {
                fprintf(stderr, "scene %s: render failed\n",
                        scene_names[scene]);
                return 1;
            }
            if (r->rejected) {
                fprintf(stderr, "scene %s: %s bank rejected %lu events\n",
                        scene_names[scene], bank_names[bank], r->rejected);
                return 1;
            }
            if (scene == SC_PHRASE && r->steals) {
                fprintf(stderr, "phrase: %s bank stole %lu voices\n",
                        bank_names[bank], r->steals);
                return 1;
            }
        }
        if (WriteScene(argv[1], scene)) {
            fprintf(stderr, "scene %s: write failed\n", scene_names[scene]);
            return 1;
        }
        for (bank = 0; bank < BANK_COUNT; bank++) {
            free(result[scene][bank].data);
            result[scene][bank].data = NULL;
        }
    }
    if (WriteReport(argv[1])) {
        fprintf(stderr, "report write failed\n");
        return 1;
    }
    printf("wrote audition set to %s\n", argv[1]);
    return 0;
}
