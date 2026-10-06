# Mini Piano 0.1

A compact native Windows 3.0 real-mode / 8088 Tandy EX/HX instrument.
It fits a 160x200 desktop and also works in 640x200 monochrome. It uses
three PSG square-wave voices, not sampled acoustic-piano timbres.

## Run

Build MINIPIAN.EXE with the period tools described in BUILD.MD. Copy it
and the existing paired MIDIMAP.DRV into a dedicated DOS directory, for
example C:\MINIPIAN. Run C:\MINIPIAN\MINIPIAN.EXE in Program Manager.
Use the unchanged driver built from ../BEATS; no driver source or binary
is duplicated here. Do not register it in SYSTEM.INI or WINDOWS\SYSTEM.
Use an existing working Windows/video/SOUND setup and a disposable test
copy first. There is no installer or configuration-changing setup step.

Normal startup is silent. The app acquires SOUND only while inputs hold
notes and releases it when the last input ends. Close Beats Lab before
playing; Beats Lab retains its lease while open. A duplicate launch
activates the existing Mini Piano window.

## Play

Hold a piano key with the left mouse button. Drag across keys to change
pitch; dragging outside the piano releases the mouse note, and returning
can play again. Release the button to stop. The physical US key positions
are:

    Key:  A  W  S  E  D  F  T  G  Y  H  U  J  K
    Note: C  C# D  D# E  F  F# G  G# A  A# B  C

Default C4..C5 is MIDI 60..72. Oct - / Oct + or Left / Right arrows select
C3..C4 through C6..C7, covering MIDI 48..96. An octave change releases
old notes; release/repress a held PC key to play in the new range.
Esc or Panic releases everything. Tab/Shift+Tab navigate buttons;
Space activates the focused button. The screen labels white keys with
PC-key letters above natural-note names; black keys carry W/E/T/Y/U.

Mapping uses scan codes, so other layouts may print different legends.
Physical keyboard rollover can limit detected combinations. Mouse and PC
keyboard can overlap: a shared note ends after its last input releases.
Auto-repeat and motion within one piano key do not retrigger notes.

## Sound and safety

The fixed velocity is 100. A fourth distinct note steals the oldest PSG
voice. A stolen held note does not resume automatically; release/repress
it. Highlights show held inputs, which may outnumber audible voices.
The existing driver's noise voice is unused by this melodic piano.

Focus loss, app deactivation, menu/titlebar actions, WM_CANCELMODE,
octave changes, Esc/Panic, normal Close and session-ending requests
release notes, reset voices and close the owned SOUND lease. A 100 ms
cooperative Windows watchdog checks capture and physical button release
while dragging. Windows 3.0 has no WM_CAPTURECHANGED; a capture-only loss
is caught by that watchdog or the next mouse event. Busy Windows tasks
can delay it beyond 100 ms. Music otherwise follows native input events.

Missing/incompatible driver, busy SOUND owner and send errors fail
silent with a compact status. After a busy refusal, release/repress a
note once the other app is finished. Failed ownership never resets the
other app's voices. Forced termination, crashes and abnormal shutdown
are unqualified; restart an isolated session if its lease is stuck.

Only pressed markers/labels and a status row are invalidated on ordinary
note changes. Initial painting can still be slow. Emulator cycle counts
do not certify physical 8088 latency or playability.

This is an app-local MIDI-message instrument, not a system-wide WinMM/
midiOut device or physical MIDI interface. There are no samples, sustain,
recording, MIDI-file playback, SID support, new IRQ hooks, PIT/PIC changes
or direct application PSG port writes. Physical hardware acceptance is
still pending. See TESTS.MD for measured scope and limitations.

Application/core: GPL-3.0-or-later; see the repository LICENSE. Required
license notices are preserved. Only source, portable tests and bounded
documentation are added; no binaries, media or private test logs.

STATUS POLISH
The status now says Keys held without dividing input holds by
three voices. Mouse and keyboard can hold the same note, and additional
notes can steal a voice; the unchanged key highlights represent holds.
