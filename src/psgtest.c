/* Minimal Tandy PSG test: plays a quick triad on channels 0..2.
 * Toolchain: Microsoft C 6.0 (C600)
 * Build:  cl /O /AS /Fepsgtest.exe src\psgtest.c
 */
#include "dos.h"
#include "conio.h"

/* Tandy PSG port */
#define SN_PORT 0xC0

/* BIOS data area tick counter: 0x40:0x6C (DWORD), ~18.2065 Hz (~55 ms/tick) */
#define BDA_SEG 0x40
#define BDA_TICKS_OFF 0x6C

/* --- PSG I/O --- */
static void sn_out(unsigned char b) { outp(SN_PORT, b); }

/* period: 10-bit (1..1023); channel: 0..2 tones (3=noise) */
static void sn_set_tone(int channel, unsigned period)
{
    unsigned char latch = 0x80 | ((channel & 3) << 5) | (period & 0x0F); /* 1 cc 0 dddd */
    unsigned char data  = (unsigned char)((period >> 4) & 0x3F);         /* 0 0 dddddd */
    sn_out(latch);
    sn_out(data);
}

/* vol4: 0 = loudest .. 15 = silent */
static void sn_set_volume(int channel, unsigned vol4)
{
    sn_out(0x90 | ((channel & 3) << 5) | (vol4 & 0x0F)); /* 1 cc 1 vvvv */
}

/* Optional: set noise (simple white noise burst) */
static void sn_set_noise(unsigned mode)
{
    /* mode: 0..3 -> shift rate, bit3=1 selects noise; 1 cc 1 dddd with cc=3 */
    sn_out(0xE0 | (mode & 0x0F));
}

/* --- Timing using BIOS tick counter --- */
static unsigned long get_ticks(void)
{
    volatile unsigned long far *ticks = (unsigned long far*)MK_FP(BDA_SEG, BDA_TICKS_OFF);
    return *ticks;
}

static void wait_ms(unsigned ms)
{
    /* Convert ms to ~55ms ticks with rounding: ticks = (ms + 27) / 55 */
    unsigned long start = get_ticks();
    unsigned long delta = (ms + 27U) / 55U;
    if (delta == 0) delta = 1; /* at least one tick */
    while ((get_ticks() - start) < delta) { /* spin */ }
}

/* --- Frequency mapping ---
 * SN76496: fout = CLK / (32 * period)
 * For NTSC-like clock ~3.579545 MHz (common on Tandy 1000 PSG path).
 */
static unsigned freq_to_period(unsigned freq_hz)
{
    const unsigned long CLK = 3579545UL; /* verify on your board if needed */
    if (freq_hz == 0) {
        /* 0 Hz indicates silence; return max period to avoid divide-by-zero */
        return 1023;
    }
    unsigned long n = (CLK / 32UL) / (unsigned long)freq_hz;
    if (n < 1UL) n = 1UL;
    if (n > 1023UL) n = 1023UL;
    return (unsigned)n;
}

/* Handy: set note on channel with amplitude (0..15, where 0 is loudest) */
static void note_play(int channel, unsigned freq_hz, unsigned vol4)
{
    sn_set_tone(channel, freq_to_period(freq_hz));
    sn_set_volume(channel, vol4);
}

static void all_silent(void)
{
    sn_set_volume(0, 15);
    sn_set_volume(1, 15);
    sn_set_volume(2, 15);
}

/* Quick audible demo: C major triad arpeggio + a tiny noise tick */
int main(void)
{
    /* Start silent */
    all_silent();

    /* Simple arpeggio on channels 0..2 (C4, E4, G4) */
    note_play(0, 262, 4);   /* C4 */
    wait_ms(200);

    note_play(1, 330, 4);   /* E4 */
    wait_ms(200);

    note_play(2, 392, 4);   /* G4 */
    wait_ms(400);

    /* Hold the triad briefly */
    wait_ms(300);

    /* Quick noise tick (optional) */
    sn_set_noise(0x05);     /* low-rate white noise */
    sn_set_volume(3, 6);    /* noise channel volume lives at "channel 3" */
    wait_ms(120);
    sn_set_volume(3, 15);   /* silence noise */

    /* Fade out */
    unsigned v;
    for (v = 4; v <= 15; ++v) {
        sn_set_volume(0, v);
        sn_set_volume(1, v);
        sn_set_volume(2, v);
        wait_ms(40);
    }

    all_silent();
    return 0;
}
