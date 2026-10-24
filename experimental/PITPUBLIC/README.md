# Tandy Windows and DOS sound kits

For Tandy 1000 EX/HX, DOS, 640 KB and Windows 3.0 real mode.

- **Sound driver and Windows instruments:** [download PITWIN.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/sound-exp-v0.1.0-pit.20261007/PITWIN.ZIP).
  Includes TSOUND.DRV, Mini Piano, Beats, JoyMIDI, MiniMIDI, PSGPLAY and chimes.
- **Write five-voice music:** [download MML3AUTH.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/sound-exp-v0.1.0-pit.20261007/MML3AUTH.ZIP)
  for Fiddle and Beats MML3 authoring.
- **Expressive instruments:** [download PITEXPR.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/sound-exp-v0.1.0-pit.20261007/PITEXPR.ZIP).
- **DOS sound test:** [download PITDOS.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/sound-exp-v0.1.0-pit.20261007/PITDOS.ZIP).

## Install the Windows sound driver

Extract PITWIN to `C:\PITWIN` and exit Windows to plain DOS:

```bat
COPY C:\PITWIN\TSOUND.DRV C:\WINDOWS\SYSTEM\TSOUND.DRV
```

Use your actual Windows directory. Under `[boot]` in `SYSTEM.INI`, change the
existing sound line to `sound.drv=TSOUND.DRV`. Keep each app's paired
MIDIMAP.DRV beside it; this mapper is app-local, not a system MIDI device.
Start Windows using your usual required display launcher, then use Program
Manager's **File / Run**, for example `C:\PITWIN\MINIPIAN.EXE`.
There is no automatic sound installer. Keep MML3AUTH's BEATS/FIDDLE folders intact.

The [short install guide](INSTALL.TXT) covers the optional speaker voice,
MML3 controls, DOS use and undo. [Winamp](../WINAMP30/INSTALL.TXT) uses this
same TSOUND filename and its own checked TPLAY launcher.

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
