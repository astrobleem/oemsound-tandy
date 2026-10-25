# Tandy Windows and DOS sound kits

For Tandy 1000 EX/HX, DOS, 640 KB and Windows 3.0 real mode.

Choose [Fiddle/Beats, the instruments or the DOS test from current downloads](../../docs/DOWNLOADS.MD).
Fiddle/Beats authoring is in MML3AUTH.ZIP; Mini Piano, JoyMIDI, MiniMIDI and
expressive Beats are in PITEXPR.ZIP.

## Install

Extract the complete Windows kit with its application folders intact.
Exit Windows to DOS and run the included TWSETUP.EXE once. It installs the
single shared SOUND driver and its required files automatically. Keep the
supplied app-local MIDIMAP.DRV files beside their clients; no system MIDI
registration, driver copying or INI editing is needed.
The [install guide](INSTALL.TXT) gives the launch paths and controls.

Beats authoring has five editable rows: three PSG tones, noise and PC
speaker. Its PIT setting controls whether the fifth row plays and exports.
The expressive Beats kit also has five rows, but does not have the authoring
app's MML preview/export controls. The same installed SOUND driver serves
both kits and Winamp; their app-local MIDI support is not a second system
sound-driver installation.

## Known limits

PIT adds one fixed-level square-wave voice, selected explicitly. Stop one
producer before starting another; speaker music cannot run alongside
sampled speaker speech. These kits have scoped emulator evidence, not full
physical EX acceptance. Windows 3.1 is outside the qualified scope.

[Timing and playback details](../../docs/SNDTIME.MD) |
[Current releases](https://github.com/astrobleem/oemdisplay-tandy/releases/tag/windows-xt).
Source and component notices are included in each kit. Historical source
and test records retain their original scope.
