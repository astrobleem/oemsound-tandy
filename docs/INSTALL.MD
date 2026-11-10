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

<a id="add-music-tools"></a>

## Start the Beats + Effects suite

Download **[BEATSFX.ZIP](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/BEATSFX.ZIP)**
for the current Beats editor with Effects patch import, Effects Lab and MML
Fiddle. Extract the complete `BEATSFX` folder, keeping its files together.

From DOS, using the example location `C:\BEATSFX`:

```dos
CD \BEATSFX
TWSETUP
GO
```

Setup installs the shared sound driver and Winamp. `GO` starts Beats;
`GOFX` starts the included Effects Lab. Both default to `C:\WINDOWS`.
If Windows is elsewhere, use `GO C:\WIN30` or `GOFX C:\WIN30` with its
actual directory.

In Beats, click **Demo** to try a pattern, then **Play**. The final demo,
**Effects groove**, combines imported effects with normal notes. To use
Fiddle, stop and close Beats, then enter `C:\BEATSFX\MMLFIDL.EXE` in
Program Manager's **File → Run**. Open an MML score exported from a normal
Beats pattern. FX-trigger songs use `.BTS` rather than MML.

Stop one sound app before playing another. When moving from an older kit,
keep the current folder intact and restart Windows before using it so an
older loaded audio DLL is not reused.

## Add the companion instruments

**PITEXPR.ZIP** adds Mini Piano, JoyMIDI and MiniMIDI. Extract its complete
folder. It includes the shared Setup and an earlier note-pattern version of
Beats; use **BEATSFX.ZIP** for the current Effects-capable editor.

Start with `C:\WINAMP\GO`, close the Winamp window, then use **File → Run**
in Program Manager, for example `C:\PITEXPR\MINIPIAN.EXE`. Keep each kit's
app-local support files together. [The app guide](APPS.MD) lists the other
launch paths.

**MML3AUTH.ZIP** remains available for the earlier Beats/Fiddle pairing.
New users should choose BEATSFX for the combined authoring suite.

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
