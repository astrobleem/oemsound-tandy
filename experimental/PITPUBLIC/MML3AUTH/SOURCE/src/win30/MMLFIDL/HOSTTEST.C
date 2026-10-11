/* Portable C89 host conformance driver. Never linked into the Win16 app.
 * cc -x c -std=c89 -pedantic -Wall -Wextra MMLCORE.C HOSTTEST.C -o hosttest
 * hosttest --selftest
 * hosttest --noise-selftest
 * hosttest --dump score.mml [initialA initialB initialC]
 * hosttest --check score.mml
 */
#include "MMLCORE.H"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MML_SCORE score;
static MML_ITER it[MML_PARTS];
static MML_ERROR error;
static MML_EVENT ev;
static char source[MML_SOURCE_MAX + 2];
static unsigned failures = 0, checks = 0;

static void check(int ok, const char *what)
{
    ++checks;
    if (!ok) {
        ++failures;
        fprintf(stderr, "FAIL: %s\n", what);
    }
}

static int validate(const char *s)
{
    return mml_validate(&score, s, (unsigned)strlen(s), 0, &error);
}

static void good(const char *s, unsigned long duration, const char *what)
{
    check(validate(s), what);
    if (score.validated) check(score.duration_ms == duration, what);
}

static void bad(const char *s, unsigned code, const char *what)
{
    check(!validate(s), what);
    check(!score.validated, what);
    check(error.code == code, what);
    check(error.line >= 1 && error.column >= 1 &&
          error.offset <= (unsigned)strlen(s), what);
}

static void expect(unsigned kind, unsigned long time, unsigned pitch,
                   unsigned velocity, unsigned program)
{
    check(mml_next(&it[0], &ev, &error) == 1, "event exists");
    check(ev.kind == kind && ev.when_ms == time && ev.pitch == pitch &&
          ev.velocity == velocity && ev.program == program, "event fields");
}

static void generated_limits(void)
{
    unsigned i, pos;
    strcpy(source, "[A]\n"); pos = 4;
    for (i = 0; i < 512; ++i) {
        strcpy(source + pos, "[[T120]8]8"); pos += 10;
    }
    check(validate(source), "exact expanded-token budget accepted");
    check(score.token_count == 32768UL, "expanded command token count");
    strcat(source, "T120");
    bad(source, MML_E_TOKENS, "32769 expanded commands rejected");

    strcpy(source, "[A]\n"); pos = 4;
    for (i = 0; i < 256; ++i) {
        strcpy(source + pos, "[[T120]8]8"); pos += 10;
    }
    strcpy(source + pos, "\n[B]\n"); pos += 5;
    for (i = 0; i < 256; ++i) {
        strcpy(source + pos, "[[T120]8]8"); pos += 10;
    }
    check(validate(source), "shared exact token budget accepted");
    check(score.token_count == 32768UL, "shared expanded token total");
    strcat(source, "\n[C]\nT120");
    bad(source, MML_E_TOKENS, "global token budget across three parts");

    strcpy(source, "[A]\nT240L64"); pos = (unsigned)strlen(source);
    for (i = 0; i < 4096; ++i) source[pos++] = 'C';
    source[pos] = 0;
    good(source, 64000UL, "4096 notes accepted");
    check(score.note_count[0] == 4096, "note counter exact");
    source[pos++] = 'R'; source[pos] = 0;
    bad(source, MML_E_NOTES, "4097 note/rest tokens rejected");

    strcpy(source, "[A]\nT40L1"); pos = (unsigned)strlen(source);
    for (i = 0; i < 100; ++i) source[pos++] = 'C';
    source[pos] = 0;
    good(source, 600000UL, "exact 600 seconds accepted");
    source[pos++] = 'R'; source[pos] = 0;
    bad(source, MML_E_DURATION, "duration over 600 seconds rejected");

    strcpy(source, "[A]\n;");
    for (i = 5; i < MML_SOURCE_MAX; ++i) source[i] = 'x';
    source[MML_SOURCE_MAX] = 0;
    good(source, 0, "8192 source bytes accepted");
    source[MML_SOURCE_MAX] = 'x'; source[MML_SOURCE_MAX + 1] = 0;
    bad(source, MML_E_SOURCE, "8193 source bytes rejected");
}

