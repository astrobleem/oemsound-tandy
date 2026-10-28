# Downloads

Start with **[TWSETUP.EXE — native DOS Setup](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/TWSETUP.EXE)**.
It installs the shared Windows sound driver, Winamp and the required helpers.
[Installation instructions](INSTALL.MD).

| Download | What you get |
| --- | --- |
| [TWSETUP.EXE](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/TWSETUP.EXE) | The self-contained DOS installer. Run `TWSETUP`, then `C:\WINAMP\GO`. |
| [TWXT2.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP) | The same driver/player as an expanded kit, with original example songs and source. Extract `TWXT2` and run its `SETUP.EXE`. |
| [MML3AUTH.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/MML3AUTH.ZIP) | Beats Lab and Fiddle: editable patterns, five-voice music and MML scores. Includes shared Setup. |
| [PITEXPR.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/PITEXPR.ZIP) | The same Beats Lab, plus Mini Piano, JoyMIDI and MiniMIDI. Includes shared Setup. |
| [EFFECTS.ZIP](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/EFFECTS.ZIP) | Effects Lab with PSG/speaker patches, C export and shared Setup. Extract `EFFECTS`, then follow its `GO C:\WINDOWS` instructions. |
| [PITDOS.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/PITDOS.ZIP) | A separate DOS sound test. Extract it, change to its folder and run `PSGTEST` or `PSGTEST /pit`. Windows is not needed. |

The Windows apps share one installed `SOUND.DRV`. Choose the music kit by
which companion apps you want; there is no different driver to select.
[Requirements](../README.md#requirements) · [App controls and launch paths](APPS.MD)

## Buddy Holly tiny video players

These separate players run actual frame video with synthesized music.
Windows 3.0 real mode fits the 160x100 16-color CGA desktop; DOS supports
CGA, Tandy graphics and text fallbacks. Sound and display choices are independent.

| Download | What you get |
| --- | --- |
| [WINPLAY.ZIP](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/examples/BUDDY/WINPLAY.ZIP) | Windows player, one-voice speaker arrangement, Tandy-compatible PSG mapper, optional faster speaker clock, wallpaper and fresh-directory DOS installer. |
| [DOSPLAY.ZIP](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/examples/BUDDY/DOSPLAY.ZIP) | Standalone DOS player with speaker, PSG or silent output, and high/low-resolution video. Use LOWRES on an original-speed CGA 8088. |
| [WINSRC.ZIP](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/examples/BUDDY/WINSRC.ZIP) | Corresponding Windows source with the current broker inputs and license notices. DOS source is in the repository. |

[Install and controls](../examples/BUDDY/README.TXT) -
[Windows guide](../examples/BUDDY/WIN30/README.TXT) -
[DOS guide](../examples/BUDDY/DOS/README.TXT) -
[Paired release and checksums](https://github.com/astrobleem/oemsound-tandy/releases/tag/buddy-2026.10.28)

The Buddy installer uses a new app folder and leaves system/INI/boot files
alone. Auto uses recognized Tandy ROM hardware or falls back to PC speaker;
select PSG explicitly for a configured compatible C0 sound card. AdLib and
General MIDI are not implemented. Read the guides for fast-clock prerequisites,
attribution and emulator qualification limits.

Source and component notices are included. [Developer documentation](DEVELOP.MD)
keeps build instructions and versioned evidence separate from installation.
