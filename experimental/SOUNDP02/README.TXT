SOUNDP02 - small sound-app interface polish
6 October 2026

Separate opt-in Windows 3.0 real-mode / 8088 / 640 KB trials. Existing runtime
packages, WININST12, BUDENV02, display distributions and startup settings
are unchanged. Use a NEW directory and keep the original files for rollback.

WHAT CHANGED
Mini MIDI: sent/skipped counters move out of normal playback; /diag retains
them. Filename, Browse, Play/Stop, progress, file errors and limits remain.
Mini Piano: Keys held replaces the misleading input-holds/three-voice ratio.
The existing keyboard highlights show held keys, including a stolen note.
JOYMIDI: raw ranges stay visible while calibrating, then give way to note
names, next-attack strength and clear Arm/Muted state. /diag retains telemetry.
Calibration, release gate, Panic and invalid-input disarming are unchanged.
Mouth: choose HELLO, TANDY, YES or NO from a fixed dropdown, then Speak.
Enter also speaks; Escape closes the list and stops. Existing word/pose
frames and the mascot are unchanged; arbitrary text entry is removed.
TEXIT: a busy sound device offers Exit silently or Cancel. Cancel has initial
focus. Silent exit still calls normal Windows exit, preserving save queries
and application vetoes. Busy /preview never requests exit. Sound playback
holds and releases its own OpenSound lease.
Windows Buddy: Stopped replaces misleading Paused labels. Space starts the
song again; Enter starts full video. No paused playback position is retained.

RUNTIME FOLDERS
LEGACY contains the three instruments and unchanged baseline MIDIMAP.DRV.
EXPRESS contains the three expressive instruments and unchanged WININST12
MIDIMAP.DRV. The eight original presets/envelopes/vibrato are existing work;
no selector, synthesis change or new MIDI API is added by this package.
Close all mapper-based apps before switching LEGACY/EXPRESS versions:
Windows 3 can reuse an already loaded DLL by its module name. Keep each
executable beside its matching mapper; never put it in WINDOWS\SYSTEM or
register it in SYSTEM.INI. Launch through Program Manager /File /Run.
To use /diag, close the existing app first, then launch with that argument.
DIRECT contains standalone MOUTH.EXE and TEXIT.EXE. TEXIT /preview auditions
without exiting; /go is for an already-confirmed caller. Ordinary TEXIT asks.
BUDDY contains the revised Windows player and unchanged paired mapper only.
Supply media from the original repository examples/BUDDY/WINPLAY.ZIP:
  python HOST/PREPBUD.PY /path/to/WINPLAY.ZIP /path/to/new-folder
This helper validates the exact baseline archive and refuses existing output.
Its 64x48 picture, score/cues and controls remain; no media is duplicated here.

QUALIFICATION
41 native cases passed in disposable Windows 3.0 real-mode emulator guests
with normal 640 KB, 8086_prefetch, no XMS/EMS/UMB/FPU and fixed 3000 cycles.
12 Mouth cases cover all words, repeat, Stop, active Close, no-selection
safety and dropdown selection. 16 instrument cases cover baseline/expressive
playback, Browse, keyboard/mouse/joystick simulation, cleanup and exact event
oracles. 3 Buddy cases cover stop/restart/minimize/deactivate/completion with
an original four-second fixture. 10 TEXIT cases cover both confirmation
paths, Cancel/default focus, preview, accepted exit and application veto.
The expressive instrument faces, Mouth dropdown, Buddy and TEXIT busy choice
were checked in 160x200x16, 320x200x16 and 640x200x2. The legacy instrument
variants were exercised in 160 mode. Full original Buddy media playback was
not rerun for these label-only changes.

Nine scoped direct-control-flow 8088 audits have no post-8088 suspects.
The existing component-aware auditor excludes the relocated __wflags DATA
word; an older generic audit mistook that data word for an instruction.
This is not exhaustive imported-OS/indirect-path or physical certification.
Host checks passed: 677 SMF cases, 100,036 Piano cases / 65,894 exact events,
194,837 joystick checks / 10,231 randomized NoteOns. Source cores and mappers
remain unchanged. See QA/SUMMARY.json and EVIDENCE for exact identities.

The first TEXIT veto observer used a cross-task synchronous button command;
that can leave a modal GetMessage waiting. The accepted observer posts the
command like ordinary input, then verifies the actual Windows exit query and
veto. No production exit logic was changed to hide the test-harness issue.
An older Piano verifier expected RETURN.LOG; the adapter checks the actual
RUNWIN RETURNED.TXT marker and preserves the exact 36-event oracle.

LIMITS AND PRESERVED DECISIONS
Cooperative scheduling still causes delayed notes, synthesis and painting.
Screenshots show UI only. Joystick inputs in the instrument suite are
simulated; physical EX joystick/sound/latency acceptance remains open.
No sound-quality or speech-intelligibility improvement is claimed. DOS Buddy
captions/speech/arrangement, the accepted short BUDENV02 opening and the
chosen private PSG Speech Lab defaults are unchanged. Word-exact closing
caption timing remains unverified, so no speculative media edit was made.
No custom typed speech, additional voices, IRQ/PIT hooks or system driver
replacement is introduced. There are no OS/compiler/emulator binaries here.

REBUILD / TEST
Supply lawful Microsoft C6/Windows 3 toolchain inputs and DOSBox-X. Each SOURCE
folder retains BUILD.PY: use --repo TOOLCHAIN --dosbox DOSBOX --out NEW_DIR.
The expressive clients require the existing 1.2 mapper; legacy clients use
baseline mapping. All native builds use /G0. Python/host tools are not needed
on the Tandy to run the applications.
QA/RUNWIN.PY --help documents disposable fixture/display/runtime inputs.
It takes a shared lock itself; do not wrap it in a second lock. RUNMEDIA.PY
also copies the three authored Buddy fixture files. QA/BUDQA and QA/EXITQA
are diagnostic programs that manipulate test apps and exit Windows; never
run them in a working session. Their source/build scripts are included.
Original component test modes likewise require disposable guests.

Project and component notices remain in LICENSE, SOURCE and ATTRIB.TXT.
No underlying third-party media rights are granted by the code license.
