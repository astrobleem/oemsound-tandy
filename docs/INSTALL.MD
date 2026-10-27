# Install Tandy sound in Windows

[Hardware and Windows requirements](../README.md#requirements) · [Downloads](DOWNLOADS.MD)

## Install the shared driver and player

The simplest download is **[TWSETUP.EXE](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/TWSETUP.EXE)**.
It is a native DOS installer with its files built in.

1. Copy TWSETUP.EXE to the Tandy. Exit Windows completely to DOS.
2. Run `TWSETUP`. Select your Windows installation if Setup finds more than one.
3. When Setup finishes, run:

   ```dos
   C:\WINAMP\GO
   ```

Setup installs `SOUND.DRV`, selects it in Windows, and supplies Winamp's DLLs
and launch helpers. GO starts Windows and Winamp with the required sound
and video support. Close the player and exit Windows normally to return to DOS.

If you prefer the full **TWXT2.ZIP**, extract its `TWXT2` folder and run
`SETUP.EXE` inside it instead. Both choices install the same driver and player.

## Add music tools

Choose **MML3AUTH.ZIP** for Beats Lab and Fiddle, or **PITEXPR.ZIP** for Beats
plus Mini Piano, JoyMIDI and MiniMIDI. Extract the complete folder, keeping
its files and subfolders together. Each kit includes the same TWSETUP.EXE;
run it once from DOS if shared sound is not already installed from the current kit.

Start with `C:\WINAMP\GO`, close the Winamp window, then use **File → Run**
in Program Manager to open an app from its extracted folder. For example:

```dos
C:\MML3AUTH\BEATS\BEATS.EXE
C:\MML3AUTH\FIDDLE\MMLFIDL.EXE
C:\PITEXPR\MINIPIAN.EXE
```

[The app guide](APPS.MD) lists every launch path and its first controls.
Stop the current sound producer before playing another. Keep each kit intact;
its app-local MIDI support files are already in the right place.

## Start Effects Lab

Extract **EFFECTS.ZIP** to `C:\EFFECTS`. If needed, run its TWSETUP.EXE once
from DOS. Then, from DOS:

```dos
CD \EFFECTS
GO C:\WINDOWS
```

Use your actual Windows directory if different. This starts Windows and
Effects Lab through its supplied launcher. Close Effects Lab and exit
Windows normally when finished.

## Repair or remove Winamp

From DOS, run `C:\WINAMP\SETUP` for **Launch**, **Repair** or **Uninstall**.
For missing or damaged installed files, run Setup from the original download
and choose Repair. Uninstall removes Winamp while leaving shared Windows
sound and other Windows XT clients installed.

If Setup or GO reports an error, use that message to identify the file,
Windows installation or busy sound/timer condition. [Installer details](../src/win30/SETUP/README.MD)
are available for troubleshooting; normal installation needs no file copying
or INI editing.
