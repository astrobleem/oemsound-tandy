# Sound timing and version scope

Sound timing depends on the exact player, producer, driver and release.
An application timer, an instrument step and a scheduled hardware deadline
measure different parts of playback. None of the releases below promises
1 ms live-input response under cooperative Windows 3.0.

## Public downloads and development candidates

| Version or path | Producer and playback timing | Availability and limits |
| --- | --- | --- |
| Mini MIDI 0.1 with its legacy app-local mapper | `SetTimer(PULSE,55)` drives the foreground message pump. Notes are sent when that pump runs. | Existing source/build guide; this player has no R4 prepared-state backend. Its recorded 49-299 ms worst-lateness results still apply to those tested binaries. |
| Legacy TSOUND SOUND API driver | A stock SYSTEM timer requested at 55 ms has roughly 54.925 ms cadence. This is separate from an application's USER timer. | Restricted SOUND API experiment. It does not take over IRQ0 or program a new PIT clock. |
| Public PIT sound kits | The Stage 4 expressive producer retains roughly 55 ms steps. The optional PIT2 square-wave voice is off by default. | Published experimental kits with pinned runtime/source pairs. They do not inherit private R4 timing. |
| Private R4 MIDICLK/RAWCLK and app-local MCI candidate | Foreground work prepares states with PIT-cycle deadlines; the prepared-state minimum spacing is 8 ms. USER refill and Stage 5 instrument steps remain 55 ms. | Private tested runtime plus a separate source adoption candidate. Fresh native adoption builds are pending; no new public R4 release is claimed. |

The [sound starter preview](https://github.com/astrobleem/oemsound-tandy/releases/tag/sound-v0.1.0-preview.20261007)
contains PSGSTART.ZIP and BEATSTART.ZIP. The
[public PIT prerelease](https://github.com/astrobleem/oemsound-tandy/releases/tag/sound-exp-v0.1.0-pit.20261007)
contains PITWIN.ZIP, PITEXPR.ZIP, MML3AUTH.ZIP and PITDOS.ZIP.
Use each package's corresponding source, runtime hashes and install guide.
These links do not distribute the newer private R4 candidate.

## What a prepared deadline improves

In R4, the foreground producer parses events and prepares bounded state
records before their playback deadlines. RAWCLK owns the scheduled hardware
delivery. A delayed USER message need not immediately move a deadline that
is already queued. PIT-cycle timestamps describe this backend clock; they
do not make the USER message loop run at that precision.

The producer must still refill its bounded queue. A long non-yielding task
can prevent refill, starve playback or delay visible updates and cleanup.
An 8 ms minimum spacing is an accepted-state constraint, not a measured
maximum scheduling error, audio fidelity claim or end-to-end latency bound.
It also does not change Stage 5 expression into an 8 ms instrument engine:
that instrument's steps remain 55 ms.

Foreground MIDI input must first reach its producer. Keyboard/joystick
polling, Windows dispatch, producer work and queue admission all contribute
latency before the backend can deliver a prepared state. Precise playback
of queued file events and responsive live input need separate tests.

## Players and formats

Mini MIDI 0.1 sends supported MIDI messages directly to its legacy mapper.
Its bounded file format, voice allocation and message limits remain in the
[Mini MIDI guide](../src/win30/MINIMIDI/README.MD). Program changes do not
give that legacy mapper the newer expressive instrument implementation.

The newer MINIPLAY (Tandy-Winamp) candidate has two paths. MIDI files use the
app-local MCI-compatible facade and its paired prepared-state providers.
Raw TND/AGI files use the raw parser/player path. Raw register states do not
become MIDI events or acquire MIDI program/envelope behavior. The existing
four-channel raw format does not gain an extra speaker channel.

The app-local MCI facade supports a bounded command subset. It is not a
complete MMSYSTEM implementation or a system-wide MIDI/wave device. Stock
MPLAYER compatibility remains unfinished. A successful MINIPLAY test does
not establish that unrelated Windows multimedia applications work.

Buddy's public WINPLAY/TINYVID and DOS players retain their own documented
cadence and evidence. The private R4 Buddy bridge is a separate version.
New backend work does not revise historical video, sampled-dialogue or
music timing results for those older packages. See the
[Buddy guide](../examples/BUDDY/README.TXT).

## Hardware and resource ownership

The shared Windows UI helpers use USER/GDI and can remain hardware neutral.
The current R4 audio admission and qualification target is the specified
Tandy HX fixture. PCjr audio support is pending; similar PSG hardware alone
does not establish matching clocks, audio routing, ownership or stop behavior.

The paired backend has one exclusive clock/PSG owner. A producer must respect
busy/full results, retry only the unaccepted suffix and retain resources
until close succeeds. Pause, reset, stop, end-of-file and close have distinct
state/ownership rules. A caller may not free a busy backend or leave sound
running after a successful stop. Direct hardware writers outside this
protocol can still interfere.

The public PIT kits' existing extra voice is an optional PIT2 square wave.
It has binary loudness and is mutually exclusive with sampled speaker/PWM
use. Its published admission/restoration limits and system-beep conflicts
remain in [INSTALL.TXT](../experimental/PITPUBLIC/INSTALL.TXT).

Automatic low-note fallback through PIT2 is not implemented in R4 or the
adoption candidate. A future extension needs explicit capability negotiation,
speaker ownership, deterministic one-voice allocation and tested gate/stop
behavior. The intended future supported mode enables it by default with a
toggle only after qualification. Existing public defaults remain unchanged;
unsupported low notes must not be silently clamped.

## Reading the evidence

Host tests exercise parsers, synthesis, event handling and contracts. Native
MSC builds check the segmented 16-bit compiler/linker path. Emulator tests
check the specified guest and machine model. Audio captures, register/event
traces, UI screenshots and physical hardware reports establish different
things; none alone proves all of them.

DOSBox-X cycle settings are not calibrated physical 8088 timings. A screenshot
shows its captured UI state and does not establish sound or latency. Preserve
the recorded executable/provider hashes and version labels when comparing
results. See [development status](STATUS.MD) and
[screenshot provenance](SHOTS/README.MD).
