JOYMIDI instrument-test preview - Tandy EX joystick-to-MIDI test instrument
====================================================

A small Windows 3.0 REAL MODE / 8088 application. It reads the built-in EX
joystick port, makes MIDI note messages, and sends them to the version 1.2
app-local MIDIMAP.DRV to play the Tandy PSG. This is an experimental, one-note
instrument, not a system-wide MIDI driver, WinMM device, DAW or external MIDI
interface. Physical EX joystick testing is still pending.

QUICK START
-----------
1. Build the source and copy the two runtime files into its own DOS directory, e.g. C:\JMIDI.
   Keep JOYMIDI.EXE and MIDIMAP.DRV together.
2. Use your existing working Windows 3.0 real-mode / Tandy SOUND setup.
   In Program Manager, File / Run: C:\JMIDI\JOYMIDI.EXE
3. The app starts MUTED and defaults to the right port, marked R. Press P
   to switch to left (L) if needed. Physical connector labels should be
   checked by moving one stick at a time.
4. Click Range (or R), and slowly move the joystick through its whole range
   several times. Both X and Y minimum/maximum values should change.
5. Put it at the physical center, then click Center (or C). The app takes a
   fresh reading. Each side of center needs at least 8 measured count steps.
   'Need wider range' means keep sweeping, center it, and try Center again.
6. Click Arm (or A), release button 1 once, then hold button 1 to play.
   Left/right selects MIDI notes 60..72 (C4..C5 chromatic). Y selects attack
   velocity 16..127: lower Y numbers are louder. Release button 1 to stop.
7. Click Panic, press Esc, or press joystick button 2 to mute and disarm.
   Arm again explicitly to resume. Clicking Mute also releases ownership.

No installer, card writes, registry or SYSTEM.INI changes are involved.
DO NOT install MIDIMAP.DRV into WINDOWS\SYSTEM or register it system-wide.
This application does not replace a video driver, SOUND driver or shell.
Close Beats Lab before arming; it retains ownership while its window is open.
Stop Mini MIDI playback first. 'PSG is busy' leaves the current owner alone.
A second JOYMIDI launch activates the existing instance.

CONTROLS AND DISPLAY
--------------------
Range / R: mute, clear calibration and collect a new range.
Center / C: mute and save fresh center; freeze a valid measured range.
Arm / A: acquire the driver and enable input; held B1 needs release first.
Mute / Panic / Esc / joystick B2: all-notes-off, reset and release.
P: switch R/L port, mute and restart calibration. Q: close.
Tab / Shift+Tab moves between buttons; Space activates a focused button.

X/Y range rows show measured low-high and C(center), in raw loop counts.
The next row shows normalized X/Y from 0..100 and B1/B2 state.
Note / Vel show the filtered pitch and the velocity for the next attack.
MIDI shows the last SENT three-byte message in hexadecimal. S counts sends.
For example, 90 48 7F is Note On, note 72, velocity 127 on channel 1.
The compact window fits both the 160x200 and 640x200 mono desktops tested.

Y does NOT continuously change a held note's volume. MIDIMAP lacks CC7.
Velocity is applied on the next press or on a horizontal pitch change.
Changing pitch while holding B1 sends the old Note Off before the new Note On.
Only one melodic note is used, on MIDI channel 1; no drums, chords, recording,
patch changes, sustain, pitch bend, external MIDI or MIDI-file output.

SAFETY AND TIMING LIMITS
-----------------------
The app starts muted and requires explicit range, center and Arm. Calibration
is session-only. There is no reliable joystick-presence bit: zero timing can
mean a valid electrical endpoint OR an absent/unknown input. To fail safely,
both selected axes must show high-to-low timing. A zero/unknown reading,
timeout, invalid calibration or B2 press immediately disarms on that poll.
An extreme physical endpoint may therefore cause a conservative mute.
Recovered input never auto-rearms. Recenter before arming again.

The production reader is copied unchanged from EXJOY build05: one write and
512 bounded reads at 0201h. No UART access, interrupt hooks, timer hooks,
PIT/PIC programming or interrupt masking is added. Windows interrupts may
stretch the measurement; raw counts are not calibrated time units.

Input uses a cooperative 55 ms Windows timer. Median-of-three filtering,
two-sample pitch confirmation, 2% boundary hysteresis and a 165 ms minimum
gap between Note Ons reduce jitter and note spam. A press needs two samples;
release and panic bypass the rate cap. This favors safety over fast playing.
The display refreshes around three times per second. Slow Windows drawing or
other programs can delay input and sound. A sampling gap above 500 ms causes
mute when dispatch resumes; this is not an independent real-time watchdog.
DOSBox cycle settings do not certify physical EX speed or latency.

Focus loss, minimization, Close, normal session ending, MIDI-send failure,
invalid input and Panic all disarm. Cleanup sends the active Note Off, CC123
and CC120 on channel 1, then resets the exclusively owned driver and closes
its OpenSound lease. A failed open never resets somebody else's voices.
Direct-PSG programs cannot be arbitrated. Forced termination, crashes and
abnormal shutdown remain unqualified; restart the isolated Windows session
if a SOUND lease gets stuck. Keep the speaker level low for first testing.

The paired mapper is the 5948-byte version 1.2 instrument test.
SHA256: a59ddd0c121d89a7d446eccc00e03f9646c3372e9505b9dcd48f6a44cbb5ac39
JOYMIDI uses its default Keys envelope, serviced by the existing 55 ms
Windows timer. Its controls still do not select patches. The driver accepts
Program Change from other callers; see ../BEATS/INSTR.MD.

BUILD
-----
python3 BUILD.PY --repo TOOLCHAIN_ROOT --dosbox DOSBOX --out NEW_DIR
Supply lawful C6/MASM/LINK4 and Win3.0 headers/libraries. Use the paired version 1.2
MIDIMAP from src/win30/BEATS.
See the kit validation report for current tests and bounds. App: GPLv3 (repository
LICENSE); copied original EXJOY core/reader: MIT (JOY-MIT.TXT).
This experimental test kit is published separately. Stable binaries remain unchanged.
