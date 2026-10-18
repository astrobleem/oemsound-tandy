# Tandy Winamp formats

[Tandy Winamp UX3 is published](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-ux3.20261018).
Its bounded MIDI, VGM/VGZ, MOD, TND and SND paths are integrated and have recorded
host/native qualification. The owner separately accepted the UX3 UI on a real
Tandy 1000 EX. This page supersedes the earlier adoption plan; it does not
announce the separate speaker-assist candidate as a released feature.

## Released UX3

| Input | Supported playback | Bounds |
| --- | --- | --- |
| MIDI | Standard MIDI File types 0/1, tempo changes, three expressive PSG tones and basic noise percussion | 32,768 bytes, 16 tracks, documented channel-message subset; melodic notes 45..96, percussion notes 35..81 |
| VGM | The supported SN76496 command/wait subset, preserving ordered register writes and explicit noise retriggers | Version syntax 1.00..1.71; strict 3,579,545 Hz clock, feedback 6, 15-bit shift register, zero variant flags; 60,000 uncompressed bytes |
| VGZ | The same checked VGM stream, decoded from one bounded gzip member | Stored/fixed/dynamic DEFLATE, CRC32/ISIZE validation; no concatenated members, trailing bytes or header CRC16 |
| MOD | `M.K.` and `4CHN` music arranged onto square-wave PSG tones | Finetune-zero used instruments, supported effects and bounded complete song path; at most three simultaneously audible source lanes |
| TND / AGI SND | The existing bounded raw-format readers | Their original schemas, ownership and limits remain unchanged |

MOD sample bytes are skipped. The arrangement retains supported pitch, volume,
effects and tempo, but does not reproduce sampled instrument timbres. Source
channel four is not implicitly percussion, and four source channels do not give
the PSG four melodic voices. The released producer uses sticky assignments to
the three tone voices and refuses a fourth simultaneously audible lane.

VGM rejects unsupported chips/variants, stereo writes, data blocks, PCM commands
and unsupported opcodes. Writes at one source timestamp form an ordered group
of at most 16 bytes. Distinct groups need at least 9,546 PIT cycles, the ceiling
of 8 ms; faster streams are refused with a source location instead of merged.
Attenuation changes do not invent noise-control writes. Explicit repeated noise
writes, selected noise mode and tone-2 coupling are retained. A valid VGM loop
gets its initial traversal plus one bounded repeat; empty/invalid loops refuse.

MIDI uses its bounded event/message profile rather than a General MIDI device.
Events closer than the prepared-state spacing can be coalesced by the selected
MIDI timing policy. The literal VGM path has a different contract and does not
use that coalescing policy. Refer to the matching source rather than assuming
that every MIDI controller or file with a recognized extension is supported.

VGM/VGZ/MOD have Pause/Resume. Play restarts from the beginning. Normal Stop,
EOF, switching and Close release the acknowledged sound owner. Pause/resume
restores complete hardware state and may restart noise; phase continuity is
not promised. The [complete format contract](https://github.com/astrobleem/libwintandy/blob/24ff4f932c0d4d4e2a18a3c49b0ea0a8ff90e71f/libtandywin/AUDIO/FORMATS30.MD)
lists tracker effects, tick/path limits, buffering and refusal details.

## Implemented authoring and speaker work

MML3 authoring is already implemented in the separately published
[Fiddle/Beats kit](../experimental/PITPUBLIC/INSTALL.TXT). Its explicit `[P]`
part uses an opt-in PIT2 square-wave speaker voice alongside PSG tone/noise.
That parser/editor belongs to those apps; MML3 is not a UX3 Winamp input.
It is not PCM/PWM sample playback.

UX3's queued backend records PSG state only. Its fifth-voice integration was
unfinished; there was no user instruction to disable an existing Winamp option.
A separate [speaker-assist candidate](SPEAKER.MD) now has source and host tests
for queued speaker state, MIDI/MOD allocation and low-note fallback. Its
16-bit/native validation is still pending. Those changes do not alter the
published UX3 binaries or imply a fifth channel in a VGM register stream.

## Reuse and evidence

[libwintandy](https://github.com/astrobleem/libwintandy) publishes the shared
MIDI service/transport, RAWCLK backend, foreground format-reader/integration
source and UI helpers. Start with
[FORMATS30](https://github.com/astrobleem/libwintandy/tree/24ff4f932c0d4d4e2a18a3c49b0ea0a8ff90e71f/examples/FORMATS30).
`FMTAPP` is source integration for a caller; it is not an exported all-format
DLL. The playlist, browser commands and selection policy belong to Winamp.

UX3 records 149 playlist host checks, 396 native UI checks and the independent
88-check gzip/PSG/refusal/EOF/ownership regression. Format/backend host suites
and the earlier bounded native format runs retain their own versioned records.
[UX3 validation](../experimental/WINAMP30/VALIDATION-UI.MD) separates those tests
from the qualitative owner report and measured allocation observations.
The current UI hardware report is not an exhaustive all-format physical sweep.

Public examples are original synthetic material. Commercial/private songs,
sample payloads, Windows/ROM/fonts/toolchains and emulator images are excluded.
See [timing and ownership](SNDTIME.MD) and the [install guide](../experimental/WINAMP30/INSTALL.TXT).
