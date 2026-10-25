# Fifth-lane bass: speaker-only admission

The A2 floor was inherited mapper admission, not the existing SOUND
service's limit. The parent approved this bounded app-local correction.
No shared SOUND driver, Winamp bank, timer or transport code changes.

## Current entry points

- `src/win30/DRIVER/PITCORE.H:pitSet` accepts whole-Hz 37..7902 and computes
  `(1193182UL + hz/2) / hz`. This service is unchanged.
- `BEATLAB/MIDIMAP.C:noteOn` first handles explicitly enabled channel 15
  (human MIDI channel 16), admitting notes 26..96 only there. Disabled PIT
  and all other melodic channels still reject below 45. PSG 45..96 and
  drums 35..81 are unchanged. The old 52 pitch entries are identical;
  only 19 lower entries are prepended. Debug 20/21 report speaker note/Hz.
- `BEATPAT.C`, `BEATEXP.C` retain lower P notes in .BTS and enabled MML3;
  disabling PIT preserves the editable lane but emits ordinary MML2
  without P. Beats preflights mapper 1.3 before owning sound for low notes.
- `FIDDLE/MMLCORE.C` permits octave 1 / notes 26..96 only in P, records
  `low_pit` during full validation, and clears it on revalidation.
  `MMLFIDL.C` accepts compatible 1.x mappers from 1.2, requiring 1.3 before
  ownership when sounding low P events are present. Older four-voice
  MML1/2 and P notes 45..96 retain their previous behavior.

MIDI 26/D1 rounds to 37 Hz, the service minimum. MIDI 25/C#1 rounds to 35 Hz and
is rejected. Demo bass 33/36/38 requests 55/65/73 Hz without folding, doubling
or another engine. These are rounded whole-Hz requests: the service's
actual divisors produce 55.000553/64.998747/72.999816 Hz respectively.

The maintained app-local mapper is under BEATLAB; FIDDLE has its matching
current parser/client. Both ship the same newly built mapper. Frozen
source snapshots and historical release bytes remain unchanged.

## Focused evidence

HOSTTEST.PY exercises 25 rejection, 26 at 37 Hz, proposed bass notes, old-range
identity, PIT off and other-channel rejection without mutation, stale
note-offs, retrigger/reset/close, ownership and missing-service failures.
It also compiles the actual unchanged PITCORE across every admitted
whole-Hz value 37..7902, and round-trips low exported P through Fiddle.
Fixed-gain reference audio checks every intended attack for AC signal,
correct routing/octave, balance and clipping. See VALIDATE.MD for counts,
actual native results and the separate unpublished packet gate.
