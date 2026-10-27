# Historical UX3 speaker-integration notes

This retained candidate report predates the current shared sound release. See [current speaker support](SPEAKER.MD).

# Winamp speaker-assist work

UX3's extra speaker path was unfinished. Its queued frame holds three PSG tone
periods and noise, without a speaker divisor. The existing task-owned
TANDYPITVOICE export cannot be called directly from IRQ. The old front-page
wording made that sound like a user-selected disabled option; no such choice
was requested.

The isolated candidate now implements an additive resident TSOUND lease,
speaker sidecar for RAWCLK frames/bursts, MIDI and MOD allocation, low-note
octave fallback, availability status and a default-on next-Play setting.
VGM/VGZ remains a literal PSG register stream. TND/SND remain unchanged.

Host checks pass for the new pure renderer, MIDI transport, MOD allocation,
actual queued backend/resident lease and the existing renderer, MIDI/live,
format and exact ordered-byte paths. They cover missing/busy support, FULL
rollback, pause/stop/EOF/reset, DOS suspension and foreign speaker preservation;
the host IRQ guard reports zero Windows calls. All 13 host cases pass. MSC6/MASM
8086 builds pass and reproduce the original driver exactly. The paired native
tests pass 93 routing/lifecycle/UI checks, 30 old-driver fallback checks and
88 gzip/ordered-PSG/refusal/EOF checks. Native tests used Windows 3.0 real mode,
an HX 8088 at 4.772728 MHz, 640 KiB and stock CGA with the actual EX WIN.COM copy.
They observe hardware registers and ownership, not physical audio quality.

The compiled files add 8,280 bytes; NE minimum segment declarations add 10,168
bytes, including the 2,048-byte queue sidecar. Heap/stack reservations are
unchanged. These are binary measurements, without a calibrated CPU comparison.
DOS suspension/foreign-speaker preservation currently has host tests only.

The implementation spans the paired SOUND driver, queue and suspension hook,
MIDI/MCI providers, MOD foreground integration, UI and tests. It is not a
one-line enable switch. PIT0 deadlines and PIT2 square-wave output are separate;
PCM/PWM speech and speaker music share PIT2/gates and remain exclusive.

The published UX3 archive, runtime identities and CF copy are preserved.
The tested candidate remains local; no replacement UX3 binary is published by
this documentation update. Physical mixing/listening remains a separate owner
check. This work does not publish a Windows XT release candidate.
