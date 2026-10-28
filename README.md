# OEMSound-Tandy

## Tandy MIDI and sound for Windows 3.0

Put the Tandy 1000's built-in sound to work in Windows. OEMSound provides
one shared Windows sound driver, MIDI playback software, and a collection
of native music tools for the hardware you already own.

**[Download DOS Setup](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/TWSETUP.EXE)** ·
[All downloads](docs/DOWNLOADS.MD) · [Installation guide](docs/INSTALL.MD)

### What it brings to Windows

- **Tandy PSG music:** three tone voices and noise percussion. Windows sound
  calls and alerts use the same installed driver.
- **MIDI instruments:** the included MIDI engine turns notes, pitch bend,
  sustain and expression into Tandy sound. Winamp offers Default and Chamber
  instrument banks.
- **An extra speaker voice:** supported apps can use the PC speaker for
  square-wave bass or effects alongside the PSG.
- **One sound installation:** the player, instruments and Windows XT sound
  clients share the same `SOUND.DRV`.

### Requirements

Tandy 1000 **EX or HX**, DOS **3.0 or later**, normal **640 KB** memory,
and **Windows 3.0 in real mode**, with a working display setup. Beats and
Effects Lab need a desktop at least 320 pixels wide. Windows 3.1, protected
mode and other Tandy/PCjr sound configurations are outside the supported scope.

### Install and hear it

1. Copy **TWSETUP.EXE** to the Tandy and exit Windows to DOS.
2. Run `TWSETUP`. It finds Windows and installs the shared sound driver,
   Winamp and its launch helpers.
3. Run `C:\WINAMP\GO` from DOS. In Winamp, choose **Add...**, select a song,
   then **Play**. Original example songs are included.

Setup handles the driver selection and required files. [The install guide](docs/INSTALL.MD)
also covers the ZIP option, the extra apps, Repair and Uninstall.

## Apps that use the driver

### Tandy Winamp — listen

![Current Tandy Winamp playing a MIDI song](docs/SHOTS/CURRENT/WINAMP.PNG)

Play MIDI, Tandy VGM/VGZ, MOD arrangements, TND and AGI SND files. Build a
playlist and choose a MIDI instrument bank. MOD plays PSG square-wave
arrangements rather than sampled instrument timbres.
[Player guide](experimental/WINAMP30/README.md) · [Supported formats](docs/FORMATS.MD)

### Beats Lab and Fiddle — make music

![Current five-lane Beats Lab editor](docs/SHOTS/BEATS/EDITOR.PNG)

Edit three PSG tone lanes, noise percussion and an optional PC-speaker lane.
Import Effects Lab patches into selected lanes and steps, transpose them, and
save the patches inside the song. Try the mixed Effects groove demo, pause or
stop playback, and export normal-note MML scores for Fiddle.
[Download the complete Beats + Effects kit](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/BEATSFX.ZIP).
[Beats guide](src/win30/BEATLAB/README.MD) · [Fiddle guide](src/win30/FIDDLE/README.MD)

### Effects Lab — make sounds

![Current Effects Lab editor](docs/SHOTS/EFFECTS/EDITOR.PNG)

Create coin, laser, explosion, jump and blip effects. Adjust pitch sweeps,
vibrato, envelopes and gating, preview on PSG or PC speaker, then save the
patch or export libtandy C data.
[Effects guide](src/win30/EFFECTS/README.MD)

The [instrument kit](docs/DOWNLOADS.MD) also includes **Mini Piano**, **JoyMIDI**
and **MiniMIDI**. All are clients of the same shared sound installation.
[Choose an app](docs/APPS.MD).

These are native Windows apps with included MIDI support; the kit does not
register a general WinMM `midiOut` device for arbitrary Windows MIDI software.
PSG instruments are synthesized chip sounds, not a General MIDI sample bank.

## Source and further reading

[Developer guide](docs/DEVELOP.MD) · [Validation and screenshot provenance](docs/SHOTS/CURRENT/README.MD) ·
[Earlier demos](docs/DEMOS.MD) · [Windows XT display project](https://github.com/astrobleem/oemdisplay-tandy)

The screenshots are authentic Windows 3.0 emulator captures of the current
apps. Runtime/source and component notices accompany the downloads. See
[LICENSE](LICENSE) and the [licensing notes](docs/DEVELOP.MD#licensing).
