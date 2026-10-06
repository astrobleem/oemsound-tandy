SOUNDP01 - sound ownership polish trial

Separate opt-in Windows 3.0 real-mode / 8088 / 640 KB test package.
No system SOUND driver, display driver, startup entry or existing package is
replaced. The original WININST12 and BUDENV02 archives remain unchanged.

RUNTIME
PSGPLAY.EXE: the existing C4/E4/G4 demonstration, now acquiring sound only
for playback. Idle/failed Stop and failed ownership do not touch the PSG.
TCHIME.EXE / XPCHIME.EXE: unchanged musical phrases, now quietly refusing
busy sound and releasing only their own acquired lease.
LEGACY: revised BEATS.EXE plus matching unchanged baseline MIDIMAP.DRV.
EXPRESS: revised expressive BEATS.EXE plus matching unchanged WININST12 mapper.
Use only one Beats variant, in its own directory with its paired mapper.

In Program Manager use File / Run and the desired full executable path.
Do not put MIDIMAP.DRV in WINDOWS\SYSTEM or register it in SYSTEM.INI.
Do not overwrite a working app directory. Closing this trial and returning
to that original folder is rollback. The display project's distribution and
installer-pinned chime binaries are deliberately not updated by this kit.

BEATS
Loading the editor or stopping a pattern no longer holds the sound device.
Play acquires it; Stop, Clear, Demo and Close release it. The in-memory pattern
survives Stop, and repeated Play reacquires sound. A busy Play leaves the
other owner's sound alone. No pattern persistence, tracks or synthesis modes
were added. The two mapper DLLs are unchanged; no system driver is installed.

LIMITS
Known older direct-I/O companions cause conservative busy refusal, even if
currently silent. OpenSound cannot arbitrate unrelated direct port writers.
Cooperative Windows scheduling and slow drawing still delay service; no new
IRQ/PIT hook or hardware timing guarantee is added. Physical Tandy acceptance
and perceived audio quality remain unverified. Experimental expressive
synthesis retains its earlier coarse service and timing limitations.

VERIFICATION
Six native Beats cases passed: full loops, repeated Play/Stop/edit/reacquire,
and Close while active, each with the baseline and expressive mapper.
Stock SOUND.DRV remained unchanged. Owner tests verified all three direct
sound apps emit zero PSG writes on busy refusal, then play normally and
release sound: 20 writes for TCHIME, 43 for XPCHIME, and the PSGPLAY triad.
All accepted cases use 640 KB, real mode, 8086_prefetch, no XMS/EMS/UMB/FPU,
and fixed 3000 emulator cycles. These cycles are not calibrated MHz.
Accepted ownership display modes: 160.
Scoped 8088 direct-control-flow audits have zero post-8088 suspects. They do
not certify imported Windows routines, indirect paths or physical timing.

The first observer attempted child launches inside its WM_CREATE callback
and stalled after the busy-refusal stage. It is not a passed test. The accepted
observer launches them after its first timer, after window creation returns.
Production sound apps were not changed to mask that harness initialization.

REPRODUCTION
Supply lawful Microsoft C6/Windows3 toolchain inputs and DOSBox-X.
For each SOURCE subfolder, run its BUILD.PY with explicit --repo TOOLCHAIN,
--dosbox DOSBOX and --out NEW_DIRECTORY arguments on a modern Python host.
The tools use /G0 and period DOS compilation. No compiler/Windows/emulator
binaries, installed guest files or display-driver binary are included.

QA/OWNER.EXE and the source test modes are isolated regression tools, not
normal applications. OWNER generates a test tone, launches companion apps
and exits Windows. Never run it in a working Windows session.
QA/RUNWIN.PY --help describes disposable-fixture inputs and the shared lock.
Run each Beats variant with --app BEATS and --arg=/test, /uitest, /closetest.
For OWNER, prepare a temporary runtime with QA/OWNER.EXE, the three direct
apps, and a paired MIDIMAP.DRV (the runner records its identity). Use
--app OWNER --arg=/test --cycles 3000. The fixture must supply its own lawful
Windows files and a suitable display driver; no working installation is used.

Normal applications do not write diagnostic logs. Explicit chime /test and
PSGPLAY test flags retain diagnostic logs for the disposable QA harness.
