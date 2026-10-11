#ifndef MMLCORE_H
#define MMLCORE_H

/* MML1 revision 2, MML2 revision 1 and explicit MML3 PIT extension.
 * C89, Microsoft C 6.0 small-model friendly.
 * No heap, recursion, compiled event list, floating point or 64-bit arithmetic.
 * Keep source alive and unchanged from validation through all iteration.
 */
#define MML_TONE_PARTS 3
#define MML_NOISE_PART 3
#define MML_PIT_PART 4
#define MML_PARTS 5                  /* maximum, use score.part_count */
#define MML_SOURCE_MAX 8192U
#define MML_TOKEN_MAX 32768UL
#define MML_NOTES_MAX 4096U
#define MML_DURATION_MAX 600000UL
#define MML_REPEAT_DEPTH 2

#define MML_NOTEON 1
#define MML_NOTEOFF 2
#define MML_PROGRAM 3
#define MML_END 4

#define MML_E_NONE 0
#define MML_E_SOURCE 1
#define MML_E_SECTION 2
#define MML_E_SYNTAX 3
#define MML_E_NUMBER 4
#define MML_E_PITCH 5
#define MML_E_PRESET 6
#define MML_E_REPEAT 7
#define MML_E_TOKENS 8
#define MML_E_NOTES 9
#define MML_E_DURATION 10
#define MML_E_STATE 11

typedef struct {
    unsigned code;
    unsigned offset;                 /* zero-based source byte offset */
    unsigned line, column;           /* one-based; CRLF counts once */
    const char *message;
} MML_ERROR;

typedef struct {
    unsigned long when_ms;
    unsigned kind, part;
    unsigned pitch, velocity, program;
    unsigned offset;
} MML_EVENT;

typedef struct {
    const char *source;
    unsigned length;
    unsigned version, part_count;   /* MML1:1,3; MML2:2,4; MML3:3,5 */
    unsigned start[MML_PARTS], stop[MML_PARTS];
    unsigned char present[MML_PARTS];
    unsigned char initial_program[MML_TONE_PARTS];
    unsigned long part_duration_ms[MML_PARTS];
    unsigned long duration_ms, token_count;
    unsigned note_count[MML_PARTS];
    unsigned validated;
} MML_SCORE;

typedef struct {
    unsigned start, opening;
    unsigned left;                   /* zero until first matching close */
    unsigned initialized;
} MML_REPEAT;

typedef struct {
    const MML_SCORE *score;
    unsigned part, pos, stop, octave, denominator, volume, program;
    unsigned depth, initial_pending, off_pending, ended, failed;
    unsigned off_pitch, off_velocity, off_program, off_offset;
    unsigned note_count;
    unsigned long tempo_us, whole_ms, remainder;
    unsigned long token_count, token_limit;
    MML_REPEAT repeat[MML_REPEAT_DEPTH];
} MML_ITER;

/* initial_program contains exactly three tone presets, or may be NULL for
 * Keys (0) on every tone part. Noise never reads an initial preset.
 * Valid programs are
 * 0,16,32,48,64,80,96,112. Returns 1 on success, 0 on error. On success all
 * parts have been fully expanded/validated within shared bounds. Source
 * length is explicit; a terminating NUL is neither needed nor included.
 */
int mml_validate(MML_SCORE *score, const char *source, unsigned length,
                 const unsigned char initial_program[MML_TONE_PARTS],
                 MML_ERROR *error);

/* Each iterator is independent. Reset only a successfully validated score.
 * Each tone part starts with its UI/default initial PROGRAM at time zero,
 * including an absent/silent tone. Explicit @ commands follow in source
 * order. NOTEON includes attack program/velocity snapshots. Written NOTEOFF
 * is emitted before commands and attacks at the same part timestamp.
 * END is emitted once at the part's duration. Then mml_next returns zero.
 * Merge consumers must drain all NOTEOFFs before attacks at equal timestamps,
 * while retaining command/attack source order within each part.
 * MML2 part 3 is noise: pitch is drum code 35..81, program is always zero,
 * default volume is 9, and no PROGRAM event is ever emitted. Map this part
 * to the existing engine's MIDI channel 9. MML1 exposes only three parts.
 * MML3 exposes five; part4 is binary PIT with default V1, no PROGRAM events.
 * present[4] records opt-in even for an empty/all-rest P section. Native
 * consumers must refuse P playback unless explicitly enabled and capable.
 */
void mml_reset(MML_ITER *iterator, const MML_SCORE *score, unsigned part);
/* Return 1 for an event, 0 after END, -1 on error. */
int mml_next(MML_ITER *iterator, MML_EVENT *event, MML_ERROR *error);
const char *mml_preset_name(unsigned program);

#endif
