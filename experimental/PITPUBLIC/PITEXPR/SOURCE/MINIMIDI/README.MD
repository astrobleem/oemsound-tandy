MINI MIDI instrument test - Tandy Windows 3.0 experimental preview
==================================================

A small native Play/Stop MIDI-file player for testing the version 1.2 app-local
MIDIMAP.DRV from Tandy Beats Lab. It targets Windows 3.0 REAL MODE on an
8088-compatible Tandy EX/HX. It fits a 160x200 desktop and also works in mono.

QUICK START
-----------
1. Build the source, then copy the executable into its own DOS directory, for example C:\MINIMIDI.
2. Keep MINIMIDI.EXE and the paired instrument-test MIDIMAP.DRV together.
3. In Program Manager, File / Run: C:\MINIMIDI\MINIMIDI.EXE
4. TINY.MID is selected initially. Click Play. Try TYPE1.MID using Browse.
5. Stop or Esc immediately mutes and releases the driver's SOUND ownership.
   Normal Close and end-of-file also reset, close and free the file buffer.

Use your existing working Windows/video/SOUND setup. No installer is provided.
DO NOT copy MIDIMAP.DRV into WINDOWS\SYSTEM or register it in SYSTEM.INI.
This package does not replace a display driver, SOUND driver, shell or card.
Close Beats Lab before playing: it retains ownership while its window is open.
Mini MIDI acquires the driver only for playback and releases it on Stop/end.
A second Mini MIDI launch activates the existing window without another owner.

CONTROLS
--------
Type a DOS path into the filename box, or Browse through *.MID files, folders
and drives. Open selects a file; Play validates and starts it from the beginning.
Repeated Play restarts; Stop/Esc mutes; Tab and Shift+Tab move among controls,
and Space activates a focused button. There is no Pause, seeking or playlist.
An optional command-line filename also works, including a quoted DOS path.
The progress display is whole seconds; the bar updates about twice a second.
S is the number of channel messages sent; Skip counts unsupported channel
messages and out-of-range sounding Note Ons. Counters refresh with progress.
Selecting/opening an invalid file never starts notes: the complete file is
validated before driver acquisition. Browse stops playback first.

WHAT IT PLAYS
-------------
- Standard MIDI File type 0 and type 1; at most 16 tracks, 32768 bytes,
  16384 total events, and one hour of exact musical time.
- PPQN timing 1..32767, default 120 BPM, tempo changes, per-track running
  status, Note Off, Note On and Note On with velocity zero.
- Deterministic type-1 merge: lower track number wins equal-tick ties;
  original order within a track is retained. Tempo changes obey that order.
- Legal length-bounded metadata is skipped, with tempo/EOT specifically
  checked. Bounded F0/F7 SysEx is skipped, NEVER sent to the driver.
- Type 2, SMPTE division, invalid/truncated data, oversized lengths/VLQs,
  tick overflow, zero tempo, missing/nonfinal EOT, and budget overflow fail.

WHAT THE PSG CAN ACTUALLY DO
---------------------------
Three square-wave melodic voices plus one noise voice. This is NOT a General
MIDI synthesizer: its eight original envelope families are PSG approximations,
not sampled instruments or authentic GM patches.
Melodic Note Ons 45..96 play; notes outside that range are skipped. MIDI
channel 10 uses one crude noise voice for drum notes 35..81. A fourth distinct
melodic note steals the oldest voice. Velocity controls PSG attenuation.
CC120 (All Sound Off), CC123 (All Notes Off), and Program Change reach the
driver. Programs select original families per channel and affect new notes
only. Sustain/volume/expression and other controllers, pressure, pitch bend,
and unsupported channel messages are skipped. In particular, sustain
pedal does not extend notes. Songs relying on those features will differ.
Ignored SysEx/meta are not included in the channel-message Skip counter.

Use the paired 5948-byte version 1.2 instrument-test MIDIMAP.DRV.
See ../BEATS/INSTR.MD for the exact original preset and timing contract.
The player calls its MIDI-message entry directly; there is no direct port
I/O in the player. This is not an MMSYSTEM/midiOut device, Win3.1 multimedia
runtime, system-wide MIDI installation, MIDI keyboard input or physical MIDI.

TIMING AND FAILURE LIMITS
-------------------------
The parser carries an exact integer rational tempo remainder across changes,
then rounds event deadlines DOWN to the nearest millisecond. There is no
floating point, new ISR, PIT/PIC programming or high-frequency timer.
Actual playback uses a cooperative 55 ms Windows timer. Other apps, slow
video drawing, file processing and logging can delay it substantially.
A pump handles at most 64 due channel messages before yielding to Windows.
An event more than one second stale at pump entry causes a muted timing-stop
instead of an unlimited catch-up burst. Short notes can collapse into a burst
when the timer is late; do not treat this as a precision music sequencer.
This instrument-test fixture measured worst note lateness of 98 ms at
25000 emulator cycles and 764 ms at 3000; these are not hardware guarantees.
DOSBox cycle settings are not calibrated claims about physical EX speed.

Normal Stop, completion, parse/send error and window Close reset all PSG
voices and release ownership. Ownership failures do not reset another app's
notes. The driver checks Tandy ROM/real mode and cooperates through OpenSound.
Apps doing their own direct PSG I/O cannot be arbitrated. Forced termination,
crashes and abnormal Windows shutdown are not qualified; restart the isolated
Windows session if the SOUND lease is left stuck. Use a test copy first.

BUILD AND LICENSE
-----------------
Run python3 BUILD.PY --repo TOOLCHAIN_ROOT --dosbox DOSBOX --out NEW_DIR.
Supply lawful Microsoft C6/DDK inputs and DOSBox-X. No toolchain or OS is added.
The version 1.2 app-local MIDIMAP.DRV source is in this
repository at src/win30/BEATS. Do not register it system-wide.
The SMF parser is unchanged. Current native playback receipts and host tests
are in the supplied QA and EVIDENCE directories.
TESTWIN.PY requires a lawful disposable guest; its test modes exit Windows.
Application: GPLv3 (repository LICENSE); SMF core: MIT (CORE-MIT.TXT).
This experimental test kit is published separately. Stable binaries remain unchanged.
