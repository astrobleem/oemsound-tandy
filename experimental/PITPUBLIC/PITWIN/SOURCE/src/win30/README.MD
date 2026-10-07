# Native Tandy PSG milestone for Windows 3.0

This is a separate, opt-in 8086/8088 Windows 3.0 real-mode test for a
known Tandy 1000 EX/HX with its original three-voice PSG at I/O port C0h.
It does not detect a sound card. Do not run it on an unknown machine,
PCjr, a later-model sound configuration, or protected-mode Windows.
It refuses Windows protected mode, unknown ROM signatures and a second instance.

## Run

Copy PSGPLAY.EXE to your existing Windows machine. From Program Manager,
File -> Run, enter the path to PSGPLAY.EXE. No SYSTEM.INI change or
sound-driver installation is performed by this test. A working SOUND lease
is now required before any direct PSG writes.

Choose Play (or P): C4 on voice 1, then E4 on voice 2 after 0.6 seconds,
then G4 on voice 3 after 1.2 seconds. All stop after approximately 2.4 seconds on an idle system.
Choose Stop (or S/Escape) to mute immediately when Windows processes
that message. Play again restarts the sequence. Delayed timers extend the sequence rather
than silently skipping a voice. Closing also mutes all
four PSG channels, including noise. The window fits the 160-pixel
profile and uses ordinary GDI controls, text and message scheduling.

This test now cooperates with applications sharing OpenSound. It cannot
arbitrate with DOS programs, system beeps or other direct-port producers. There is no
PCM/WAV playback, MIDI support, Win3.1 multimedia interface, or sample
interrupt. Windows timers are cooperative: a blocked application,
modal operation or overloaded emulator can delay the next transition.
Normal Stop/close cleanup is verified separately from crashes, power
loss, forced process termination or frozen Windows, which cannot be
made safe by this small application.

A bounded diagnostic log, C:\PSGTRACE.LOG, is written when the app exits.
It contains BIOS/Windows clock and interrupt-vector snapshots followed
by the latest 128 timestamped PSG bytes. A DROPPED field reports overflow. It is overwritten each run. The log
records the bytes supplied to OUT; audio capture independently checks
the emulator's resulting sound. A trace alone is not proof of audio.

## Implementation

PSG periods 427, 339 and 285 correspond to approximately 261.97,
329.97 and 392.49 Hz for a 3.579545 MHz chip clock. Attenuation is 4
(8 dB below the PSG's maximum), and each Stop emits 9F BF DF FF.
Sixteen NOPs after OUT provide settling time on the 4.77 MHz 8088.
The app never changes the interrupt flag, interrupt vectors, PIT or
PIC. It uses WM_TIMER; it does not busy-wait for note durations.
No framebuffer allocation, display-driver change or direct video I/O.

## Build with an existing licensed DOS/Win16 toolchain

Build using Microsoft C 6, LINK4 (segmented linker), Windows headers,
SLIBCEW.LIB and LIBW.LIB. These dependencies are NOT redistributed in
this milestone package. The sound repository's root DOS-only toolchain
is insufficient by itself; the already-working display repository
provides the tested Windows libraries and linker in this workspace.

python3 BUILD.PY --repo /path/to/existing/windows-toolchain-repo \
  --dosbox /path/to/dosbox-x --out /new/build/directory

The toolchain repo must contain BIN/CL.EXE, BIN/LINK4.EXE,
DDK/286/INC/WINDOWS.H, INCLUDE, LIB/SLIBCEW.LIB and LIB/LIBW.LIB.
BUILD.BAT compiles /G0 /Gw /AS /W3 /Od; /G0 is required for 8088.
The helper requires a fresh output directory and reports the EXE hash.
The build is executed in DOSBox, not with a native Linux C compiler.

## Automated testing (disposable guest only)

/test plays the entire sequence, stops, and closes Windows.
/stoptest interrupts the first tone through the same Stop code path.
/closetest closes while the first tone is active.
These switches call ExitWindows after cleanup: use only in an isolated
Windows test session, never with unsaved work. Normal invocation does
not close Windows. Unknown arguments do not activate automation.

Use machine=tandy, cputype=8086_prefetch, memsize=0, memsizekb=640,
xms=false, ems=false, umb=false, fpu=false and your own installed
Windows 3.0 plus the existing accepted display driver/RESERVE launcher.
The emulator's name 8086_prefetch uses the intended four-byte prefetch
semantics; cputype=8088 is not a valid substitute in this DOSBox-X build.
A cycles value is an emulator speed setting, not a hardware MHz claim.

DX-CAPTURE /A /-V win /r c:\psgplay.exe /test

DOSBox-X can capture the resulting audio to WAV; this does not add WAV
playback support to the app. Expected port sequence is four initial
mutes, four start mutes, 8B 1A 94, A3 15 B4, CD 11 D4, then all-channel
mutes on stop, close and final exit. Timings are quantized by Windows.

## Verification limits and provenance

See EVIDENCE/ for actual results. Emulator verification is not physical
Tandy EX/HX listening or cycle-accurate hardware certification. Hardware
listening, write-settling behavior and analog volume remain unverified.
The newer legacy SOUND.DRV experiment is separate and does not make
this app a system-wide Windows multimedia or event-sound driver.

Upstream source baseline:
https://github.com/astrobleem/oemsound-tandy/commit/e811c22a6bbeb7bdbe2a04ed5bc6e713ba90e5c6

Existing root README says MIT while LICENSE contains GPLv3. This work
preserves those existing notices and does not resolve or change the
project's licensing claim. No stock DDK/Windows sources or binaries are
included in the milestone deliverable. Newly added source and test
artifacts are provided alongside the executable for review/rebuilding.

## Optional startup chime

CHIME/TCHIME.C adds a separate, optional one-shot startup app. It does not
require installing the experimental driver. See CHIME/README.MD for manual
tryout, opt-in WIN.INI setup, disabling it and startup-test evidence.

F3 toggles an optional fixed-level C3 PIT layer; /pit starts with it enabled.
Default is off. Optional playback uses version1 of SOUND TANDYPITVOICE and
refuses unavailable/busy sound. Stop, Escape, focus loss while enabled, option
changes and close silence owned output and release. Historical evidence below
refers to the older unleased demo; see docs/PITCONS.MD for current receipts.
