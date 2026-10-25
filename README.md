# OEMSound-Tandy

Music players and instruments for the **Tandy 1000 EX/HX** and Windows 3.0.

## Tandy Winamp

Browse your music, build a playlist, and play **MIDI, VGM/VGZ and tracker MOD
arrangements** through the Tandy PSG.

**[Download Tandy Winamp](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-xt1.20261024/TWXT1.ZIP)**
 Â· [Install guide](experimental/WINAMP30/INSTALL.TXT)
 Â· [Release notes](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-xt1.20261024)

![Tandy Winamp in color](docs/SHOTS/WAMPCOL.PNG)

*Windows 3.0 player capture. [Capture details](docs/SHOTS/WINAMPCL.MD).*

- **Browse/Add:** choose songs from the Windows file browser.
- **32-song playlist:** scroll past the visible rows or use Home, End and Page Up/Down.
- **Remove while playing:** change the list without deleting the song or stopping it.
- **A proper player window:** the title stays **Tandy Winamp**, with track and status inside.

![The Add song browser in color](docs/SHOTS/WAMPCBR.PNG)

*The same released app with its Add song browser open. The folder contains
original synthetic examples.*

On a real **Tandy 1000 EX at 7.16 MHz**, the player was accepted with reported
uninterrupted music while using Program Manager.
[Hardware report and test record](experimental/WINAMP30/EVIDENCE/PHYSICAL.JSON).

### Try it

1. [Download TWXT1.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-xt1.20261024/TWXT1.ZIP) and extract its `TWXT1` folder to
   `C:\TWXT1`. The sound driver and video helper are included.
2. Exit Windows. Copy the included driver:

   ```dos
   COPY C:\TWXT1\TSOUND.DRV C:\WINDOWS\SYSTEM\TSOUND.DRV
   ```

   Under `[boot]` in `C:\WINDOWS\SYSTEM.INI`, change the existing sound line
   to `sound.drv=TSOUND.DRV`. Keep `system.drv=SYSTEM.DRV` and your display/fonts.
3. From plain DOS, check:

   ```dos
   C:\TWXT1\TPLAY C:\WINDOWS /CHECK
   ```

   After **preflight PASS**, start:

   ```dos
   C:\TWXT1\GO
   ```

Use your actual Windows directory if different; `GO C:\W30SND`, for example,
starts that installation. HPXY and XRSYNC use the included video reservation
automatically. Stock CGA uses the same command without reservation.
The [install guide](experimental/WINAMP30/INSTALL.TXT) covers other paths and undo.

Choose Add... to load songs. Stop and close the player, then exit Windows
normally. TPLAY removes its owned timer bridge after Windows returns.

MIDI uses the Tandy voices and basic percussion. VGM/VGZ preserves the supported
PSG register stream. MOD follows supported tracker pitch, volume and effects
using square-wave tones; it does not reproduce sample timbres.
[Supported formats](docs/FORMATS.MD) Â· [Playback timing](docs/SNDTIME.MD)

## Make five-voice music

**Fiddle and Beats** add an optional **PC-speaker square-wave voice** alongside
the PSG's **three tones and noise**. Write a fifth `[P]` part in MML3, or edit
and export the fifth lane in Beats.

**[Download MML3AUTH.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/sound-exp-v0.1.0-pit.20261007/MML3AUTH.ZIP)**
 Â· [Authoring and setup](experimental/PITPUBLIC/INSTALL.TXT)

For the other instruments and DOS sound test, use [current sound downloads](docs/DOWNLOADS.MD).

## Build your own Windows music app

**[libwintandy](https://github.com/astrobleem/libwintandy)** shares the 8086
Windows MIDI services, prepared-frame/ordered-burst playback backend and USER
UI helpers. Its VGM, gzip and MOD readers and foreground integration are
available as source with working examples.

Start with **[FORMATS30](https://github.com/astrobleem/libwintandy/tree/24ff4f932c0d4d4e2a18a3c49b0ea0a8ff90e71f/examples/FORMATS30)**,
then follow the [build guide](experimental/WINAMP30/BUILDING-UI.MD) and
[API/format boundaries](docs/FORMATS.MD#reuse-and-evidence). The playlist and
Browse/Add/Remove UI belong to Winamp; `TPLAY` is its guarded DOS launcher.

For separate source-only DOS and portable C89 Tandy components, see
[libtandy](https://github.com/astrobleem/libtandy). It covers SN76489-family PSG
state, graphics, surfaces, input and cooperative timing; the Windows driver and
apps here remain separate projects.

## More to play with

- **[Mini Piano](src/win30/PIANO/README.MD)** â€” play notes with the keyboard or mouse.
- **[JOYMIDI](src/win30/JOYMIDI/README.MD)** â€” turn a joystick into a pitch/velocity instrument.
- **[Beats Lab](artifacts/win30/BEATS/README.MD)** â€” build patterns on a four-lane step sequencer.
- **[Buddy Holly: BUDCAP video with PSG music and timed captions](https://youtu.be/wzUQWxt3Eno)** â€” an offline export. [BUDCAP 0.4 docs](examples/BUDDY/BUDCAP/README.md) describe the 256Ã—160 DOS video at 4 fps. [Watch the genuine emulator demo GIF](docs/SHOTS/BUDCAP04.GIF)
  Â· [Capture details](docs/SHOTS/README.MD#budcap-04-animated-preview).
- **[Blondie's "Call Me" five-channel oscilloscope](https://youtu.be/gpdcfCYBdpk)** â€” an offline visualization
  of the Tandy PSG and PC-speaker arrangement.

See [validation](experimental/WINAMP30/VALIDATION-UI.MD),
[checksums](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-xt1.20261024/SHA256SUMS)
and component notices for exact versions and scope. Winamp and the
[Windows XT display project](https://github.com/astrobleem/oemdisplay-tandy)
have separate release gates.

## Licensing

Keep the [root license](LICENSE) and component-specific notices. Winamp/shared
source and the first-party browser retain their MIT notices; TSOUND has its own
license. The Winamp package includes no private/commercial music, sample media,
Windows, ROMs or proprietary tools. BUDCAP's existing media has separate rights
and is not included in the Winamp download.
