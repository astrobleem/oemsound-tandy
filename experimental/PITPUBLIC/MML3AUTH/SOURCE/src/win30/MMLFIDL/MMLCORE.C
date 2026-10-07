#include "MMLCORE.H"
#include <string.h>

static int white(unsigned c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
           c == '\v' || c == '\f';
}

static unsigned upper(unsigned c)
{
    if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
    return c;
}

static int fail(const MML_SCORE *s, MML_ERROR *e, unsigned code,
                unsigned at, const char *message)
{
    unsigned p, line, column, c;
    if (e != 0) {
        line = 1; column = 1;
        if (s != 0 && s->source != 0) {
            if (at > s->length) at = s->length;
            for (p = 0; p < at; ++p) {
                c = (unsigned char)s->source[p];
                if (c == '\r') {
                    ++line; column = 1;
                    if (p + 1 < at && s->source[p + 1] == '\n') ++p;
                } else if (c == '\n') {
                    ++line; column = 1;
                } else ++column;
            }
        }
        e->code = code; e->offset = at;
        e->line = line; e->column = column; e->message = message;
    }
    return 0;
}

static int iter_fail(MML_ITER *it, MML_ERROR *e, unsigned code,
                     unsigned at, const char *message)
{
    it->failed = 1;
    fail(it->score, e, code, at, message);
    return -1;
}

const char *mml_preset_name(unsigned program)
{
    static const char *names[8] = {
        "KEYS", "ORGAN", "BASS", "PAD", "REED", "LEAD", "BELL", "HIT"
    };
    if (program > 112 || (program & 15) != 0) return 0;
    return names[program / 16];
}

static void initialize(MML_ITER *it, const MML_SCORE *s, unsigned part)
{
    memset(it, 0, sizeof(*it));
    it->score = s; it->part = part;
    it->pos = s->start[part]; it->stop = s->stop[part];
    it->octave = 4; it->denominator = 4;
    it->volume = part == MML_PIT_PART ? 1 :
        part == MML_NOISE_PART ? 9 : 12;
    it->program = part < MML_TONE_PARTS ? s->initial_program[part] : 0;
    it->tempo_us = 500000UL;
    it->token_limit = MML_TOKEN_MAX;
    it->initial_pending = part < MML_TONE_PARTS;
}

void mml_reset(MML_ITER *it, const MML_SCORE *s, unsigned part)
{
    if (it == 0) return;
    if (s == 0 || !s->validated || part >= s->part_count) {
        memset(it, 0, sizeof(*it));
        it->score = s; it->failed = 1;
        return;
    }
    initialize(it, s, part);
}

static void skip(MML_ITER *it)
{
    const char *s;
    unsigned c;
    s = it->score->source;
    while (it->pos < it->stop) {
        c = (unsigned char)s[it->pos];
        if (white(c)) ++it->pos;
        else if (c == ';') {
            while (it->pos < it->stop && s[it->pos] != '\r' &&
                   s[it->pos] != '\n') ++it->pos;
        } else break;
    }
}

/* Returns 1 on a present valid integer, 0 when absent, -1 on overflow.
 * Decimal digits are contiguous. Leading whitespace/comments are allowed.
 */
static int number(MML_ITER *it, unsigned *value)
{
    unsigned n, c, any;
    const char *s;
    skip(it); s = it->score->source; n = 0; any = 0;
    while (it->pos < it->stop) {
        c = (unsigned char)s[it->pos];
        if (c < '0' || c > '9') break;
        c -= '0';
        if (n > (65535U - c) / 10U) return -1;
        n = n * 10U + c; ++it->pos; any = 1;
    }
    *value = n;
    return any ? 1 : 0;
}

static int length_valid(unsigned n)
{
    return n == 1 || n == 2 || n == 4 || n == 8 || n == 16 ||
           n == 32 || n == 64;
}

static unsigned long rounded_time(const MML_ITER *it)
{
    return it->whole_ms + (it->remainder >= 48000UL ? 1UL : 0UL);
}

static void event_set(MML_ITER *it, MML_EVENT *e, unsigned kind,
                      unsigned at, unsigned pitch, unsigned velocity,
                      unsigned program)
{
    e->when_ms = rounded_time(it); e->kind = kind; e->part = it->part;
    e->pitch = pitch; e->velocity = velocity; e->program = program;
    e->offset = at;
}

static int advance(MML_ITER *it, unsigned ticks, unsigned at, MML_ERROR *e)
{
    unsigned long sum;
    /* Maximum sum 576*1,500,000+95,999 = 864,095,999, fits 32 bits. */
    sum = (unsigned long)ticks * it->tempo_us + it->remainder;
    it->whole_ms += sum / 96000UL;
    it->remainder = sum % 96000UL;
    if (it->whole_ms > MML_DURATION_MAX ||
        (it->whole_ms == MML_DURATION_MAX && it->remainder != 0))
        return iter_fail(it, e, MML_E_DURATION, at,
                         "Part exceeds 600 seconds");
    return 1;
}

