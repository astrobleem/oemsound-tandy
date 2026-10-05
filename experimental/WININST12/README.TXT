WININST 1.2 - EXPERIMENTAL WINDOWS 3.0 INSTRUMENT TEST
5 October 2026

The app-local mapper now has eight original melodic envelope families,
three noise-drum shapes, optional vibrato and per-channel Program Change.
It still uses exactly three tone voices plus one noise voice.

This is a experimental test kit, not a system MIDI installation or
hardware-qualified release. Windows' cooperative timing can interrupt the
shaping during slow redraws or a non-yielding task. The evidence includes
both safe recovery and the measured timing limits. Read VALIDATE.MD.

TRY IT IN AN ISOLATED COPY FIRST

1. Keep your existing installation and original files untouched.
2. Copy the contents of RUNTIME to a new DOS directory such as C:\MINSTR.
3. Start Windows 3.0 in REAL MODE with your existing Tandy display setup.
4. From Program Manager / File / Run, launch C:\MINSTR\MINIMIDI.EXE.
5. TINY.MID is an original short test with mid-song program changes. Press
   Play. PRESETS.MID is the same supplied test under a descriptive filename.
6. MINIPIAN.EXE, JOYMIDI.EXE and BEATS.EXE preserve their original controls.
   Their paired versions enable the instrument layer. Piano and JOYMIDI use
   the default Keys preset; Beats uses Keys plus expiring noise percussion.
   They do not gain preset-selection controls in this build.
7. Use Stop/Panic or close the current sound owner before changing apps.

Keep MIDIMAP.DRV beside the EXE files. Do not copy it into WINDOWS\SYSTEM,
register it in SYSTEM.INI, replace SOUND.DRV, or run two direct PSG writers.
There is no installer and no auto-start change. To roll back, close these
apps and return to your untouched original application directory.

The old public Beats EXE was also tested with the new mapper. It remains in
legacy rectangular mode unless the client explicitly enables the extension.
New instrument clients require the paired version 1.2 mapper.

WHAT IS INCLUDED

- RUNTIME: paired experimental clients, MIDIMAP.DRV, original MIDI fixture, and
  test executables. INSTEST/MIDITEST/MIDIINTR are automated QA programs,
  not normal desktop apps. They expect C:\ paths and exit the test Windows
  session; use QA/RUNWIN.PY with a disposable guest for them.
- SOURCE: mapper, instrument core, clients, C6 build scripts and tests.
- SOURCE/BEATS/INSTR.MD: exact private API, preset mapping and limitations.
- AUDIO: lossless digital Tandy mixer recordings in legacy, envelopes-only,
  and envelopes-plus-vibrato modes. The same new mapper and original notes
  are used for all three, with equal gain and no loudness normalization.
- QA: host tests, scoped 8088 audits, exact native event checks, screenshots
  and a self-contained disposable-guest runner.
- EVIDENCE: 23 accepted native-run receipts and compiler/link logs/maps.
- MANIFEST.json and SHA256SUMS: exact packaged runtime and file identities.

No Windows binaries, Microsoft compiler/libraries, game music/code/bank,
user-computer files, credentials, emulator binary or display driver is
included. Supply lawful Windows/toolchain inputs and your display driver.
The kit is published as an experimental draft. No system-driver replacement,
CF write or user-computer change was performed. Stable builds are unchanged.
