# Choose an app

All these Windows apps use the same shared sound installation.
[Downloads](DOWNLOADS.MD) · [Install once](INSTALL.MD)

| App | Try it | Open after setup |
| --- | --- | --- |
| **Tandy Winamp** | Add an included song, choose Play, then try Default/Chamber for MIDI. | From DOS: `C:\WINAMP\GO` |
| **Beats Lab** | Click Demo, Play, then edit a cell. Save `.BTS` patterns or export MML. | `C:\MML3AUTH\BEATS\BEATS.EXE`, or `C:\PITEXPR\BEATS.EXE` |
| **Fiddle** | Open a Beats-exported `.MML` score, edit the text and Play. | `C:\MML3AUTH\FIDDLE\MMLFIDL.EXE` |
| **Effects Lab** | Choose Coin/Laser/Boom/Jump/Blip, edit a field and preview. | From DOS in `C:\EFFECTS`: `GO C:\WINDOWS` |
| **Mini Piano** | Play the keys with the mouse or the PC keyboard. Esc/Panic releases notes. | `C:\PITEXPR\MINIPIAN.EXE` |
| **JoyMIDI** | Choose the joystick port, collect Range, Center, then Arm. Button 1 plays; Esc/Panic mutes. | `C:\PITEXPR\JOYMIDI.EXE` |
| **MiniMIDI** | Browse to a MIDI file and Play. Stop/Esc releases sound. | `C:\PITEXPR\MINIMIDI.EXE` |

Paths assume you extracted each kit directly on C:. Use the matching path if
you chose another folder. For the instruments and Fiddle, start Windows as
described in [installation](INSTALL.MD#add-music-tools), then enter the EXE
path in Program Manager's File → Run.

## Which voices do I get?

The PSG supplies three tone voices and noise percussion. The speaker adds
one fixed-volume square wave where the app supports it: the PIT control in
Beats/MiniMIDI, F3 in Mini Piano/JoyMIDI, or an MML3 `[P]` part in Fiddle.
Effects Lab selects PSG or speaker as separate output targets.

Winamp arranges MIDI for the available voices and can use the speaker for
low bass, with PSG fallback. Its VGM/VGZ path plays the supported Tandy PSG
register stream; it does not invent an extra speaker part. MOD plays a PSG
arrangement of supported pitch/volume/effects rather than the samples.

## App guides

[Winamp](../experimental/WINAMP30/README.md) ·
[Beats](../src/win30/BEATLAB/README.MD) ·
[Fiddle](../src/win30/FIDDLE/README.MD) ·
[Effects](../src/win30/EFFECTS/README.MD) ·
[Mini Piano](../src/win30/PIANO/README.MD) ·
[JoyMIDI](../src/win30/JOYMIDI/README.MD) ·
[MiniMIDI](../src/win30/MINIMIDI/README.MD)