static void fuzz(void)
{
    static const char alphabet[] = "ABCDEFRGTOLV@#.+-<>[];0123456789 \r\nKEYS";
    unsigned long seed;
    unsigned trial, n, i, total;
    int result;
    seed = 1UL;
    for (trial = 0; trial < 5000; ++trial) {
        strcpy(source, "[A]\n");
        seed = (seed * 1664525UL + 1013904223UL) & 0xffffffffUL;
        n = 1 + (unsigned)(seed % 240UL);
        for (i = 0; i < n; ++i) {
            seed = (seed * 1664525UL + 1013904223UL) & 0xffffffffUL;
            source[4 + i] = alphabet[seed % (sizeof(alphabet) - 1)];
        }
        source[4 + n] = 0;
        if (!validate(source)) {
            check(error.offset <= (unsigned)strlen(source) && error.line > 0 &&
                  error.column > 0, "fuzz source error in bounds");
        } else {
            total = 0;
            mml_reset(&it[0], &score, 0);
            do {
                result = mml_next(&it[0], &ev, &error);
                ++total;
                if (total > 20000) break;
            } while (result > 0);
            check(result == 0 && total <= 20000, "valid fuzz iteration bounded");
        }
    }
}

static int selftest(void)
{
    unsigned char initial[3];
    unsigned i;
    unsigned long last;
    int result;
    good("[A]\nT120 O4 L4 V12 @KEYS C D", 1000UL, "BASIC");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    expect(MML_NOTEON, 0, 60, 100, 0);
    expect(MML_NOTEOFF, 500, 60, 100, 0);
    expect(MML_NOTEON, 500, 62, 100, 0);
    expect(MML_NOTEOFF, 1000, 62, 100, 0);
    expect(MML_END, 1000, 0, 0, 0);
    check(mml_next(&it[0], &ev, &error) == 0, "iterator exhausted after END");

    good("[A]\nT120 O4 C4. R8 > C8", 1250UL, "DOTTED");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    expect(MML_NOTEON, 0, 60, 100, 0);
    expect(MML_NOTEOFF, 750, 60, 100, 0);
    expect(MML_NOTEON, 1000, 72, 100, 0);
    expect(MML_NOTEOFF, 1250, 72, 100, 0);
    expect(MML_END, 1250, 0, 0, 0);

    good("[A]\nT121 O4 [C64 R64]8", 496UL, "CARRIED");
    check(score.note_count[0] == 16, "CARRIED note/rest count");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    for (i = 0; i < 8; ++i) {
        expect(MML_NOTEON, (unsigned long)i * 62UL, 60, 100, 0);
        expect(MML_NOTEOFF, (unsigned long)i * 62UL + 31UL, 60, 100, 0);
    }
    expect(MML_END, 496UL, 0, 0, 0);

    good("[A]\nV0 C4 V15 @ORGAN C4", 1000UL, "silent velocity and max velocity");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    expect(MML_PROGRAM, 500, 0, 0, 16);
    expect(MML_NOTEON, 500, 60, 127, 16);
    expect(MML_NOTEOFF, 1000, 60, 127, 16);
    expect(MML_END, 1000, 0, 0, 16);

    initial[0] = 96; initial[1] = 48; initial[2] = 112;
    check(mml_validate(&score, "[A]\nC@ORGAN D@KEYS E",
          (unsigned)strlen("[A]\nC@ORGAN D@KEYS E"), initial, &error),
          "UI preset and explicit overrides");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 96);
    expect(MML_NOTEON, 0, 60, 100, 96);
    expect(MML_NOTEOFF, 500, 60, 100, 96);
    expect(MML_PROGRAM, 500, 0, 0, 16);
    expect(MML_NOTEON, 500, 62, 100, 16);
    expect(MML_NOTEOFF, 1000, 62, 100, 16);
    expect(MML_PROGRAM, 1000, 0, 0, 0);
    expect(MML_NOTEON, 1000, 64, 100, 0);
    mml_reset(&it[1], &score, 1);
    check(mml_next(&it[1], &ev, &error) == 1 && ev.program == 48,
          "missing section retains UI preset");
    check(mml_next(&it[1], &ev, &error) == 1 && ev.kind == MML_END &&
          ev.when_ms == 0, "missing section silent");
    initial[2] = 1;
    check(!mml_validate(&score, "[A]\n", 4, initial, &error), "bad UI preset rejected");

    good("; hi\r\n [c] ; comment\rO4 c#8.d-8 e+8.f-8\r\n[B]\nR4\n[A]\nC",
         1250UL, "case comments CRLF sections out of order and accidentals");
    good("[A]\n@keysO4C", 500UL, "adjacent preset and octave commands");
    good("[A]\nO2A O7C", 1000UL, "inclusive pitch endpoints");
    good("[A]\n[[C64R64]8]8", 4000UL, "two-level repeats");
    good("[A]\n[]8", 0, "empty bounded repeat");
    good("[A]\nT121C64T120C64T121C64", 93UL, "remainder across tempo changes");
    good("[A]\nV1C", 500UL, "minimum nonzero velocity");
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 0);
    expect(MML_NOTEON, 0, 60, 1, 0);

    good("[A]\nC@PAD@KEYS C\n[B]\n@BELL C\n[C]\nR8C8", 1000UL,
         "independent simultaneous iterators");
    for (i = 0; i < 3; ++i) mml_reset(&it[i], &score, i);
    for (i = 0; i < 3; ++i) {
        last = 0;
        while ((result = mml_next(&it[i], &ev, &error)) > 0) {
            check(ev.when_ms >= last && ev.part == i, "monotonic independent events");
            last = ev.when_ms;
        }
        check(result == 0, "independent iterator exhausted");
    }

    bad("", MML_E_SECTION, "empty source");
    bad(";comment", MML_E_SECTION, "no section");
    bad("C\n[A]\nC", MML_E_SECTION, "content before section");
    bad("[A] C", MML_E_SECTION, "header must stand alone");
    bad("[A]\nC\n[A]\nD", MML_E_SECTION, "duplicate section");
    check(error.line == 3 && error.column == 1 && error.offset == 6,
          "duplicate exact source position");
    bad("[A]\r\n  O1", MML_E_NUMBER, "octave too low");
    check(error.line == 2 && error.column == 3 && error.offset == 7,
          "CRLF exact error location");
    bad("[A]\nO8", MML_E_NUMBER, "octave too high");
    bad("[A]\nO2<", MML_E_NUMBER, "lower octave shift underflow");
    bad("[A]\nO7>", MML_E_NUMBER, "upper octave shift overflow");
    bad("[A]\nO2C", MML_E_PITCH, "pitch below 45");
    bad("[A]\nO7C#", MML_E_PITCH, "pitch above 96");
    bad("[A]\nT0", MML_E_NUMBER, "tempo zero");
    bad("[A]\nT39", MML_E_NUMBER, "tempo below 40");
    bad("[A]\nT241", MML_E_NUMBER, "tempo above 240");
    bad("[A]\nL3", MML_E_NUMBER, "unsupported default length");
    bad("[A]\nC3", MML_E_NUMBER, "unsupported note length");
    bad("[A]\nC0", MML_E_NUMBER, "zero note length");
    bad("[A]\nC65536", MML_E_NUMBER, "number uint16 overflow");
    bad("[A]\nT999999999999999999", MML_E_NUMBER, "long decimal overflow");
    bad("[A]\nV16", MML_E_NUMBER, "volume above 15");
    bad("[A]\nV-1", MML_E_NUMBER, "negative volume");
    bad("[A]\n@PLUCK", MML_E_PRESET, "legacy alias rejected");
    bad("[A]\n@", MML_E_PRESET, "missing preset");
    bad("[A]\nC..", MML_E_SYNTAX, "double dot");
    bad("[A]\nR#", MML_E_SYNTAX, "rest accidental");
    bad("[A]\nC&C", MML_E_SYNTAX, "tie unsupported");
    bad("[A]\n[C", MML_E_REPEAT, "unmatched repeat open");
    bad("[A]\nC]2", MML_E_REPEAT, "unmatched repeat close");
    good("[A]\n[C]", 0, "standalone C line starts a silent second section");
    bad("[A]\n[C] D", MML_E_REPEAT, "repeat count required");
    bad("[A]\n[C]9", MML_E_REPEAT, "repeat nine");
    bad("[A]\n[C]1", MML_E_REPEAT, "repeat one");
    bad("[A]\n[C]0", MML_E_REPEAT, "repeat zero");
    bad("[A]\n[[[C]2]2]2", MML_E_REPEAT, "third repeat level");
    bad("\xef\xbb\xbf[A]\nC", MML_E_SOURCE, "UTF-8 BOM");
    bad("[A]\n;\xc3\xa9", MML_E_SOURCE, "non-ASCII even in comment");
    source[0] = '['; source[1] = 'A'; source[2] = ']'; source[3] = '\n';
    source[4] = 0; source[5] = 'C';
    check(!mml_validate(&score, source, 6, 0, &error) && error.offset == 4,
          "embedded NUL with explicit source length rejected");
    mml_reset(&it[0], &score, 0);
    check(mml_next(&it[0], &ev, &error) == -1, "failed validation cannot play");

    generated_limits();
    fuzz();
    printf("%u checks, %u failures\n", checks, failures);
    return failures != 0;
}

