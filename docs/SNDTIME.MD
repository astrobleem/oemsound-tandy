# Sound timing and version scope

[Tandy Winamp UX3](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-ux3.20261018)
is a public app download with the prepared-state MIDI backend and ordered-burst
VGM/VGZ/MOD producers. The earlier description of these paths as only a private
R4 plan is obsolete. Timing still depends on the exact app/provider version.

## Which clock does what

| Published path | Scheduled output | Foreground service |
| --- | --- | --- |
| Winamp UX3 MIDI | MIDICLK renders prepared PSG states for RAWCLK PIT-cycle deadlines, with an 8 ms minimum state spacing | App-local MCI/USER refill and instrument steps retain an approximately 55 ms cadence |
| Winamp UX3 VGM/VGZ/MOD | RAWCLK emits ordered groups of at most 16 source/prepared bytes; groups require at least 9,546 PIT cycles | Foreground file reads, bounded parsing, synthesis and queue refill |
| Mini MIDI 0.1 / legacy mapper | Notes are submitted when that older app's USER timer runs | Requested 55 ms timer; its historical lateness measurements apply to those binaries |
| Legacy TSOUND SOUND API | Restricted SOUND queues use the existing SYSTEM timer, about 54.925 ms | Separate from the newer app-owned prepared deadline backend |
| Public PIT/MML3 kits | Optional fixed-level PIT2 speaker square wave; each kit retains its documented producer cadence | Stage 4 expression and older authoring apps do not automatically inherit UX3's deadline transport |

The newer clock uses an owned timer bridge and prepared records. The interrupt
path does no file I/O, parsing, synthesis, heap allocation or Windows calls.
A delayed USER message need not move a deadline that has already been queued.
It can still delay refill, visible UI and cleanup. The 55 ms foreground refill
interval is not the timestamp resolution of scheduled output; the 8 ms minimum
spacing is an admission constraint rather than a measured maximum jitter bound.

MIDI source timestamps use integer elapsed time. The selected file-playback
policy can coalesce closely spaced MIDI events into a prepared state. VGM
groups preserve source write order instead: a stream faster than their bounded
contract is refused, not silently coalesced. VGM wait conversion and MOD tempo
calculation retain fractional carry in PIT-cycle units.

The VGM/MOD ring holds 512 groups: about 8.53 seconds at 60 Hz or 4.096 seconds
at minimum spacing. MIDI uses its own bounded event/state queues. Reserve is
finite; a long foreground stall can starve output. Late admission or multiple
overdue groups is an explicit failure, without shifting timestamps to catch up.
These paths do not promise 1 ms response, zero jitter or sample-accurate audio.

## Formats, UI and shared services

MIDI files use the app-local MCITNDY/MIDICLK services. TND/SND use their raw
reader/player path. VGM/VGZ/MOD use FMTAPP/FMTCORE and negotiated RAWCLK bursts.
Register streams do not gain MIDI program/envelope behavior, and MOD plays a
PSG arrangement rather than its sampled timbres. See [formats](FORMATS.MD).

Winamp's message loop and timer service keep refilling while its file browser
runs a modal loop. The native UX3 test observed advancing foreground service,
PSG output and the displayed clock during a visible browser interval. The owner
separately reported uninterrupted playback while interacting with Program
Manager on the real Tandy EX at 7.16 MHz. This is useful evidence for those
workloads, not every Windows app or a measured physical zero-jitter result.

[libwintandy](https://github.com/astrobleem/libwintandy) contains the shared
services/backend and source-level foreground format integration. The app-local
MCI command subset is not a complete MMSYSTEM implementation or a registered
system-wide MIDI/wave device. TPLAY is a guarded DOS launcher; it is not a
public Windows library API. Unrelated multimedia clients and live input retain
their own compatibility and latency requirements.

Buddy's public Windows/DOS players retain their separately documented timing.
UX3 does not revise old video, subtitle or sampled-dialogue results. See the
[Buddy guide](../examples/BUDDY/README.TXT).

## Ownership and the speaker

One backend owns the clock/PSG path. Callers respect busy/full, retry only the
unaccepted suffix, and retain resources until acknowledged close succeeds.
Pause, reset, Stop, EOF and Close have distinct state/ownership rules. Normal
stop mutes and releases; direct hardware writers can still bypass cooperative
ownership.

PIT0 scheduling and PIT2 square-wave generation are separate counters. The
existing [public PIT kits](../experimental/PITPUBLIC/INSTALL.TXT) implement the
extra speaker voice through their SOUND owner. It has binary loudness and is
mutually exclusive with sampled speaker/PWM use. UX3's queued records have no
speaker state; the older task-checking speaker export cannot be called from
the IRQ path. This was an unfinished integration, not a user-requested disabled
Winamp option.

The separate [speaker-assist candidate](SPEAKER.MD) adds a negotiated resident
lease and speaker sidecar to the existing queue/owner. Its source and host tests
cover MIDI/MOD allocation, low-note octave fallback, pause/stop/EOF and DOS
suspension with foreign-speaker preservation. It defaults assistance on in the
candidate and reports availability. The paired 16-bit build and native
routing/lifecycle, old-driver fallback and gzip regressions pass. DOS-suspension
and foreign-speaker preservation have host tests; physical listening remains a
separate check. These changes are not in the published UX3 download. VGM remains
a literal PSG stream, with no inferred speaker channel.

## Reading the results

UX3 native tests used Windows 3.0 real mode, an HX 8088 emulator configured at
4.772728 MHz, 640 KiB and stock CGA. The qualitative owner report used the
physical Tandy 1000 EX at 7.16 MHz. Neither result qualifies PCjr audio,
Windows 3.1/protected mode, all formats/workloads or the pending combined
display-driver build. GitHub CI checks pinned package inputs, without native
Microsoft-toolchain compilation or a repeated hardware test.

Keep runtime hashes and version labels with timing comparisons. Host tests,
16-bit builds, register traces, screenshots, audio recordings and physical
reports establish different things. DOSBox-X cycle settings are not calibrated
physical CPU timings. [UX3 evidence](../experimental/WINAMP30/VALIDATION-UI.MD)
and [runtime pins](../experimental/WINAMP30/RUNTIME.JSON) identify the release.
