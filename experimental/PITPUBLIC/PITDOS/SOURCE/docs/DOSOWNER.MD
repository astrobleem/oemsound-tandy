# DOS cooperative sound adapter, revision 1

Maker handoff: DOSSND.H + DOSSND.C + DOSGUARD.H + existing
win30/DRIVER/PITCORE.H. Compile DOSSND.C once with MSC6 C89 /G0 /AS,
default packing and cdecl, then link DOSSND.OBJ into DOSPLAY. Include
DOSSND.H where the existing output wrappers call the adapter. No assembly,
installed driver, resident component or timer interrupt hook is added.

## Scope and acquisition

This adapter manages a single foreground DOS player's cooperative resources.
The verified original-Tandy ROM and INT2F context guard establish platform
eligibility, not protection against TSRs, BIOS beeps or unrelated direct I/O.
The player must run in a controlled exclusive DOS foreground session without
other sound/timer writers. It is not a cross-process lease. A true global lease
would require an independently coordinated resident component plus cooperating
clients; that is outside this change. Do not advertise universal ownership.

DosSoundAcquire(resources) is the explicit opt-in/capability boundary.
DS_PSG=1, DS_PIT=2, DS_PWM=4. Only nonempty combinations of these bits are valid.
PIT-required movies, even all-rest P, require explicit user enable checked by
the player before requesting DS_PIT. Acquire DS_PSG|DS_PIT, plus DS_PWM if speech
exists. Disabled/unavailable required PIT must refuse playback; never drop P.
The adapter cannot inspect a parsed movie or the player's command-line switch.

Complete frozen MML3/bundle/speech validation first, then acquire before initial
mute, PSG/speaker writes or video-mode changes. Hold acquisition for the entire
playback lifetime. Acquisition checks the existing DOSGUARD, the owner slot,
and a quiet port61 low-two-bit state, and performs no output writes. Windows3
real-mode DOS-child refusal occurs before even a port read. A second acquire,
including the same caller, refuses busy rather than adding hidden references.

## Exact Maker interface

All functions are explicitly cdecl. Zero means success; negative means refusal.

    int DosSoundAcquire(unsigned resources);
    int DosSoundPsgByte(unsigned value);
    int DosSoundPitNote(unsigned note,unsigned attack);
    int DosSoundPwmBegin(void);
    void DosSoundPwmEnd(void);
    void DosSoundRelease(void);

PsgByte accepts one byte only while DS_PSG is owned and PWM is inactive.
PitNote accepts note0/off with attack0, or MIDI45..96 with attack0/1 while
DS_PIT is owned. Equal pitch with attack0 holds without I/O. Changed pitch
implicitly attacks. Attack1 immediately gates off, reprograms channel2 mode3
and gates on, including an equal repeated pitch; no invented timed gap.
The table and rounded divisors exactly match the frozen PITCH.json fixture.
Invalid off+attack, pitch, flags, resource access and busy cases never write.
Port61 gate writes preserve the upper six bits from the immediately prior read.
Only C0h, 61h, 42h and channel2 mode3 command B6h to43h are written by the adapter.
SaveIRQ/RestoreIRQ restore the caller's flags, including an initially clear IF.
No PIT0/1 programming, IRQ/vector/cadence change or unknown old-divisor restore.

PwmBegin requires DS_PWM and success before calling the owner's legacy PlayPCM.
It gates owned PIT music off, forgets the pitch, mutes PSG output previously
written through the adapter, and blocks melodic PSG/PIT calls until PwmEnd.
Begin/end bracket the whole speech clip, not each chunk. The existing PlayPCM
owns its PWM hardware access during that bracket; this adapter does not alter
its mode0 sampling or its existing mode3/divisor0 finish sequence. PwmEnd clears
speaker gate bits and forgets state, with no saved-pitch/divisor restoration.
After speech, evaluate current movie time, invalidate muted output caches and
reapply only presently valid PSG/PIT state. Do not replay expired attacks.

Release clears owned PIT/PWM output, mutes PSG touched by this owner, and clears
ownership/resources. Repeated release is silent and safe. Untouched acquisition
and failed acquisition do not induce cleanup writes. Route all stop, error,
Escape/cancellation and exit paths through release; an abandoned process cannot
be cleaned up by a nonresident library. No direct-port fallback after refusal.
Errors: -1 no owner; -3 busy/speaker active; -13 pitch; -16 resource/flags;
-20 Windows/ROM context; -21 PWM-phase exclusion.

## Acceptance and limits

Host production-core tests: 20036 lifecycle/port checks, including wrong internal
owner, empty/unknown resources, disabled PIT, busy speaker, 64 upper-bit patterns,
all 52 pitches, hold/retrigger, speech exclusion, rearm and repeated cleanup.
Frozen 52 pitch/divisor entries compare exactly, independently of the formula.
MSC6 /G0 /AS builds both production DOSSND.C and the counted-I/O native harness.
Native foreground DOS and actual Windows3 real-mode DOS-child runs use the
shared emulator lock, unchanged owned guest source pins and bounded timeouts.
Foreground: 18 checks, no failures; child: 2 checks, no failures, zero port I/O.
Independent SPKR and TANDY tracks verify simultaneous emulator output.

The first native upper-bit test incorrectly compared two separated hardware
reads. The final check audits the value written against the immediate read;
dynamic input bits need not remain equal across time. That failed diagnostic
is retained outside the accepted kit. Final-source runs must pass before delivery.

This qualifies API state transitions and counted-I/O handoff, not real sampled
speech playback inside Maker, arbitrary Windows modes, physical EX summing,
human listening, unrelated TSR/BIOS interference or complete Maker playback.
Maker still owns parser enable checks, frozen speech guards, video timing,
whole-clip PlayPCM integration, cache invalidation and final runtime acceptance.
Old MML1/2 bytes and schemas are not changed by this adapter.
