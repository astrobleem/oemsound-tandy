/* Bounded SMF parser and integer-only tempo clock. See README.TXT.
 * ANSI C89; no heap, floating point, 64-bit integers or unbounded recursion.
 */
#include "SMF.H"

typedef char SMF_ULONG_MUST_BE_32_BITS[(sizeof(SMF_ULONG) == 4) ? 1 : -1];
typedef char SMF_BYTE_MUST_BE_8_BITS[(sizeof(unsigned char) == 1) ? 1 : -1];

#define U32_MAX ((SMF_ULONG)0xffffffffUL)
#define EV_CHANNEL 1
#define EV_TEMPO   2
#define EV_OTHER   3
#define EV_END     4

typedef struct SMF_TRACK {
    unsigned start;
    unsigned end;
    unsigned pos;
    SMF_ULONG tick;
    SMF_ULONG tempo;
    unsigned char running;
    unsigned char kind;
    unsigned char status;
    unsigned char d1;
    unsigned char d2;
    unsigned char length;
    unsigned char done;
} SMF_TRACK;

static const unsigned char SMF_FAR *g_file;
static unsigned g_size;
static unsigned g_tracks;
static unsigned g_division;
static unsigned g_parsed;
static unsigned g_channels;
static unsigned g_file_channels;
static int g_open;
static int g_failed;
static SMF_ULONG g_tick;
static SMF_ULONG g_us;
static SMF_ULONG g_remainder;
static SMF_ULONG g_tempo;
static SMF_ULONG g_duration;
static const char *g_error = "No file open";
static SMF_TRACK g_track[SMF_MAX_TRACKS];

static int fail(const char *message)
{
    g_error = message;
    g_failed = 1;
    return 0;
}

static unsigned read16(unsigned p)
{
    return ((unsigned)g_file[p] << 8) | (unsigned)g_file[p + 1U];
}

static SMF_ULONG read32(unsigned p)
{
    return ((SMF_ULONG)g_file[p] << 24) |
           ((SMF_ULONG)g_file[p + 1U] << 16) |
           ((SMF_ULONG)g_file[p + 2U] << 8) |
           (SMF_ULONG)g_file[p + 3U];
}

static int tag(unsigned p, unsigned char a, unsigned char b,
               unsigned char c, unsigned char d)
{
    return g_file[p] == a && g_file[p + 1U] == b &&
           g_file[p + 2U] == c && g_file[p + 3U] == d;
}

static int get_byte(SMF_TRACK *t, unsigned char *value)
{
    if (t->pos >= t->end)
        return fail("Truncated track event");
    *value = g_file[t->pos++];
    return 1;
}

static int get_vlq(SMF_TRACK *t, SMF_ULONG *value)
{
    unsigned i;
    unsigned char b;
    SMF_ULONG n;

    n = 0;
    for (i = 0; i < 4U; ++i) {
        if (!get_byte(t, &b))
            return 0;
        n = (n << 7) | (SMF_ULONG)(b & 127U);
        if ((b & 128U) == 0) {
            *value = n;
            return 1;
        }
    }
    return fail("VLQ exceeds four bytes");
}

static int data_byte(SMF_TRACK *t, unsigned char *value)
{
    if (!get_byte(t, value))
        return 0;
    if ((*value & 128U) != 0)
        return fail("Status byte where MIDI data was required");
    return 1;
}

