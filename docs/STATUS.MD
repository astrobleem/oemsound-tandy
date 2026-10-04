# Sound development status and evidence

The README showcases available experiments. App-local MIDI-message playback,
the restricted legacy SOUND driver and DOS sound tools are separate components.
Milestone archives do not automatically include newer source previews.

## What MIDI means here

Mini MIDI, Mini Piano, JOYMIDI and Beats use the **paired app-local
`MIDIMAP.DRV` from `src/win30/BEATS`**. They call its MIDI-message entry
directly. The apps do not perform their own PSG port writes.

This is not an installed `midiOut`/WinMM device, a physical MIDI interface or
system-wide Media Player support. Keep the matching driver beside each app;
**do not register it in SYSTEM.INI or copy it into WINDOWS\SYSTEM**.
The legacy root `src/MIDIMAP.C` scaffold is separate from this working preview.

The paired driver implements:

- Three melodic voices shared across MIDI channels 1–9 and 11–16.
- Note On/Off, velocity-zero Note Off, velocity-to-attenuation mapping and
  least-recently-triggered voice stealing. Supported melodic notes are 45–96.
- One monophonic channel-10 noise voice for drum notes 35–81, using crude
  periodic/white-noise choices for bass drums, snares, hats and other drums.
- Per-channel CC120/CC123 all-off, plus global Reset and normal Close/mute.
- Exclusive task ownership with rejection of another task's mutations.

**It does not reproduce General MIDI instruments.** There are no sampled
piano/string patches. Program changes, pitch bend, sustain, volume/expression
and other unsupported controllers do not shape the sound. Mini MIDI skips
unsupported channel messages and out-of-range sounding notes; bounded SysEx
and most metadata are skipped without being sent to the driver. Songs relying
on those features will sound different or lose parts.

Mini MIDI supports type-1 track merging and integer tempo calculations, but
actual playback uses a cooperative roughly 55 ms Windows timer. Slow drawing
or other apps can delay dispatch; short notes can bunch together. It stops
and mutes on excessive lateness instead of issuing an unlimited catch-up burst.
Beats and JOYMIDI have their own bounded timing/filtering behavior. These are
small instruments and experiments, not precision music clocks.


## Driver work and earlier DOS experiments

The separate [TSOUND.DRV experiment](../src/win30/DRIVER/README.MD) implements a
restricted **Windows 3.0 legacy SOUND API**, not MIDI or wave output:

- Three voices with bounded note/sound queues, rest/note durations and accents.
- Tempo, articulation, pitch/volume settings for subsequently queued records.
- Start, Stop, Close, queue counts, threshold events and yielding wait states.
- Exclusive ownership, ordinary cleanup/mute and a stock SYSTEM timer with
  coarse cadence; no new IRQ/PIT/PIC programming in this driver.

It deliberately rejects unsupported envelopes, noise, fractional-frequency
requests and SyncAllVoices. DoBeep produces no system beep. Its recorded
105-check isolated native suite and PCM captures qualify that tested subset,
not complete SOUND compatibility. Installation/rollback instructions are for
**disposable cloned guests**; this is not a general replacement recommendation.
Abrupt owner exit and timer exhaustion remain unqualified.

Earlier source remains available for review: [PSGTEST](../src/psgtest.c) is the DOS
tone test; [WAVHYB](../src/wavhyb.c) experiments with unsigned 8-bit mono PCM using
PSG volume modulation plus the PC speaker. WAVHYB is DOS-side work with timer
hardware changes, not Windows wave-driver support or acceptance of sampled
playback in the newer Windows apps. The root build also retains the earlier
[MIDIMAP scaffold](../src/MIDIMAP.C); use each current component's own build guide.


## Run and build

Use a working Windows 3.0 **real-mode** installation on known compatible Tandy
hardware or an isolated Tandy emulator guest. Keep the display project's
existing guarded launcher/video-memory reservation. Sound apps do not install
or replace your display driver or shell.

