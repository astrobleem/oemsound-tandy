# Optional native Windows 3.0 startup chime

TCHIME.EXE is a standalone, one-shot PSG app for a known original Tandy
1000 EX/HX running Windows 3.0 in real mode on an 8086/8088. It does not
require the experimental TSOUND.DRV. It never edits startup settings itself.

It plays an original short rising C5/E5/A5 figure, gently reduces volume,
mutes all four PSG channels, and exits. The three nominal fundamentals
are about 523, 658 and 881 Hz. The captured sound lasts about 0.85Ã¢â‚¬â€œ0.90s
in the tested startup fixtures. Output starts after the first delivered
Windows timer so the initial desktop painting can settle first.

There is no visible window, dialog, busy-wait, added startup pause, IRQ
hook or resident helper. A hidden window receives timer messages; GetMessage
yields to Program Manager and other startup apps. Windows unloads the app
after playback. Audio generation is in the PSG, not a sample-rate CPU loop.
This is asynchronous startup, not a promise of zero load/CPU overhead.

## Try it manually first

Copy TCHIME.EXE to C:\TANDY88\TCHIME.EXE (or another short DOS path).
From Program Manager choose File -> Run and enter that complete path.
A second simultaneous instance exits without touching the first instance.
The app silently refuses protected mode or a nonmatching Tandy ROM signature.
That guard does not identify an exact machine model or qualify real hardware.

Enable it only on known compatible hardware, with no other startup program
writing to the PSG. This direct-port app does not arbitrate with another
PSG driver or DOS sound producer; its cleanup mutes all four channels.

## Optional startup enable (reversible)

1. Close Windows. Make a new, separate backup of your existing WIN.INI;
   do not overwrite an older backup. Keep the working Windows installation.
2. Open C:\WINDOWS\WIN.INI in a text editor. In its existing [windows]
   section, locate run=. Preserve every existing entry and setting.
3. Append the full path to TCHIME.EXE once, separated from existing programs
   by a space. If run= is empty, the example is:

   [windows]
   run=c:\tandy88\tchime.exe

4. Save, then start Windows normally through your existing display-driver
   launcher. The chime is optional; no SYSTEM.INI or driver change is needed.
5. To disable, close Windows and remove only the TCHIME.EXE path from run=,
   preserving all other programs. Alternatively restore the backup only if
   you have made no other WIN.INI changes since taking it.

These directions were tested only in cloned guest fixtures. No existing
working startup settings were changed. Do not add the chime to AUTOEXEC.BAT:
it is a Windows app and should start after Windows has opened.

## Timing and cleanup limits

Windows 3.0 is cooperative. A task that never yields can delay timer delivery.
There is no absolute wall-clock mute guarantee while Windows is frozen.
For a delayed playback transition, the next delivered timer mutes and exits
instead of firing a burst of overdue notes. Nominal completion is 825ms from
the first note; a transition more than 220ms late aborts quietly. Stop, close,
session end, timer failure and ordinary application exit all mute channels.
No callback is installed into an interrupt or SYSTEM timer, so this EXE does
not leave an interrupt pointing into its code after unloading.

Normal cleanup is tested; forced process termination, frozen Windows and
physical-machine behavior remain unqualified. There are no user files,
registry entries or resident processes created by the chime itself.

## Build

Use the existing licensed Windows toolchain as described in ../README.MD:

python3 BUILD.PY --repo /path/to/windows-toolchain-repo \
  --dosbox /path/to/dosbox-x --out /new/chime-build

This uses Microsoft C /G0 /Gw /AS /W3 /Od and the segmented Windows linker.
The EXE is 3,999 bytes. Required Windows headers/libraries, OS media and tools
are not redistributed. Current binary and evidence are recorded in the
milestone package manifest and EVIDENCE/CHIME.

## Verification

Native startup tests: enabled in 640x200x4, 160x200x16 and the baseline display
profile; disabled in 640x200x4. All enabled cases captured all three tones,
then showed no TCHIME module or hidden window. A second startup observer ran
while TCHIME was still loaded, establishing asynchronous behavior. Disabled
startup captured digital silence. The observer is test-only and closes its
isolated Windows session; do not add BOOTTEST.EXE to a normal startup list.

See the included audit evidence for host-mocked failure/late-timer checks,
scoped 8088 binary analysis and their limitations. Physical Tandy listening
and analog sound level are still unverified. The WAV preview is an emulator
recording for listening elsewhere; TCHIME.EXE does not play WAV files.

TESTBOOT.PY creates fresh clones of clean startup fixtures (empty run=),
using --runtime-root, --exe, --observer, --dosbox and --out arguments.
The included BOOTTEST source builds with the same existing toolchain.
AUDIO.PY verifies the captured startup cases and observer logs.

LOCAL SHARED-OWNER EXTENSION
----------------------------
Playback now acquires OpenSound before direct PSG writes and closes it after
cleanup. Busy/unavailable sound performs no application PSG writes. Original
PSG phrase is the default. /pit explicitly enables the optional fixed-level
PIT layer; requires SOUND TANDYPITVOICE version1. No port fallback. Existing
cooperative timing and exit policy apply. See docs/PITCONS.MD for gates.

Historical measurements/hashes above describe the prior PSG-only release.
The isolated local build and new native receipts are delivered separately.
PSG-only use requires a working SOUND lease; optional PIT requires version1.
