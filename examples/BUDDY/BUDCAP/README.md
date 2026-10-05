# BUDCAP 0.4: full-video captions and corrected speech

The accepted BUDCAP04 runtime is preserved byte-for-byte in
[`../BUDCAP04.ZIP`](../BUDCAP04.ZIP). Extract it to a **new directory**; the ZIP
contains its own `BUDCAP/` folder with all 33 runtime/source/evidence files.
This repository directory contains the exact source, caption track, original
notes and verification evidence, plus portable host-side reproduction tools.
Large runtime assets are stored only in the ZIP.

Release ZIP SHA-256:
`0ea79bf57992e408115c9b4daeb36daee224a0ea56a36dd0b1fcdc2d0ef8b75d`

`BUDTALK.EXE`: 20,155 bytes, SHA-256
`2e866172774626deec2c699ba7df7738671feb9357e752977429c5aebbb38986`.
The original `SHA256.TXT` describes files **inside the extracted runtime**;
it is not a manifest of the smaller source directory on GitHub.

## Run

Use DOS 3+ on a Tandy 1000, outside Windows and without sound/PIT TSRs.
Change into the extracted `BUDCAP` folder and run:

- `RUN` or `RUNFULL`: full 241.4-second movie with subtitles and short speech
- `RUNTEST`: 18–42-second opening preview
- `RUNSONG`: song-only range with subtitles
- `NOLYRICS`: full movie with speech, no subtitles

No options (`BUDTALK`) selects full playback with subtitles. Escape, Space or
Ctrl+C stops; relaunch to restart. No installer or system configuration edits.
For emulation, use DOSBox-X with Tandy machine and `8086_prefetch` CPU; the
host test script generates a 640 KB, no-XMS/EMS/UMB configuration.

## What this version changes

84 subtitle cues cover the supplied full-video transcription, including all
42 existing sung phrases with their original timing. Corrected opening speech
windows are 23.900–24.800 seconds (Weezer) and 28.500–31.950 seconds (fish line).
The prior mistaken 19.8-second introduction excerpt is removed. Two closing
clips remain; all four total 63,600 samples / 10.60 seconds at 6 kHz.
Video is 256×160 at 4 fps. Video, PSG music and CUE assets remain unchanged
from the accepted BUDSWEET baseline (not the earlier 128×96 DOSPLAY package). Subtitles advance
inside speech clips; the video intentionally holds during each clip, then
catches up to the movie clock.

## Evidence and limits

[`QA.TXT`](QA.TXT) and [`VERIFY/`](VERIFY/) preserve the accepted package's
full-run logs at fixed 240 and 3000 emulator budgets, an opening-caption
screenshot, stop/error tests, and the scoped 8088 ISA audit. Both archived
full runs complete all 84 captions and four speech clips with no unexpected
video drops or skipped music events. Emulator budgets are not MHz.

Publication reproduction rebuilt the EXE byte-identically using Microsoft C 6,
MASM 5.1 and DOSBox-X. See [`REPRODUCE.md`](REPRODUCE.md).

PC-speaker PWM whine remains unresolved. Speech temporarily holds the image
(maximum 4.75 seconds); caption changes can briefly interrupt speech. Spoken
subtitle timing is approximate, and closing clips lack word-exact listening
confirmation. Physical Tandy EX playback and Windows rejection were not newly
tested. This is still a test candidate, not a hardware-certification claim.
No offline EQ is applied by the DOS player. No original AVI, PDF, full WAV,
compiler or assembler is redistributed by this addition.
