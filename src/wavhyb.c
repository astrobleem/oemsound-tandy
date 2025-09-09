/* wavhybrid.c — Hybrid WAV player for Tandy 1000 (built-in hardware only)
 *
 * Plays 8-bit unsigned mono PCM WAVs using both PC speaker (1-bit sigma-delta)
 * and PSG (volume modulation on a fixed high-pitch tone).
 *
 * Toolchain: Microsoft C 6.0
 * Build: cl /O /AS /Fewavhybrid.exe src\wavhybrid.c
 */

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hardware ports */
#define PIT_CTL   0x43
#define PIT_CH0   0x40
#define PIT_FREQ  1193182UL

#define PORT_SPKR 0x61
#define SN_PORT   0xC0

/* RIFF/WAV chunk IDs */
#define ID_RIFF   0x46464952UL /* "RIFF" */
#define ID_WAVE   0x45564157UL /* "WAVE" */
#define ID_FMT    0x20746D66UL /* "fmt " */
#define ID_DATA   0x61746164UL /* "data" */

/* --- Globals --- */
static volatile unsigned playing = 0;
static unsigned char far *samples = 0;
static unsigned long nsamples = 0;
static volatile unsigned long idx = 0;

static int sd_acc = 0;
static unsigned char spkr_state = 0;
static void (interrupt far *old_timer)(void) = 0;

/* --- PSG helpers --- */
static void sn_out(unsigned char b)
{
    outp(SN_PORT, b);
}

static void sn_set_tone(int chan, unsigned period)
{
    unsigned char latch;
    unsigned char data;

    latch = 0x80 | ((chan & 3) << 5) | (period & 0x0F);
    data  = (unsigned char)((period >> 4) & 0x3F);

    sn_out(latch);
    sn_out(data);
}

static void sn_set_volume(int chan, unsigned vol4)
{
    sn_out(0x90 | ((chan & 3) << 5) | (vol4 & 0x0F));
}

/* --- PIT helpers --- */
static void pit_set_rate(unsigned hz)
{
    unsigned div;

    div = (unsigned)(PIT_FREQ / (unsigned long)hz);
    if (div < 2) div = 2;

    outp(PIT_CTL, 0x36);
    outp(PIT_CH0, (unsigned char)(div & 0xFF));
    outp(PIT_CH0, (unsigned char)((div >> 8) & 0xFF));
}

/* --- WAV reader --- */
typedef struct {
    unsigned long id;
    unsigned long size;
    unsigned long fmt;
} RIFFHDR;

typedef struct {
    unsigned long id;
    unsigned long size;
} CHUNK;

static int load_wav_8m(const char *path,
                       unsigned *sr_out,
                       unsigned char far **data_out,
                       unsigned long *len_out)
{
    FILE *f;
    RIFFHDR h;
    CHUNK c;
    unsigned short fmt_tag = 0;
    unsigned short channels = 0;
    unsigned short bits = 0;
    unsigned short block_align;
    unsigned long byte_rate;
    unsigned long sr = 0;
    unsigned char *buf = 0;
    unsigned long datalen = 0;
    int got_fmt = 0;
    int got_dat = 0;

    f = fopen(path, "rb");
    if (!f) return -1;

    if (fread(&h, sizeof(h), 1, f) != 1) {
        fclose(f);
        return -2;
    }
    if (h.id != ID_RIFF || h.fmt != ID_WAVE) {
        fclose(f);
        return -3;
    }

    while (fread(&c, sizeof(c), 1, f) == 1) {
        if (c.id == ID_FMT) {
            if (c.size < 16) {
                fclose(f);
                return -4;
            }
            fread(&fmt_tag, 2, 1, f);
            fread(&channels, 2, 1, f);
            fread(&sr, 4, 1, f);
            fread(&byte_rate, 4, 1, f);
            fread(&block_align, 2, 1, f);
            fread(&bits, 2, 1, f);
            if (c.size > 16) {
                fseek(f, c.size - 16, SEEK_CUR);
            }
            got_fmt = 1;
        } else if (c.id == ID_DATA) {
            datalen = c.size;
            buf = (unsigned char *)malloc(datalen);
            if (!buf) {
                fclose(f);
                return -5;
            }
            if (fread(buf, 1, datalen, f) != datalen) {
                free(buf);
                fclose(f);
                return -6;
            }
            got_dat = 1;
        } else {
            fseek(f, c.size, SEEK_CUR);
        }
        if (got_fmt && got_dat) break;
    }
    fclose(f);

    if (!got_fmt || !got_dat) {
        if (buf) free(buf);
        return -7;
    }
    if (!(fmt_tag == 1 && channels == 1 && bits == 8)) {
        free(buf);
        return -8;
    }

    *sr_out   = (unsigned)sr;
    *data_out = (unsigned char far *)MK_FP(FP_SEG(buf), FP_OFF(buf));
    *len_out  = datalen;
    return 0;
}

/* --- IRQ0 handler --- */
void interrupt far new_timer(void)
{
    if (playing && idx < nsamples) {
        unsigned char s;

        s = samples[idx++];
        /* 1-bit sigma-delta to speaker */
        sd_acc += ((int)s - 128);
        if (sd_acc >= 0) {
            spkr_state |= 0x02;
            sd_acc -= 255;
        } else {
            spkr_state &= ~0x02;
        }
        spkr_state |= 0x01;
        outp(PORT_SPKR, spkr_state);

        /* PSG volume uses low nibble */
        sn_set_volume(0, 15 - (s & 0x0F));
    } else {
        playing = 0;
    }

    outp(0x20, 0x20);
    (*old_timer)();
}

/* --- Main --- */
int main(int argc, char **argv)
{
    unsigned sr;
    unsigned char far *data;
    unsigned long len;

    if (argc < 2) {
        puts("Usage: WAVHYBRID <file.wav>\n"
             "8-bit unsigned mono PCM only.");
        return 1;
    }
    if (load_wav_8m(argv[1], &sr, &data, &len) != 0) {
        puts("Invalid WAV. Must be 8-bit unsigned mono.");
        return 2;
    }

    if (sr < 6000)  sr = 6000;
    if (sr > 12000) sr = 12000;

    sn_set_tone(0, 1);
    sn_set_volume(0, 15);

    spkr_state = inp(PORT_SPKR);
    spkr_state |= 0x01;
    spkr_state &= ~0x02;
    outp(PORT_SPKR, spkr_state);

    samples  = data;
    nsamples = len;
    idx      = 0;
    sd_acc   = 0;
    playing  = 1;

    old_timer = _dos_getvect(0x08);
    _disable();
    _dos_setvect(0x08, new_timer);
    pit_set_rate(sr);
    _enable();

    while (playing) { /* spin */ }

    _disable();
    _dos_setvect(0x08, old_timer);
    _enable();

    sn_set_volume(0, 15);
    {
        unsigned char p = inp(PORT_SPKR);
        p &= ~0x03;
        outp(PORT_SPKR, p);
    }

    free((void *)MK_FP(FP_SEG(data), FP_OFF(data)));
    return 0;
}