int mml_next(MML_ITER *it, MML_EVENT *e, MML_ERROR *error)
{
    unsigned at, c, n, denom, ticks, dotted, pitch, velocity, i, a;
    int nr, semitone, accidental;
    const char *s, *name;
    MML_REPEAT *r;
    if (it == 0 || e == 0) {
        fail(0, error, MML_E_STATE, 0, "Invalid iterator or event");
        return -1;
    }
    if (it->failed || it->score == 0) {
        fail(it->score, error, MML_E_STATE, 0,
             "Iterator requires a validated score");
        return -1;
    }
    if (it->ended) return 0;
    if (it->initial_pending) {
        it->initial_pending = 0;
        event_set(it, e, MML_PROGRAM, it->pos, 0, 0, it->program);
        return 1;
    }
    if (it->off_pending) {
        it->off_pending = 0;
        event_set(it, e, MML_NOTEOFF, it->off_offset, it->off_pitch,
                  it->off_velocity, it->off_program);
        return 1;
    }
    s = it->score->source;
    for (;;) {
        skip(it);
        if (it->pos >= it->stop) {
            if (it->depth != 0)
                return iter_fail(it, error, MML_E_REPEAT,
                                 it->repeat[it->depth - 1].opening,
                                 "Unmatched repeat opening bracket");
            it->ended = 1;
            event_set(it, e, MML_END, it->stop, 0, 0, it->program);
            return 1;
        }
        at = it->pos; c = upper((unsigned char)s[it->pos++]);
        if (c == '[') {
            if (it->depth >= MML_REPEAT_DEPTH)
                return iter_fail(it, error, MML_E_REPEAT, at,
                                 "Repeat nesting exceeds two levels");
            r = &it->repeat[it->depth++];
            r->start = it->pos; r->opening = at;
            r->left = 0; r->initialized = 0;
            continue;
        }
        if (c == ']') {
            if (it->depth == 0)
                return iter_fail(it, error, MML_E_REPEAT, at,
                                 "Unmatched repeat closing bracket");
            nr = number(it, &n);
            if (nr != 1 || n < 2 || n > 8)
                return iter_fail(it, error, MML_E_REPEAT, at,
                                 "Repeat count must be 2 through 8");
            r = &it->repeat[it->depth - 1];
            if (!r->initialized) {
                r->left = n - 1; r->initialized = 1;
            }
            if (r->left != 0) {
                --r->left; it->pos = r->start;
            } else --it->depth;
            continue;
        }
        if (it->token_count >= it->token_limit)
            return iter_fail(it, error, MML_E_TOKENS, at,
                             "Expanded score exceeds 32768 tokens");
        ++it->token_count;
        if (it->part == MML_NOISE_PART && c != 'T' && c != 'L' &&
            c != 'V' && c != 'R' && c != 'N')
            return iter_fail(it, error, MML_E_SYNTAX, at,
                             "Noise permits only T, L, V, R, N hits and repeats");
        if (c == 'T' || c == 'O' || c == 'L' || c == 'V') {
            nr = number(it, &n);
            if (nr != 1)
                return iter_fail(it, error, MML_E_NUMBER, at,
                                 "Command needs a valid decimal number");
            if (c == 'T') {
                if (n < 40 || n > 240)
                    return iter_fail(it, error, MML_E_NUMBER, at,
                                     "Tempo must be 40 through 240");
                it->tempo_us = (60000000UL + (unsigned long)(n / 2)) /
                               (unsigned long)n;
            } else if (c == 'O') {
                if (n < 2 || n > 7)
                    return iter_fail(it, error, MML_E_NUMBER, at,
                                     "Octave must be 2 through 7");
                it->octave = n;
            } else if (c == 'L') {
                if (!length_valid(n))
                    return iter_fail(it, error, MML_E_NUMBER, at,
                                     "Length must be 1,2,4,8,16,32 or 64");
                it->denominator = n;
            } else {
                if (n > (it->part == MML_PIT_PART ? 1U : 15U))
                    return iter_fail(it, error, MML_E_NUMBER, at,
                                     it->part == MML_PIT_PART ?
                                     "PIT volume must be 0 or 1" :
                                     "Volume must be 0 through 15");
                it->volume = n;
            }
            continue;
        }
        if (c == '<' || c == '>') {
            if ((c == '<' && it->octave <= 2) ||
                (c == '>' && it->octave >= 7))
                return iter_fail(it, error, MML_E_NUMBER, at,
                                 "Octave shift is outside 2 through 7");
            if (c == '<') --it->octave; else ++it->octave;
            continue;
        }
        if (c == '@') {
            if (it->part == MML_PIT_PART)
                return iter_fail(it, error, MML_E_PRESET, at,
                                 "PIT has no presets");
            skip(it); a = it->pos;
            /* Fixed preset names are complete tokens, so @KEYSO4C is legal.
             * No whitespace is required between MML commands.
             */
            for (i = 0; i < 8; ++i) {
                name = mml_preset_name(i * 16);
                for (n = 0; name[n] != 0 && a + n < it->stop; ++n)
                    if (upper((unsigned char)s[a + n]) !=
                        (unsigned char)name[n]) break;
                if (name[n] == 0) break;
            }
            if (i >= 8)
                return iter_fail(it, error, MML_E_PRESET, at,
                                 "Unknown preset; use KEYS/ORGAN/BASS/PAD/REED/LEAD/BELL/HIT");
            it->pos = a + n;
            it->program = i * 16;
            event_set(it, e, MML_PROGRAM, at, 0, 0, it->program);
            return 1;
        }
        semitone = -1;
        if (c == 'C') semitone = 0;
        else if (c == 'D') semitone = 2;
        else if (c == 'E') semitone = 4;
        else if (c == 'F') semitone = 5;
        else if (c == 'G') semitone = 7;
        else if (c == 'A') semitone = 9;
        else if (c == 'B') semitone = 11;
        if (semitone < 0 && c != 'R' &&
            !(c == 'N' && it->part == MML_NOISE_PART))
            return iter_fail(it, error, MML_E_SYNTAX, at,
                             "Unknown or unsupported MML token");
        accidental = 0;
        denom = it->denominator;
        pitch = 0;
        if (c == 'N') {
            nr = number(it, &n);
            if (nr != 1 || n < 35 || n > 81)
                return iter_fail(it, error, MML_E_NUMBER, at,
                                 "Noise drum code must be 35 through 81");
            pitch = n;
            skip(it);
            if (it->pos < it->stop && s[it->pos] == '/') {
                ++it->pos;
                nr = number(it, &n);
                if (nr != 1 || !length_valid(n))
                    return iter_fail(it, error, MML_E_NUMBER, at,
                                     "Noise hit length must be 1,2,4,8,16,32 or 64");
                denom = n;
            }
        } else {
            skip(it);
            if (c != 'R' && it->pos < it->stop) {
                n = (unsigned char)s[it->pos];
                if (n == '#' || n == '+') { accidental = 1; ++it->pos; }
                else if (n == '-') { accidental = -1; ++it->pos; }
            }
            nr = number(it, &n);
            if (nr < 0 || (nr == 1 && !length_valid(n)))
                return iter_fail(it, error, MML_E_NUMBER, at,
                                 "Note/rest length must be 1,2,4,8,16,32 or 64");
            if (nr == 1) denom = n;
        }
        dotted = 0;
        skip(it);
        if (it->pos < it->stop && s[it->pos] == '.') {
            dotted = 1; ++it->pos;
        }
        ticks = 384U / denom;
        if (dotted) ticks = ticks * 3U / 2U;
        if (c != 'R' && c != 'N') {
            semitone += (int)((it->octave + 1) * 12) + accidental;
            if (semitone < 45 || semitone > 96)
                return iter_fail(it, error, MML_E_PITCH, at,
                                 "Note pitch must be MIDI 45 through 96");
            pitch = (unsigned)semitone;
        }
        if (it->note_count >= MML_NOTES_MAX)
            return iter_fail(it, error, MML_E_NOTES, at,
                             "Part exceeds 4096 notes/rests");
        ++it->note_count;
        if (c == 'R' || it->volume == 0) {
            if (advance(it, ticks, at, error) < 0) return -1;
            continue;
        }
        velocity = 1 + 9 * (it->volume - 1);
        event_set(it, e, MML_NOTEON, at, pitch, velocity, it->program);
        if (advance(it, ticks, at, error) < 0) return -1;
        it->off_pending = 1;
        it->off_pitch = pitch; it->off_velocity = velocity;
        it->off_program = it->program; it->off_offset = at;
        return 1;
    }
}

