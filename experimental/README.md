# Experimental sound test kits, 5 October 2026

These are **draft, emulator-qualified test builds**. They are not stable
replacements or physical Tandy 1000 / 4.77 MHz 8088 qualification.
The main README, stable builds, baseline source and rollback paths remain
unchanged. Keep existing installations intact and use separate DOS folders.

## Windows 3.0: WININST12

[Download WININST12.zip](WININST12.zip?raw=1), extract it, and read
[WININST/README.TXT](WININST12/README.TXT) before using `WININST/RUNTIME`.
The ZIP includes matching source, build scripts, original MIDI/audio
fixtures, host checks, and accepted emulator evidence.

The paired app-local mapper adds eight original melodic envelope families,
three noise shapes, per-channel Program Change, and optional preset vibrato.
MiniMIDI, Mini Piano, JOYMIDI, and Beats retain their original controls.
Keep MIDIMAP.DRV beside these clients. Do not install it into WINDOWS/SYSTEM,
replace SOUND.DRV, change SYSTEM.INI, or run competing direct PSG writers.
Close the trial apps and return to the untouched original folder to roll back.

Windows cooperative timers can stall during drawing or a non-yielding task.
Measured hostile-test service gaps reached 5.71 seconds. Recovery is bounded,
but the held sound is stale until the owner runs again. Read the
[validation report](WININST12/VALIDATE.MD) and [API/timing detail](WININST12/INSTR.MD).
The four clients do not all gain instrument-selector UI; MiniMIDI can use
Program Change from files, while Piano/JOYMIDI/Beats use their default presets.

## DOS Buddy: BUDENV02 selected short opening

[Download BUDENV02-OVERLAY.zip](BUDENV02-OVERLAY.zip?raw=1). This contains the
selected 0.28–3.30 second opening cue plus the expressive main arrangement.
It does not contain the later long-tail alternative.

This is a **media-free overlay**, with matching C89/MASM source, arrangement
scripts, regression tests and evidence. It does not duplicate the baseline
video, speech PCM, captions, or cue. Supply the exact existing
[BUDCAP04.ZIP](../examples/BUDDY/BUDCAP04.ZIP) to the included preparation script:

    python BUDENV02/HOST/PREPARE.PY BUDCAP04.ZIP /path/to/new/BUDENV02

The destination must not exist. PREPARE validates the baseline ZIP and
individual media hashes, then copies the selected executable/score and the
unchanged baseline media into that fresh folder. Run RUN.BAT there in plain
DOS, outside Windows and sound/timer TSRs. Stop with Escape, Space, or Ctrl+C.
Rollback is returning to the untouched BUDCAP04 directory.

See [run instructions](BUDENV02/README.TXT), [reproduction](BUDENV02/REPRODUCE.TXT),
[QA limits](BUDENV02/QA.TXT), and [third-party attribution](BUDENV02/ATTRIB.TXT).
The existing speech whine remains unresolved. Emulator budgets are not MHz.
The opening is an approximate chord-tone reduction, not a verified vocal
transcription. The 3.30 second endpoint is the selected short excerpt.

## Scope, provenance and verification

- Both ZIPs contain corresponding source and all original code-license notices.
- Third-party music, score-derived material and baseline media keep their
  original provenance. The code license grants no rights to underlying works.
- No Microsoft OS/compiler/SDK, emulator, original video/PDF/audio excerpts,
  TMNT game assets, credentials, user documents, or machine images are added.
- [PUBLICATION.json](PUBLICATION.json) records exact runtime and input identities
  and the publication rechecks. [SHA256SUMS](SHA256SUMS) verifies both ZIPs.
- The kits' historical evidence is distinguished from fresh publication checks.
  Host tests and scoped ISA audits do not establish physical sound quality,
  electrical behavior, hardware joystick timing, or cycle-calibrated CPU cost.

No system installation, auto-start change, CF write, or merge is performed by
these kits. Supply lawful external period tools and Windows for native builds.
