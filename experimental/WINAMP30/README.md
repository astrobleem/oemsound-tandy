# Tandy Winamp

[Download TWXT2.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP).
This is the native Windows 3.0 real-mode Tandy player. The expanded ZIP
includes DOS Setup, the player, its DLLs, shared SOUND driver and helpers.

## Install and start

1. Extract the TWXT2 folder on the DOS computer and exit Windows.
2. Run `SETUP.EXE` in that folder and select the Windows installation.
3. Start the installed player with `C:\WINAMP\GO`.

Setup selects the shared sound driver and installs the required files.
No manual driver copying, INI editing or separate helper download is needed.
The [install guide](INSTALL.TXT) also covers Launch, Repair and Uninstall.

## Play

Add opens the file browser. The playlist holds 32 songs; Remove changes
the list without deleting a song. The filename edit also accepts a path.
Stop and close the player, then exit Windows normally to return to DOS.

The MIDI bank button selects Default or Chamber and remembers the choice
for later playback. Held live voices retain the bank they started with.

Large VGM reads directly from disk. VGZ expands automatically, with
foreground progress and Cancel, then uses the same playback path. Temporary
files need no user configuration. MIDI retains sustain, volume, expression,
pitch bend, controller resets and PC speaker bass with PSG fallback.

The [current package tests](../../tools/WINAMP/TESTS.JSN) identify the exact
native builds, playback and cleanup evidence. Older records under LAUNCH
and EVIDENCE retain their original scope.

[Current downloads](../../docs/DOWNLOADS.MD) · [Install guide](INSTALL.TXT) ·
[Supported formats](../../docs/FORMATS.MD). Source and component notices
are included.
