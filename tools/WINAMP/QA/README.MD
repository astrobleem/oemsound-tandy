Native acceptance observer (pending execution)

Copy the source/header/DEF/RSP/ASM/BAT files into a fresh DOS build staging
directory. Mount the existing owned MSC6/MASM5.10/LINK4 workspace as C and
the staging directory as D. Run BUILDQA.BAT inside the already coordinated
8086 DOS emulator slot. Do not invoke it outside the shared lease.

Copy the built RAWGUI.EXE and UNINTR.EXE into the disposable C:\MMLAB
Winamp fixture alongside the production player/runtime and STREAM.MID.
The test is TPLAY C:\WINDOWS /TEST, using the actual canonical production
SOUND.DRV installed once at C:\WINDOWS\SYSTEM\SOUND.DRV. The observer
checks its loaded module filename and stable handle across every phase.
Production-only public APIs are used; DEBUGTESTTICK must be absent.

WIN.INI optional [UNIFYQA] settings: sound=full installed path,
display=loaded display basename, app=absolute application directory.
Defaults: C:\WINDOWS\SYSTEM\SOUND.DRV, XRSYNC.DRV, C:\MMLAB.
The /TEST launcher restriction still requires C:\MMLAB for the observer.

The observer runs legacy queues and idle/managed-busy alerts; plain/raw
owner alert protection; independent Win16 task refusals and subsequent
handoff; PIT2 simple voice and prepared lease set/mute/suspend/reenter/
release/stale-token refusal; then two play/stop/replay cycles of the
unchanged player while moving and repainting graphics. Only PITREAD.ASM
reads port61; all writes belong to the production driver and audio runtime.
The observer also runs actual TCHIME/XPCHIME one-shots, TEXIT /testpreview
and TP /TEST with active focus, then busy variants with another raw owner.
All four rebuilt clients must be present beside the player. Their module
completion and ownership are checked; TP and TEXIT native logs are retained
as TPGOOD.LOG/TPBUSY.LOG and TEXGOOD.LOG/TEXBUSY.LOG.
Actual TSHELL startup is exercised next. The fixture must select
[boot]shell=TSHELL.EXE with the current shell at C:\WINDOWS\TSHELL.EXE,
current chimes in C:\WINXT, and TSHELL.INI [Sounds]Directory=C:\WINXT,
Startup=TANDY and [Startup]LoadRun=0. The observer launches TSHELL, observes
its TandyStart window, actual TCHIME module and sound ownership/release.
It sends the normal WM_ENDSESSION(TRUE) notification to its freshly
launched shell for cleanup without requesting Windows exit, and verifies
module/window unload. A second run holds the observer raw claim, requires
no startup chime and verifies the owner survives startup and cleanup.

Normal completion closes the player, confirms sound available, invokes an
idle alert, and exits Windows. The DOS launcher owns bridge cleanup.

Expected logs at the disposable C root: UNIFYQA.LOG, UNBUSY.LOG,
UNFREE.LOG. Every got/want line must match and the final line of each is
TOTAL_FAILURES got=0 want=0. VERIFYQA.PY validates these complete logs and
pins guest observer/driver bytes to the native build and supplied driver
SHA. It also requires --tshell-sha256 for the exact current shell and pins
the identical rebuilt chime bytes in MMLAB and WINXT. This verifier is
contributor tooling; consumers do not need Python.

PITBOOT.C/DEF/RSP and BLDPIT.BAT form a separate tiny contributor shell.
It launches the existing unmodified C:\PIT\MMLFIDL.EXE /pittest, records
the WinExec result in C:\PITBOOT.LOG and returns; the existing Fiddle
self-test is responsible for its documented automatic ExitWindows.
No Fiddle source, binary or musical data is changed by this helper.

Remaining separate acceptance: Tetris client execution if source is located,
installer upgrade/repair/rollback/refusal tests, bridge
cleanup logs and waveform inspection. This source does not claim any
execution, physical-hardware qualification, or those independent cases.

Initial actual-primary acceptance now precedes the legacy and manual-shell
cases. Functional checks run from the first observer timer dispatch after
its message loop starts, allowing the primary shell's startup launch call
to return. The observer verifies selected/loaded Windows-root TSHELL, the
bar's instance path and stable distinct task, then500ms of idle/no-chime
state. Only after revalidating that exact identity does it send the normal
WM_ENDSESSION(TRUE) notification. It requires observer survival, shell/window
unload and unchanged idle SOUND before the existing manual idle/busy startup
chime cases. No prior startup chime is claimed as observed.

Whether ending the primary shell automatically ends Windows is a mandatory
native gate. An incomplete log, missing survival assertion or premature
Windows return is failure, never a qualification pass. The PROGMAN control
skips only initial-primary teardown and retains the existing manual-primary
expectations, so its intentional profile failures remain explicit.