/* Parse exactly one event into a track's single pending-event slot. */
static int parse_event(SMF_TRACK *t)
{
    SMF_ULONG delta;
    SMF_ULONG length;
    unsigned char b;
    unsigned char type;
    unsigned char high;

    if (g_parsed >= SMF_MAX_EVENTS)
        return fail("File exceeds event budget");
    ++g_parsed;
    if (t->pos >= t->end)
        return fail("Track lacks final end-of-track event");
    if (!get_vlq(t, &delta))
        return 0;
    if (delta > U32_MAX - t->tick)
        return fail("Absolute tick overflow");
    t->tick += delta;
    if (!get_byte(t, &b))
        return 0;
    t->kind = EV_OTHER;
    t->length = 0;
    t->d1 = 0;
    t->d2 = 0;
    if (b < 128U) {
        if (t->running == 0)
            return fail("Running status without channel status");
        t->status = t->running;
        t->d1 = b;
    } else if (b < 240U) {
        t->status = b;
        t->running = b;
        if (!data_byte(t, &t->d1))
            return 0;
    } else {
        t->running = 0;
        if (b == 255U) {
            if (!get_byte(t, &type))
                return 0;
            if (type >= 128U)
                return fail("Invalid meta-event type");
            if (!get_vlq(t, &length))
                return 0;
            if (length > (SMF_ULONG)(t->end - t->pos))
                return fail("Meta event exceeds track boundary");
            if (type == 47U) {
                if (length != 0)
                    return fail("End-of-track length is not zero");
                if (t->pos != t->end)
                    return fail("Bytes follow end-of-track event");
                t->kind = EV_END;
            } else if (type == 81U) {
                if (length != 3)
                    return fail("Tempo event length is not three");
                t->tempo = ((SMF_ULONG)g_file[t->pos] << 16) |
                           ((SMF_ULONG)g_file[t->pos + 1U] << 8) |
                           (SMF_ULONG)g_file[t->pos + 2U];
                if (t->tempo == 0)
                    return fail("Tempo must be nonzero");
                t->kind = EV_TEMPO;
            }
            t->pos += (unsigned)length;
            return 1;
        }
        if (b == 240U || b == 247U) {
            if (!get_vlq(t, &length))
                return 0;
            if (length > (SMF_ULONG)(t->end - t->pos))
                return fail("SysEx event exceeds track boundary");
            t->pos += (unsigned)length;
            return 1;
        }
        return fail("Unsupported system status in MIDI file");
    }
    high = (unsigned char)(t->status & 240U);
    t->length = 2;
    if (high != 192U && high != 208U) {
        if (!data_byte(t, &t->d2))
            return 0;
        t->length = 3;
    }
    t->kind = EV_CHANNEL;
    return 1;
}

/* Add delta * tempo / PPQN microseconds, retaining the exact fractional
 * numerator modulo PPQN across ALL tempo changes.  Let delta = a*d + b
 * and tempo = q*d + r. Then whole us = a*tempo + b*q + (b*r+carry)/d.
 * b*r+carry < 32767^2; only a*tempo needs a pre-multiply budget guard.
 * The one-hour bound is <2^32, so no wide multiply/divide is necessary.
 */
static int advance_clock(SMF_ULONG tick)
{
    SMF_ULONG delta;
    SMF_ULONG a;
    SMF_ULONG b;
    SMF_ULONG q;
    SMF_ULONG r;
    SMF_ULONG add;
    SMF_ULONG fraction;
    SMF_ULONG available;

    /* Chords, controller groups and cross-track ties need no division. */
    if (tick == g_tick)
        return 1;
    delta = tick - g_tick;
    available = SMF_MAX_US - g_us;
    a = delta / (SMF_ULONG)g_division;
    b = delta % (SMF_ULONG)g_division;
    if (a > available / g_tempo)
        return fail("File exceeds one-hour duration budget");
    add = a * g_tempo;
    available -= add;
    q = g_tempo / (SMF_ULONG)g_division;
    r = g_tempo % (SMF_ULONG)g_division;
    fraction = b * r + g_remainder;
    b = b * q + fraction / (SMF_ULONG)g_division;
    if (b > available)
        return fail("File exceeds one-hour duration budget");
    g_us += add + b;
    g_remainder = fraction % (SMF_ULONG)g_division;
    if (g_us == SMF_MAX_US && g_remainder != 0)
        return fail("File exceeds one-hour duration budget");
    g_tick = tick;
    return 1;
}

int smf_next(SMF_EVENT *event)
{
    unsigned i;
    unsigned best;
    SMF_TRACK *t;
    SMF_EVENT result;

    if (!g_open || g_failed) {
        if (!g_failed)
            fail("No file open");
        return -1;
    }
    if (event == 0) {
        fail("Null event output pointer");
        return -1;
    }
    for (;;) {
        best = g_tracks;
        for (i = 0; i < g_tracks; ++i) {
            if (!g_track[i].done &&
                (best == g_tracks || g_track[i].tick < g_track[best].tick))
                best = i;
        }
        if (best == g_tracks)
            return 0;
        t = &g_track[best];
        if (!advance_clock(t->tick))
            return -1;
        if (t->kind == EV_END) {
            t->done = 1;
            continue;
        }
        if (t->kind == EV_TEMPO)
            g_tempo = t->tempo;
        if (t->kind == EV_CHANNEL) {
            result.ms = g_us / (SMF_ULONG)1000U;
            result.tick = g_tick;
            result.status = t->status;
            result.data1 = t->d1;
            result.data2 = t->d2;
            result.length = t->length;
            result.track = (unsigned char)best;
            ++g_channels;
            if (!parse_event(t))
                return -1;
            *event = result;
            return 1;
        }
        if (!parse_event(t))
            return -1;
    }
}

