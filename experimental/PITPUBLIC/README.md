# Tandy Windows and DOS sound kits

For Tandy 1000 EX/HX, DOS, 640 KB and Windows 3.0 real mode.

Choose [Fiddle/Beats, the instruments or the DOS test from current downloads](../../docs/DOWNLOADS.MD).
Each Windows packet includes TSOUND.DRV and its paired app-local MIDIMAP files.

## Install

Extract the selected kit. From plain DOS, copy its top-level TSOUND.DRV to
your Windows SYSTEM folder and set the existing `[boot]` sound.drv line to
`sound.drv=TSOUND.DRV`. Keep MIDIMAP.DRV beside its paired clients.
The [install guide](INSTALL.TXT) gives the exact paths and controls.

## Known limits and technical details

PIT2 adds one on/off square-wave voice; it is off until selected. It cannot
run alongside sampled speaker speech. TSOUND has restricted SOUND semantics,
including unsupported system beeps; direct beep writers can disturb playback.
These kits have scoped emulator tests, not full physical EX integration.
Windows 3.1 is not supported.

[Versioned timing and playback details](../../docs/SNDTIME.MD) ·
[Release notes and optional checksums](https://github.com/astrobleem/oemsound-tandy/releases/tag/sound-exp-v0.1.0-pit.20261007).
Source and original component notices are included in each kit. Historical
source and test records retain their original scope.
