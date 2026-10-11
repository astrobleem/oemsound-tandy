/* Production app + owner, mocked host or counted native DOS verification. */
#include <stdio.h>
#include <string.h>
#ifdef PSGTEST_NATIVE
#include <dos.h>
#include <conio.h>
#include "DOSGUARD.H"
#endif
#define DOS_SOUND_TEST
static unsigned reads, writes, badPorts, upperErrors, last61, programs;
static unsigned psgBytes[128], psgCount, mock61, mockFlags = 0x200;
static unsigned checks, errors;
static unsigned long tickCount, escapeAt;
static int sessionOkay = 1, noteBusy;
static FILE *report;
int SessionPermit(void)
{
#ifdef PSGTEST_NATIVE
    return DosSessionPermit();
#else
    return sessionOkay;
#endif
}
unsigned ReadPort(unsigned port)
{
    unsigned value;
    ++reads;
#ifdef PSGTEST_NATIVE
    value = inp(port);
#else
    value = mock61;
#endif
    if (port == 0x61) last61 = value;
    return value;
}
void WritePort(unsigned port, unsigned value)
{
    ++writes;
    if (port == 0x61 && (value & 0xfc) != (last61 & 0xfc))
        ++upperErrors;
    if (port != 0xc0 && port != 0x61 && port != 0x42 &&
        !(port == 0x43 && value == 0xb6)) ++badPorts;
    if (port == 0x43) ++programs;
    if (port == 0xc0 && psgCount < 128) psgBytes[psgCount++] = value;
#ifdef PSGTEST_NATIVE
    outp(port, value);
#else
    if (port == 0x61) mock61 = value;
#endif
    /* A foreign speaker starts after acquire, before optional PIT note. */
    if (noteBusy && port == 0xc0 && value == 0xd4) {
#ifdef PSGTEST_NATIVE
        outp(0x61, inp(0x61) | 3);
#else
        mock61 |= 3;
#endif
    }
}
unsigned SaveIRQ(void)
{
#ifdef PSGTEST_NATIVE
    unsigned flags;
    _asm pushf
    _asm pop flags
    _asm cli
    return flags;
#else
    unsigned flags = mockFlags;
    mockFlags &= ~0x200;
    return flags;
#endif
}
void RestoreIRQ(unsigned flags)
{
#ifdef PSGTEST_NATIVE
    _asm push flags
    _asm popf
#else
    mockFlags = flags;
#endif
}
#include "DOSSND.C"
unsigned long PsgTestTicks(void)
{
#ifdef PSGTEST_NATIVE
    union REGS r;
    r.h.ah = 0; int86(0x1a, &r, &r);
    tickCount = ((unsigned long)r.x.cx << 16) | r.x.dx;
    return tickCount;
#else
    return tickCount++;
#endif
}
int PsgTestKey(void)
{
    return escapeAt && currentNote == 72;
}
int PsgTestGetKey(void) { escapeAt = 0; return 27; }
#define PSGTEST_TEST
#define main PsgMain
#include "PSGTEST.C"
#undef main
static void Check(char *label, int okay)
{
    ++checks; if (!okay) ++errors;
    fprintf(report, "%s %s\n", okay ? "PASS" : "FAIL", label);
}
static void Reset(void)
{
    reads = writes = badPorts = upperErrors = programs = psgCount = 0;
    tickCount = escapeAt = 0; noteBusy = 0; mock61 = 0xa0;
    mockFlags = 0x200; sessionOkay = 1;
}
static int BaselineBytes(void)
{
    static unsigned expected[54] = {
        0x9f,0xbf,0xdf,0x8a,0x1a,0x94,0xa2,0x15,0xb4,
        0xcd,0x11,0xd4,0xe5,0xf6,0xff,
        0x94,0xb4,0xd4,0x95,0xb5,0xd5,0x96,0xb6,0xd6,
        0x97,0xb7,0xd7,0x98,0xb8,0xd8,0x99,0xb9,0xd9,
        0x9a,0xba,0xda,0x9b,0xbb,0xdb,0x9c,0xbc,0xdc,
        0x9d,0xbd,0xdd,0x9e,0xbe,0xde,0x9f,0xbf,0xdf,
        0x9f,0xbf,0xdf
    };
    unsigned i;
    if (psgCount != 58) return 0;
    for (i = 0; i < 54; ++i)
        if (psgBytes[i] != expected[i]) return 0;
    for (i = 0; i < 4; ++i)
        if (psgBytes[i+54] != 0x9fu+(i<<5)) return 0;
    return 1;
}
int main(int argc, char **argv)
{
    int result; unsigned before;
    char *plain[1] = {"PSGTEST"};
    char *pit[2] = {"PSGTEST", "/pit"};
    char *invalid[2] = {"PSGTEST", "/unknown"};
#ifdef PSGTEST_NATIVE
    void (interrupt far *irq8)(void), (interrupt far *irq1c)(void);
    irq8 = _dos_getvect(8); irq1c = _dos_getvect(0x1c);
    report = fopen("C:\\DOSCHILD.LOG", "w");
#else
    report = stdout;
#endif
    if (!report) return 2;
    Reset();
    if (!SessionPermit()) {
        result = PsgMain(2, pit);
        Check("WINDOWS_CHILD_REFUSAL", result == 1);
        Check("ZERO_SOUND_IO", reads == 0 && writes == 0);
        result = PsgMain(1, plain);
        Check("WINDOWS_DEFAULT_REFUSAL", result == 1);
        Check("DEFAULT_ZERO_SOUND_IO", reads == 0 && writes == 0);
    } else if (argc > 1 && !strcmp(argv[1], "/escape")) {
        escapeAt = 1; result = PsgMain(2, pit);
        Check("ESCAPE_WHILE_PIT_ACTIVE", result == 2 && programs == 1);
        Check("ESCAPE_OWNER_RELEASED", !soundOwner && !currentNote);
        Check("ESCAPE_NO_LATER_SEQUENCE", psgCount == 16);
    } else if (argc > 1 && !strcmp(argv[1], "/busy")) {
#ifdef PSGTEST_NATIVE
        outp(0x61, inp(0x61) | 3);
#else
        mock61 |= 3;
#endif
        result = PsgMain(2, pit);
        Check("BUSY_REFUSAL", result == 1);
        Check("BUSY_NO_SOUND_WRITE", writes == 0 && !soundOwner);
#ifdef PSGTEST_NATIVE
        outp(0x61, inp(0x61) & 0xfc);
#else
        mock61 &= 0xfc;
#endif
    } else if (argc > 1 && !strcmp(argv[1], "/note-busy")) {
        noteBusy = 1; result = PsgMain(2, pit);
        Check("PIT_CONFLICT_FAILS_CLOSED", result == 1 && !soundOwner);
        Check("FOREIGN_PIT_NOT_REPROGRAMMED", programs == 0);
#ifdef PSGTEST_NATIVE
        Check("FOREIGN_GATE_PRESERVED", (inp(0x61) & 3) == 3);
        outp(0x61, inp(0x61) & 0xfc);
#else
        Check("FOREIGN_GATE_PRESERVED", (mock61 & 3) == 3);
        mock61 &= 0xfc;
#endif
    } else {
        result = PsgMain(1, plain);
        Check("BASELINE_SEQUENCE_BYTES", result == 0 && BaselineBytes());
        Check("BASELINE_NO_PIT_WRITE", !programs && !pitActive);
        Check("BASELINE_RELEASE", !soundOwner);
        Check("BASELINE_PORT_AUDIT", !badPorts && !upperErrors);
        Reset(); result = PsgMain(2, pit);
        Check("OPTIONAL_PIT_SEQUENCE", result == 0 && BaselineBytes());
        Check("ONE_PIT_NOTE_PROGRAM", programs == 1);
        Check("PIT_RELEASE", !pitActive && !soundOwner && !currentNote);
        Check("PIT_PORT_AUDIT", !badPorts && !upperErrors);
        before = writes; DosSoundRelease();
        Check("IDEMPOTENT_CLOSE", writes == before);
        Reset(); result = PsgMain(2, invalid);
        Check("INVALID_OPTION_NO_IO", result == 1 && !reads && !writes);
#ifndef PSGTEST_NATIVE
        Reset(); sessionOkay = 0; result = PsgMain(2, pit);
        Check("CONTEXT_REFUSAL_NO_IO", result == 1 && !reads && !writes);
#endif
    }
    Check("ONLY_ALLOWED_PORTS", !badPorts);
    Check("PRESERVE_IMMEDIATE_61_UPPER_BITS", !upperErrors);
#ifdef PSGTEST_NATIVE
    Check("UNCHANGED_IRQ_VECTORS", irq8 == _dos_getvect(8) &&
        irq1c == _dos_getvect(0x1c));
    if (SessionPermit()) Check("SPEAKER_OFF", !(inp(0x61) & 3));
#else
    Check("RESTORED_INTERRUPT_FLAGS", mockFlags == 0x200);
#endif
    fprintf(report, "RESULT checks=%u failures=%u writes=%u reads=%u\n",
        checks, errors, writes, reads);
    if (report != stdout) fclose(report);
    return errors ? 1 : 0;
}
