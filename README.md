# OEMSound-Tandy

Music players and instruments for the **Tandy 1000 EX/HX** and Windows 3.0.

## Tandy Winamp

Browse your music, build a playlist, and play **MIDI, VGM/VGZ and tracker MOD
arrangements** through the Tandy PSG.

**[Download Tandy Winamp](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP)**
 · [Install guide](experimental/WINAMP30/INSTALL.TXT)
 · [Release notes](https://github.com/astrobleem/oemdisplay-tandy/releases/tag/windows-xt)

![Tandy Winamp in color](docs/SHOTS/WAMPCOL.PNG)

*Windows 3.0 player capture. [Capture details](docs/SHOTS/WINAMPCL.MD).*

- **Browse/Add:** choose songs from the Windows file browser.
- **MIDI instruments:** choose Default or Chamber; Winamp remembers your choice.
- **32-song playlist:** scroll past the visible rows or use Home, End and Page Up/Down.
- **Remove while playing:** change the list without deleting the song or stopping it.
- **A proper player window:** the title stays **Tandy Winamp**, with track and status inside.

![The Add song browser in color](docs/SHOTS/WAMPCBR.PNG)

*The same released app with its Add song browser open. The folder contains
original synthetic examples.*

### Try it

1. [Download TWXT2.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP).
2. Extract its `TWXT2` folder on the DOS computer, exit Windows, and run
   `SETUP` in that folder. It finds Windows and installs Winamp, the shared
   sound driver, DLLs and video-memory helper automatically.
3. Start the player from DOS with `C:\WINAMP\GO`.

Setup handles the existing Windows sound entry and current Windows XT sound
clients. The same SOUND driver serves Winamp, alerts and PIT instruments.
Run `C:\WINAMP\SETUP` for Launch, Repair or Uninstall Winamp.

The [install guide](experimental/WINAMP30/INSTALL.TXT) covers normal use.
[TWXT2.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP) includes the same ready-built runtime and corresponding source.

Choose Add... to load songs. Stop and close the player, then exit Windows
normally. TPLAY removes its owned timer bridge after Windows returns.

MIDI uses the Tandy voices and basic percussion. VGM/VGZ preserves the supported
PSG register stream. MOD follows supported tracker pitch, volume and effects
using square-wave tones; it does not reproduce sample timbres.
[Supported formats](docs/FORMATS.MD) · [Playback timing](docs/SNDTIME.MD)

## Your Tandy has an extra voice!

### Tandy FauxBlaster, from Doebelinschmirtz Incorporated!

Behold the Fifth-Voice-inator! **Fiddle and Beats** add an optional
**PC-speaker square-wave voice** alongside the Tandy PSG's **three tone voices
and noise channel**. A little extra mischief from the speaker you already own.

Write a fifth `[P]` part in MML3, or edit and export the fifth lane in Beats.

**[Download MML3AUTH.ZIP](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/MML3AUTH.ZIP)**
 · [Authoring and setup](experimental/PITPUBLIC/INSTALL.TXT)

For the other instruments and DOS sound test, use [current sound downloads](docs/DOWNLOADS.MD).

## Build your own Windows music app

**[libwintandy](https://github.com/astrobleem/libwintandy)** shares the 8086
Windows MIDI services, prepared-frame/ordered-burst playback backend and USER
UI helpers. Its VGM, gzip and MOD readers and foreground integration are
available as source with working examples.

Start with **[FORMATS30](https://github.com/astrobleem/libwintandy/tree/main/examples/FORMATS30)**,
then follow the [build guide](experimental/WINAMP30/BUILDING-UI.MD) and
[API/format boundaries](docs/FORMATS.MD#reuse-and-evidence). The playlist and
Browse/Add/Remove UI belong to Winamp; `TPLAY` is its guarded DOS launcher.

For separate source-only DOS and portable C89 Tandy components, see
[libtandy](https://github.com/astrobleem/libtandy). It covers SN76489-family PSG
state, graphics, surfaces, input and cooperative timing; the Windows driver and
apps here remain separate projects.

## More to play with

- **[Mini Piano](experimental/PITPUBLIC/INSTALL.TXT)** - play notes with the keyboard or mouse; use the current PITEXPR kit and its included Setup.
- **[JoyMIDI](experimental/PITPUBLIC/INSTALL.TXT)** - turn a joystick into a pitch/velocity instrument; use the same PITEXPR kit and shared sound setup.
- **[Tandy Beats Lab](src/win30/BEATLAB/README.MD)** - DWA: DAWs With Attitude. Edit five lanes, including PC speaker, with original editable demos and pattern Save/Load. [Authoring setup](experimental/PITPUBLIC/INSTALL.TXT).
- **[Effects Lab](src/win30/EFFECTS/README.MD)** - create short PSG or PC-speaker effects with editable presets, pitch sweeps, vibrato and gating; save patches and export libtandy C data. [Download the complete native kit](https://raw.githubusercontent.com/astrobleem/oemsound-tandy/main/artifacts/win30/EFFECTS.ZIP).
- **[Buddy Holly: BUDCAP video with PSG music and timed captions](https://youtu.be/wzUQWxt3Eno)** — an offline export. [BUDCAP 0.4 docs](examples/BUDDY/BUDCAP/README.md) describe the 256×160 DOS video at 4 fps. [Watch the genuine emulator demo GIF](docs/SHOTS/BUDCAP04.GIF)
  · [Capture details](docs/SHOTS/README.MD#budcap-04-animated-preview).
- **[Blondie's "Call Me" five-channel oscilloscope](https://youtu.be/gpdcfCYBdpk)** — an offline visualization
  of the Tandy PSG and PC-speaker arrangement.

The instrument kits include their paired app-local MIDI support. All use
the same installed SOUND driver; the included Setup handles it. Keep each
complete kit together and follow its current setup guide.

See [validation](experimental/WINAMP30/VALIDATION-UI.MD),
[checksums](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-xt2.20261025/SHA256SUMS)
and component notices for exact versions and scope. Winamp and the
[Windows XT display project](https://github.com/astrobleem/oemdisplay-tandy)
have separate release gates.

## Licensing

Keep the [root license](LICENSE) and component-specific notices. Winamp/shared
source and the first-party browser retain their MIT notices; the sound driver has its own
license. The Winamp package includes no private/commercial music, sample media,
Windows, ROMs or proprietary tools. BUDCAP's existing media has separate rights
and is not included in the Winamp download.