static void noise_fuzz(void)
{
    static const char alphabet[] =
        "NRTOLVM/ABCDEFG@#.+-&<>[];0123456789 \t\r\nKEYS";
    unsigned long seed, last;
    unsigned trial, n, i, part, total;
    int result;
    seed = 0x4d4d4c32UL;
    for (trial = 0; trial < 10000; ++trial) {
        strcpy(source, "MML2\n[N]\n");
        seed = (seed * 1664525UL + 1013904223UL) & 0xffffffffUL;
        n = 1 + (unsigned)(seed % 300UL);
        for (i = 0; i < n; ++i) {
            seed = (seed * 1664525UL + 1013904223UL) & 0xffffffffUL;
            source[9 + i] = alphabet[seed % (sizeof(alphabet) - 1)];
        }
        source[9 + n] = 0;
        if (!validate(source)) {
            check(error.offset <= (unsigned)strlen(source) && error.line > 0 &&
                  error.column > 0, "noise fuzz source error in bounds");
        } else {
            check(score.version == 2 && score.part_count == 4,
                  "noise fuzz version and part count");
            for (part = 0; part < score.part_count; ++part) {
                total = 0; last = 0;
                mml_reset(&it[part], &score, part);
                while ((result = mml_next(&it[part], &ev, &error)) > 0) {
                    check(ev.part == part && ev.when_ms >= last,
                          "noise fuzz independent monotonic iteration");
                    check(part != MML_NOISE_PART ||
                          (ev.kind != MML_PROGRAM && ev.program == 0),
                          "noise fuzz no program event or preset");
                    last = ev.when_ms;
                    if (++total > 40000U) break;
                }
                check(result == 0 && total <= 40000U,
                      "noise fuzz finite iteration");
            }
        }
    }
}