int mml_validate(MML_SCORE *score, const char *source, unsigned length,
                 const unsigned char initial_program[MML_TONE_PARTS],
                 MML_ERROR *error)
{
    unsigned p, line_start, end, next, a, b, c, part, active, found, content;
    unsigned long used;
    MML_ITER it;
    MML_EVENT event;
    int result;
    if (error != 0) {
        memset(error, 0, sizeof(*error)); error->line = 1; error->column = 1;
    }
    if (score == 0)
        return fail(0, error, MML_E_STATE, 0, "Missing score object");
    memset(score, 0, sizeof(*score));
    score->source = source; score->length = length;
    score->version = 1; score->part_count = MML_TONE_PARTS;
    if (source == 0)
        return fail(score, error, MML_E_SOURCE, 0, "Missing source text");
    if (length > MML_SOURCE_MAX)
        return fail(score, error, MML_E_SOURCE, MML_SOURCE_MAX,
                    "Source exceeds 8192 bytes");
    for (part = 0; part < MML_TONE_PARTS; ++part) {
        c = initial_program != 0 ? initial_program[part] : 0;
        if (mml_preset_name(c) == 0)
            return fail(score, error, MML_E_PRESET, 0, "Invalid initial UI preset");
        score->initial_program[part] = (unsigned char)c;
    }
    for (p = 0; p < length; ++p) {
        c = (unsigned char)source[p];
        if (c > 126 || (c < 32 && !white(c)))
            return fail(score, error, MML_E_SOURCE, p,
                        "Source must be ASCII text; UTF-8 BOM is not supported");
    }
    /* Establish section ranges without expanding or allocating tokens.
     * Headers must stand alone on otherwise blank/commented lines.
     * MML2 is recognized only before any nonblank, noncomment content.
     */
    p = 0; active = MML_PARTS; found = 0; content = 0;
    while (p < length) {
        line_start = p; end = p;
        while (end < length && source[end] != '\r' && source[end] != '\n') ++end;
        next = end;
        if (next < length) {
            c = (unsigned char)source[next++];
            if (c == '\r' && next < length && source[next] == '\n') ++next;
        }
        a = line_start;
        while (a < end && white((unsigned char)source[a])) ++a;
        b = a;
        while (b < end && source[b] != ';') ++b;
        while (b > a && white((unsigned char)source[b - 1])) --b;
        if (b == a + 4 && upper((unsigned char)source[a]) == 'M' &&
            upper((unsigned char)source[a + 1]) == 'M' &&
            upper((unsigned char)source[a + 2]) == 'L' && (source[a + 3] == '2' || source[a + 3] == '3')) {
            if (content)
                return fail(score, error, MML_E_SECTION, a,
                            "MML2 must be the first nonblank, noncomment line");
            score->version = source[a + 3] - '0';
            score->part_count = score->version == 3 ?
                MML_PARTS : MML_NOISE_PART + 1;
            content = 1; p = next;
            continue;
        }
        if (a < b) content = 1;
        if (b == a + 3 && source[a] == '[' && source[a + 2] == ']' &&
            ((upper((unsigned char)source[a + 1]) >= 'A' &&
              upper((unsigned char)source[a + 1]) <= 'C') ||
             (score->version >= 2 && upper((unsigned char)source[a + 1]) == 'N') ||
             (score->version == 3 && upper((unsigned char)source[a + 1]) == 'P'))) {
            c = upper((unsigned char)source[a + 1]);
            part = c == 'P' ? MML_PIT_PART :
                c == 'N' ? MML_NOISE_PART : c - 'A';
            if (score->present[part])
                return fail(score, error, MML_E_SECTION, a, "Duplicate part section");
            if (active < MML_PARTS) score->stop[active] = line_start;
            active = part; score->present[part] = 1;
            score->start[part] = next; score->stop[part] = length; found = 1;
        } else if (a < b && active == MML_PARTS)
            return fail(score, error, MML_E_SECTION, a,
                        score->version == 2 ?
                        "Content before first standalone [A], [B], [C] or [N] section" :
                        "Content before first standalone [A], [B] or [C] section");
        p = next;
    }
    if (!found)
        return fail(score, error, MML_E_SECTION, 0,
                    score->version == 2 ?
                    "Score needs a standalone [A], [B], [C] or [N] section" :
                    "Score needs a standalone [A], [B] or [C] section");
    used = 0;
    for (part = 0; part < score->part_count; ++part) {
        initialize(&it, score, part);
        it.token_limit = MML_TOKEN_MAX - used;
        do { result = mml_next(&it, &event, error); } while (result > 0);
        if (result < 0) return 0;
        used += it.token_count;
        score->note_count[part] = it.note_count;
        score->part_duration_ms[part] = rounded_time(&it);
        if (score->part_duration_ms[part] > score->duration_ms)
            score->duration_ms = score->part_duration_ms[part];
    }
    if (score->version == 3 && !score->duration_ms)
        return fail(score, error, MML_E_DURATION, 0,
                    "MML3 requires advancing notes or rests");
    score->token_count = used; score->validated = 1;
    return 1;
}
