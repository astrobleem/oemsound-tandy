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
the host IRQ guard reports zero Windows calls. Segmented 16-bit builds and
native routing/lifecycle validation are pending.

The implementation spans the paired SOUND driver, queue and suspension hook,
MIDI/MCI providers, MOD foreground integration, UI and tests. It is not a
one-line enable switch. PIT0 deadlines and PIT2 square-wave output are separate;
PCM/PWM speech and speaker music share PIT2/gates and remain exclusive.

The published UX3 archive, runtime identities and CF copy are preserved.
A new candidate must complete its native checks before it is offered as tested.
Physical mixing/listening remains a separate owner check. This work does not
publish a Windows XT release candidate.