static void noise_limits(void)
{
    unsigned i, p, pos;
    strcpy(source, "MML2\n");
    for (p = 0; p < 4; ++p) {
        strcat(source, p == 3 ? "[N]\n" : p == 2 ? "[C]\n" :
                       p == 1 ? "[B]\n" : "[A]\n");
        for (i = 0; i < 128; ++i) strcat(source, "[[T120]8]8");
        strcat(source, "\n");
    }
    good(source, 0, "four parts share exact 32768 token budget");
    check(score.token_count == 32768UL, "four-part token count exact");
    strcat(source, "T120");
    bad(source, MML_E_TOKENS, "noise cannot exceed four-part token budget");

    strcpy(source, "MML2\n");
    for (p = 0; p < 4; ++p) {
        strcat(source, p == 3 ? "[N]\n" : p == 2 ? "[C]\n" :
                       p == 1 ? "[B]\n" : "[A]\n");
        strcat(source, "T240L64");
        for (i = 0; i < 64; ++i)
            strcat(source, p == 3 ? "[[N35]8]8" : "[[C]8]8");
        strcat(source, "\n");
    }
    good(source, 64000UL, "4096 notes independently allowed on four parts");
    for (p = 0; p < 4; ++p)
        check(score.note_count[p] == 4096, "four-part note count exact");
    strcat(source, "R");
    bad(source, MML_E_NOTES, "noise 4097th note or rest rejected");

    strcpy(source, "MML2\n[N]\nT240 L64 V0");
    for (i = 0; i < 64; ++i) strcat(source, "[[N35]8]8");
    good(source, 64000UL, "4096 silent noise hits consume time");
    strcat(source, "N35");
    bad(source, MML_E_NOTES, "silent noise hits consume note budget");

    strcpy(source, "MML2\n[N]\nT40 L1 ");
    for (i = 0; i < 100; ++i) strcat(source, "N35 ");
    good(source, 600000UL, "noise exact 600 seconds accepted");
    strcat(source, "T240 R64");
    bad(source, MML_E_DURATION, "noise fractional excess over duration rejected");

    strcpy(source, "MML2\n[N]\n;"); pos = (unsigned)strlen(source);
    for (i = pos; i < MML_SOURCE_MAX; ++i) source[i] = 'x';
    source[MML_SOURCE_MAX] = 0;
    good(source, 0, "MML2 exact source byte boundary");
    source[MML_SOURCE_MAX] = 'x'; source[MML_SOURCE_MAX + 1] = 0;
    bad(source, MML_E_SOURCE, "MML2 source byte boundary exceeded");
}

