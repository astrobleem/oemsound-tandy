# Optional pre-exit chime for Windows 3.0

TEXIT.EXE is an explicit Exit Windows command for a known original Tandy PSG.
It asks first, plays a short original descending Bb4-G4-Eb4 companion phrase,
mutes all four channels, destroys its timer/window, then calls ExitWindows.
This is a new companion in E-flat, not a transcription of the XP shutdown tune.
No resident hook or replacement Windows/system file is installed.

The chime happens BEFORE Windows asks other applications to save or exit.
An application can veto the exit; the chime may already have played, but
Windows remains running. TEXIT does not force an exit, suppress save prompts,
or retry after a veto. Successful exit returns to DOS; it does not power off
the Tandy. Ordinary Program Manager exits do not automatically play this sound.

## Use and disable

Copy TEXIT.EXE to C:\TANDY88\TEXIT.EXE. Program Manager File, Run can launch it,
or create a Program Manager item called "Exit with chime" for that path.
The small OK/Cancel prompt defaults to Cancel. Cancel plays nothing and exits
only TEXIT. After OK, normal Windows/application save and cancellation behavior
still applies. To disable, stop using or remove that optional item; no startup
or system configuration was changed. Never add TEXIT to WIN.INI run=.

Options (exact lowercase spelling):
- /preview plays the phrase and closes TEXIT, without requesting Windows exit.
- /go skips TEXIT's own question, for a launcher that already confirmed exit.
  It still honors Windows/application vetoes and reports cancellation.
- /testgo and /testpreview are disposable-fixture diagnostics. They append
  C:\TEXIT.LOG and suppress the result dialog. /testgo can exit Windows.
  Do not use diagnostic commands with unsaved work.

TSHELL integration is optional and is not included or enabled by this package.
A future launcher can invoke TEXIT /go after its own confirmation. Do not call
it from WM_QUERYENDSESSION or WM_ENDSESSION.

## PSG ownership and timing limits

TEXIT refuses before touching PSG ports if a known PSG app window is open
(PSGPLAY, either startup chime, Automatic Mouth) or the experimental TSOUND
module is loaded. Close those apps or use ordinary Windows Exit. The check
is conservative, including idle windows. Unknown third-party PSG writers
cannot be detected reliably: close them first and do not start another PSG
producer during this short phrase. TEXIT does not claim a global hardware lock.

Playback uses yielding Windows messages and a 55 ms timer. The first timer
starts the sound after initial painting. Completion is nominally 825 ms;
an overdue note/fade transition over 220 ms late aborts both the phrase
and the exit request. A completed phrase can finish late if Windows delays it.
A grossly delayed incomplete phrase cannot trigger ExitWindows. There are no
IRQ/PIT/PIC changes or busy waits. All ordinary close/destroy, creation failure
and confirmed session-end paths mute channels. Another application's exit
query immediately cancels this chime and posts its close, without waiting.

Windows 3.0 is cooperative. No hard wall-clock mute is possible while another
task never yields or Windows is frozen. Forced task termination, unknown PSG
concurrency and physical Tandy listening remain unqualified. Protected mode,
duplicate instance and a mismatched ROM signature are rejected; that signature
is a compatibility guard, not proof of the exact model.

## Build and evidence

Use the existing licensed DOS Windows toolchain with:
python3 BUILD.PY --repo /path/to/toolchain --dosbox /path/to/dosbox-x \
  --out /new/exit-build

Microsoft C /G0 /Gw /AS /W3 /Od, segmented linker. The validated EXE is 7,103
bytes and a separate repeat build is identical. Stock OS/DDK/toolchain payloads
are not included. TESTWIN.PY uses fresh clones only; EXITOBS is a test-only
application that records native Windows query/end-session messages and later
exits the disposable fixture. Never use it as a normal application.

Nine native cases cover successful exit to DOS in three display profiles,
an independent application's veto, preview with no exit request, default
confirmation cancellation without sound or exit, Tab/Return acceptance, known-PSG
busy refusal and experimental-driver busy refusal. Audio checks verify all
three frequencies or exact digital silence for busy cases. Host mocks cover
66 core lifecycle/option/failure checks; the compact prompt is mocked there
and checked separately in the native GUI; a scoped audit checks 1,796 reachable
instructions. Imported Windows routines and indirect CRT continuations are
excluded from the audit. These checks do not certify physical hardware.

The included preview is captured PSG audio for listening elsewhere. TEXIT
writes note registers directly and does not play WAV files. Existing source
notices and repository README/LICENSE mismatch are preserved.