Ready first-party milestone files and their source/evidence are under
[artifacts/win30](../artifacts/win30/README.md). Newer Mini MIDI, Mini Piano and
JOYMIDI previews have their own source/build/test guides under `src/win30`;
an older milestone archive does not automatically contain those additions.

For an app-local MIDI instrument, put its executable and the paired Beats
`MIDIMAP.DRV` together in a dedicated DOS directory, then launch the executable
with Program Manager > File > Run. Close Beats before trying another instrument:
it keeps its SOUND lease while open. Mini MIDI releases ownership on Stop/end;
Mini Piano uses it while notes are held; JOYMIDI requires explicit arming.
Arbitrary direct-port sound programs cannot share this cooperative ownership.

Build using licensed Microsoft C 6.0, the matching Windows headers/libraries,
MASM where required, and the segmented LINK4 linker inside DOSBox-X. Python 3
is the host build wrapper, not a Windows guest dependency. For example:

```sh
python3 src/win30/MINIMIDI/BUILD.PY --repo /path/to/toolchain --dosbox /path/to/dosbox-x --out /new/midi-build
```

Use a new output directory, inspect the recorded executable/driver identities
and follow the selected app's instructions. The Windows app builds target
8086/8088 with `/G0`; the root DOS batch is not a build for every modern component.
No Windows installation, ROM or compiler package is supplied by this documentation.


## Evidence, hardware reports and limits

The recorded emulator tests use Windows 3.0 real mode, 640 KB, no XMS/EMS/UMB,
and DOSBox-X's documented `8086_prefetch` setting. Cycle counts are emulator
parameters, not measured physical 8088 timing. Test-only switches in several
apps write diagnostics and exit Windows; use them only in isolated guests.

A recent user report from a genuine 8088 Tandy 1000 EX with DOS 6.22 and Windows
3.0 real mode says **Mini MIDI works, with limited sound quality**. This does
not establish fidelity, calibrated timing, every MIDI file or an exact-binary
hardware acceptance result. Mini Piano and the physical joystick/JOYMIDI path
remain unverified on hardware. No universal physical qualification is claimed
for Beats, Automatic Mouth, chimes or the restricted SOUND driver.

Normal Stop, close and completion paths mute/release as documented. Busy or
failed ownership requests leave the other owner alone in the app-local driver
path. Non-yielding Windows tasks can delay cleanup; forced termination, crashes
and abnormal shutdown are outside the qualified lifecycle. Direct-I/O programs
cannot be fully arbitrated. ROM signature guards do not identify an exact model.

For exact scope, see [Mini MIDI tests](../src/win30/MINIMIDI/TESTS.MD),
[Mini Piano tests](../src/win30/PIANO/TESTS.MD),
[JOYMIDI tests](../src/win30/JOYMIDI/TESTS.MD),
[Beats qualification](../artifacts/win30/BEATS/VALIDATE.MD) and
[the SOUND driver contract](../src/win30/DRIVER/README.MD).
Recorded native tests, host logic tests, audio inspection, hardware feedback
and GitHub CI are separate evidence. This README update does not claim a new
runtime test or CI pass.

Contributions and careful hardware reports are welcome. Include app/driver
hashes, environment and reproducible steps, and distinguish heard sound from
logged port writes. The repository contains a [GPLv3 LICENSE](../LICENSE) and
component-specific notices, including MIT cores. Its previous root README
said MIT; that historical mismatch needs maintainer reconciliation. This
documentation does not relicense any component.


## Screenshot provenance

[Original capture paths, source revision and SHA-256 identities](SHOTS/README.MD)
are separate from the UI showcase. Screenshots are byte-identical existing
emulator evidence and do not prove audio. The Mini Piano 0.1 capture is now
included; Mini MIDI and JOYMIDI capture assets remain future coverage. No new
emulator or CI pass is claimed by this documentation update.