static int noise_selftest(void)
{
    unsigned i;
    unsigned char initial[MML_TONE_PARTS];
    static const char *invalid[] = {
        "N", "N0", "N34", "N82", "N65536", "N9999999999999999999",
        "N-35", "N35/", "N35/0", "N35/3", "N35/128", "N35/65536",
        "N35//16", "N35/16..", "N35..", "N35#", "N35-", "N3516",
        "N35&N35", "N35/16/16", "C", "D", "E", "F", "G", "A", "B",
        "O4", "<", ">", "@KEYS", "@HIT", "R/8", "R#", "V16", "V-1",
        "L3", "T39", "T241", "[N35]1", "[N35]9", "[[[N35]2]2]2",
        "[N35", "N35]2", "MML2", "MML1", "[N]\nN35", "N35 / 16 . ."
    };
    good("[A]\nC", 500UL, "MML1 remains version one");
    check(score.version == 1 && score.part_count == 3,
          "MML1 exposes exactly three parts");
    mml_reset(&it[0], &score, 3);
    check(mml_next(&it[0], &ev, &error) == -1,
          "MML1 has no fourth iterator");
    good("MML2\n[A]\nC", 500UL, "MML2 optional absent noise");
    check(score.version == 2 && score.part_count == 4,
          "MML2 exposes four parts even if N absent");
    mml_reset(&it[0], &score, 3);
    expect(MML_END, 0, 0, 0, 0);
    check(ev.part == 3, "absent noise ends without program event");
    mml_reset(&it[0], &score, 4);
    check(mml_next(&it[0], &ev, &error) == -1,
          "MML2 out-of-range iterator rejected");
    good(" \r\n;Comment\r\n mMl2 ;Version\r\n[n]\r\nn35", 500UL,
         "version is case insensitive after blank and comment lines");
    mml_reset(&it[0], &score, 3);
    expect(MML_NOTEON, 0, 35, 73, 0);
    check(ev.part == 3, "noise event part three");
    expect(MML_NOTEOFF, 500, 35, 73, 0);
    expect(MML_END, 500, 0, 0, 0);

    good("MML2\n[N]\nL8 N35. R32 N35/16. N81/8", 875UL,
         "dotted default, slash length and explicit noise gate");
    mml_reset(&it[0], &score, 3);
    expect(MML_NOTEON, 0, 35, 73, 0);
    expect(MML_NOTEOFF, 375, 35, 73, 0);
    expect(MML_NOTEON, 438, 35, 73, 0);
    expect(MML_NOTEOFF, 625, 35, 73, 0);
    expect(MML_NOTEON, 625, 81, 73, 0);
    expect(MML_NOTEOFF, 875, 81, 73, 0);
    expect(MML_END, 875, 0, 0, 0);

    good("MML2\n[N]\nT121N35/64T120N35/64T121N35/64", 93UL,
         "noise carries remainder across tempo changes");
    good("MML2\n[N]\nT120V15[N35/16 N35/16]2 R8 V0N42/8 V1N81/8",
         1250UL, "noise repeat retrigger, gate, V0 and velocity endpoints");
    mml_reset(&it[0], &score, 3);
    for (i = 0; i < 4; ++i) {
        expect(MML_NOTEON, (unsigned long)i * 125UL, 35, 127, 0);
        expect(MML_NOTEOFF, (unsigned long)(i + 1) * 125UL, 35, 127, 0);
    }
    expect(MML_NOTEON, 1000, 81, 1, 0);
    expect(MML_NOTEOFF, 1250, 81, 1, 0);
    expect(MML_END, 1250, 0, 0, 0);

    strcpy(source, "MML2\n[N]\nT240L64 ");
    for (i = 35; i <= 81; ++i) sprintf(source + strlen(source), "N%u ", i);
    good(source, 734UL, "all 47 drum codes accepted");
    mml_reset(&it[0], &score, 3);
    for (i = 0; i < 47; ++i) {
        expect(MML_NOTEON, ((unsigned long)i * 125UL + 4UL) / 8UL,
               i + 35, 73, 0);
        expect(MML_NOTEOFF, ((unsigned long)(i + 1) * 125UL + 4UL) / 8UL,
               i + 35, 73, 0);
    }
    expect(MML_END, 734, 0, 0, 0);

    initial[0] = 96; initial[1] = 48; initial[2] = 112;
    strcpy(source, "MML2\n[N]\nN35\n[A]\nC");
    check(mml_validate(&score, source, (unsigned)strlen(source), initial, &error),
          "MML2 accepts exactly three caller preset bytes");
    mml_reset(&it[0], &score, 3);
    expect(MML_NOTEON, 0, 35, 73, 0);
    mml_reset(&it[0], &score, 0);
    expect(MML_PROGRAM, 0, 0, 0, 96);

    bad("[N]\nN35", MML_E_SECTION, "missing version cannot introduce N");
    bad("[A]\nN35", MML_E_SYNTAX, "unversioned noise token rejected");
    bad("MML2\n[A]\nN35", MML_E_SYNTAX, "noise hit cannot enter tone lane");
    bad("MML2 [N]\nN35", MML_E_SECTION, "version must stand alone");
    bad("MML2\nMML2\n[N]\nN35", MML_E_SECTION, "duplicate version rejected");
    bad("[A]\nC\nMML2\n[N]\nN35", MML_E_SECTION, "late version rejected");
    bad("MML2", MML_E_SECTION, "version alone is not a score");
    bad("MML2\r\n[N]\r\n  N34", MML_E_NUMBER, "invalid noise has exact location");
    check(error.offset == 13 && error.line == 3 && error.column == 3,
          "noise CRLF exact source coordinates");
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        strcpy(source, "MML2\n[N]\n"); strcat(source, invalid[i]);
        check(!validate(source) && !score.validated,
              "invalid noise token or section rejected");
        check(error.offset <= (unsigned)strlen(source) && error.line > 0 &&
              error.column > 0, "invalid noise diagnostic position in bounds");
    }
    noise_limits();
    noise_fuzz();
    printf("%u MML2 checks, %u failures\n", checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    FILE *f;
    unsigned size, part;
    unsigned char initial[MML_TONE_PARTS];
    int result, dump, c;
    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) return selftest();
    if (argc == 2 && strcmp(argv[1], "--noise-selftest") == 0)
        return noise_selftest();
    if ((argc != 3 && argc != 6) ||
        (strcmp(argv[1], "--dump") != 0 && strcmp(argv[1], "--check") != 0)) {
        fprintf(stderr, "Usage: hosttest --selftest | --noise-selftest | --dump file [presetA presetB presetC] | --check file\n");
        return 2;
    }
    f = fopen(argv[2], "rb");
    if (f == 0) { fprintf(stderr, "Cannot open %s\n", argv[2]); return 2; }
    size = 0;
    while ((c = fgetc(f)) != EOF && size < sizeof(source)) source[size++] = (char)c;
    fclose(f);
    if (size > MML_SOURCE_MAX) {
        fprintf(stderr, "Source exceeds 8192 bytes\n"); return 1;
    }
    for (part = 0; part < MML_TONE_PARTS; ++part)
        initial[part] = (unsigned char)(argc == 6 ? atoi(argv[3 + part]) : 0);
    if (!mml_validate(&score, source, size, initial, &error)) {
        fprintf(stderr, "ERROR %u %u %u %u %s\n", error.code, error.offset,
                error.line, error.column, error.message);
        return 1;
    }
    dump = strcmp(argv[1], "--dump") == 0;
    printf("SCORE %lu %lu %u %u %u", score.duration_ms, score.token_count,
            score.note_count[0], score.note_count[1], score.note_count[2]);
    if (score.version == 2) printf(" %u", score.note_count[MML_NOISE_PART]);
    printf("\n");
    if (!dump) return 0;
    for (part = 0; part < score.part_count; ++part) {
        mml_reset(&it[part], &score, part);
        while ((result = mml_next(&it[part], &ev, &error)) > 0)
            printf("EVENT %lu %u %u %u %u %u %u\n", ev.when_ms, ev.kind,
                   ev.part, ev.pitch, ev.velocity, ev.program, ev.offset);
        if (result < 0) return 1;
    }
    return 0;
}