int smf_rewind(void)
{
    unsigned i;
    SMF_TRACK *t;

    if (!g_open)
        return fail("No file open");
    g_failed = 0;
    g_error = "No error";
    g_tick = 0;
    g_us = 0;
    g_remainder = 0;
    g_tempo = (SMF_ULONG)500000UL;
    g_parsed = 0;
    g_channels = 0;
    for (i = 0; i < g_tracks; ++i) {
        t = &g_track[i];
        t->pos = t->start;
        t->tick = 0;
        t->running = 0;
        t->done = 0;
        if (!parse_event(t))
            return 0;
    }
    return 1;
}

void smf_close(void)
{
    g_file = 0;
    g_size = 0;
    g_tracks = 0;
    g_division = 0;
    g_open = 0;
    g_failed = 0;
    g_duration = 0;
    g_file_channels = 0;
    g_error = "No file open";
}

int smf_open(const unsigned char SMF_FAR *file, unsigned size)
{
    unsigned format;
    unsigned declared;
    unsigned p;
    SMF_ULONG length;
    SMF_EVENT event;
    int status;

    smf_close();
    if (file == 0)
        return fail("Null MIDI file pointer");
    if (size > SMF_MAX_FILE)
        return fail("MIDI file exceeds 32768 bytes");
    if (size < 14U)
        return fail("Truncated MIDI header");
    g_file = file;
    g_size = size;
    if (!tag(0, 'M', 'T', 'h', 'd'))
        return fail("Missing MThd header");
    length = read32(4);
    if (length < 6 || length > (SMF_ULONG)(g_size - 8U))
        return fail("Invalid MIDI header length");
    format = read16(8);
    declared = read16(10);
    g_division = read16(12);
    if (format > 1U)
        return fail("Only format 0 and format 1 are supported");
    if (declared == 0 || declared > SMF_MAX_TRACKS ||
        (format == 0 && declared != 1U))
        return fail("Invalid or unsupported track count");
    if (g_division == 0 || (g_division & 32768U) != 0)
        return fail("Only nonzero PPQN timing is supported");
    p = (unsigned)length + 8U;
    while (p < g_size) {
        if (g_size - p < 8U)
            return fail("Truncated MIDI chunk header");
        length = read32(p + 4U);
        if (length > (SMF_ULONG)(g_size - p - 8U))
            return fail("MIDI chunk exceeds file boundary");
        if (tag(p, 'M', 'T', 'r', 'k')) {
            if (g_tracks >= declared)
                return fail("More tracks than declared in header");
            g_track[g_tracks].start = p + 8U;
            g_track[g_tracks].end = p + 8U + (unsigned)length;
            ++g_tracks;
        }
        /* Unknown bounded chunks are skipped as required by SMF readers. */
        p += 8U + (unsigned)length;
    }
    if (g_tracks != declared)
        return fail("Fewer tracks than declared in header");
    g_open = 1;
    if (!smf_rewind()) {
        g_open = 0;
        return 0;
    }
    do {
        status = smf_next(&event);
    } while (status > 0);
    if (status < 0) {
        g_open = 0;
        return 0;
    }
    g_duration = g_us / (SMF_ULONG)1000U;
    g_file_channels = g_channels;
    if (!smf_rewind()) {
        g_open = 0;
        return 0;
    }
    return 1;
}

const char *smf_error(void) { return g_error; }
SMF_ULONG smf_duration_ms(void) { return g_duration; }
unsigned smf_event_count(void) { return g_file_channels; }
unsigned smf_track_count(void) { return g_tracks; }
unsigned smf_division(void) { return g_division; }
