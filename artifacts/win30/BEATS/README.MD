# Tandy Beats Lab, experimental preview

A tiny native Windows 3.0 real-mode step sequencer for the Tandy 1000 EX
PSG: three square-wave lanes and one noise/percussion lane, eight steps,
40–240 BPM, editable notes, Play/Stop and a built-in pattern.

## What MIDI means here

BEATS.EXE loads its paired MIDIMAP.DRV and calls the driver's midiMessage
entry directly with MODM_OPEN, MODM_DATA, MODM_RESET and MODM_CLOSE. Notes
are actual packed MIDI channel messages. The PSG output lives only in the
driver; the app has no OUT instruction or direct PSG bypass.

This completes a bounded experimental version of the existing repository's
previously silent MIDI handler. It is a direct-driver test harness, not
system-wide MIDI installation or an MMSYSTEM/midiOut device. Vanilla Windows
3.0 in this project has no multimedia MIDI dispatcher. Media Player, general
MIDI files, Win3.1 multimedia compatibility and physical MIDI interfaces are
not provided. The accepted SOUND driver is not modified.

## Running safely

1. Use an isolated copy of your working Tandy Windows 3.0 real-mode setup.
2. Place BEATS.EXE and the paired MIDIMAP.DRV in the same DOS directory.
3. Run BEATS.EXE through Program Manager / File / Run.
4. Use Stop or Esc before closing. Normal close also sends Reset and Close.

Do not overwrite a system MIDIMAP.DRV or register this preview in SYSTEM.INI.
No installation script changes the user's Windows setup. The driver checks
real mode and the Tandy ROM signature before PSG output. It uses OpenSound
for cooperative ownership, refusing to play when another SOUND client owns
it. The included tests use the previously accepted experimental TSOUND.DRV
only in disposable cloned guests. Arbitrary applications doing direct PSG
I/O cannot be arbitrated by this driver. Abrupt process termination remains
unqualified: WEP mutes using its own fixed code but cannot safely call another
DLL to release a retained SOUND lease; restart the isolated Windows session
if ownership remains stuck.

## Controls

- Click a cell to select it and toggle between its default pitch and a rest.
- Arrow keys select a cell; Space toggles it when the grid has focus.
- Pitch- / Pitch+ change the selected cell by a semitone (or drum number).
- Keyboard + / - also changes the selected pitch.
- BPM - / + change tempo by 10, bounded to 40–240 BPM.
- Play / P starts or restarts from step 1. Stop / S / Esc mutes immediately.
- Demo restores the starting pattern. Clear empties all four rows.
- Tab / Shift+Tab move among native buttons; Space activates a focused button.
- The noise row is MIDI channel 10 (zero-based channel 9).

Patterns live only in memory in this preview. Save/load and MIDI-file import
are deliberately not included. Playback uses the cooperative Windows timer;
it is not a precision MIDI clock. Timer quantization adds audible jitter,
and other applications can delay it. Musical deadlines retain their cadence
rather than accumulating each small dispatch delay. A stall longer than one
second re-anchors playback instead of issuing a catch-up burst.

## Bounded MIDI contract

- One owner task; wrong-owner and wrong-cookie mutations are refused.
- Three simultaneous melodic voices; channels 1–9 and 11–16 share them.
- Melodic note range 45 (A2) through 96 (C7). Lower pitches exceed the
  available 10-bit PSG divider and are rejected instead of silently clamped.
- Note On, Note Off and Note On with velocity zero are implemented.
- Velocity 1–127 maps to 15 sounding attenuation levels; 0 means note-off.
- Repeated note/channel pairs retrigger one voice. A fourth distinct melodic
  note steals the least recently triggered voice. A stale note-off for that
  stolen note does not stop its replacement.
- Channel-10 notes 35–81 use one monophonic noise voice. Bass drums 35/36 use
  periodic low-rate noise, snares 38/40 white medium-rate noise, hats 42/44/46
  white high-rate noise, and remaining drum numbers white low-rate noise.
  This is deliberately crude PSG percussion, not General MIDI instruments.
- CC 120 All Sound Off and CC 123 All Notes Off affect their own channel.
- Reset and normal Close mute every PSG channel. Close releases ownership.
- Program changes, pitch bend, other controllers, SysEx/long data, callbacks,
  running-status byte streams and unsupported driver messages are rejected.
- GETDEVCAPS honors the caller's byte count and reports experimental version
  1.01, three melodic voices, and no multimedia capability claims.

ABI: DWORD FAR PASCAL midiMessage(WORD device, WORD message, DWORD user,
DWORD param1, DWORD param2). MODM_OPEN takes a far DWORD cookie destination
in user and a TOPEN descriptor in param1; only a zero callback and flags are
supported. Success writes MIDI_COOKIE. DATA takes the packed status/data
bytes in param1 (status in bits 0–7). Subsequent owner calls use MIDI_COOKIE.
See MIDIAPI.H. modMessage is an alias for test compatibility, not a claim
that a multimedia runtime has loaded or qualified this driver.

## Build and tests

Use the existing licensed Microsoft C6 / MASM toolchain in a DOS environment.
BUILD.PY runs BUILD.BAT through the existing DOSBox-X launcher; /G0 targets
8086/8088 instructions. No compiler, DDK, Windows or guest images are shipped.
BUILD requires --repo (toolchain checkout), --dosbox and a new --out directory.

MIDITEST.EXE checks driver semantics and starts MIDIINTR.EXE as a second task
to check that nonowners cannot open, play, reset or close another task's
notes. BEATS /test plays two loops; /uitest exercises sequencer commands;
/closetest closes during playback. Those are automatic message-driven tests,
not claims of manual mouse/keyboard qualification. TESTWIN.PY documents the
isolated native emulation setup. See VALIDATE.MD for exact results and limits.

The app does not modify interrupt vectors, the PIT or PIC. The driver uses
only task-context PSG writes to C0h; there is no driver timer or callback.
All tests and audio are emulator evidence, not physical Tandy validation.

Upstream source inspected: https://github.com/astrobleem/oemsound-tandy/blob/main/src/MIDIMAP.C
This preview is delivered separately; no GitHub publication was performed.
