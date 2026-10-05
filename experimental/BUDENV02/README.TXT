BUDENV02 - EXPRESSIVE BUDDY HOLLY A/B CANDIDATE
5 October 2026

This is a separate experimental DOS overlay. Prepare BUDENV02 beside BUDCAP04.
First run HOST/PREPARE.PY with the original BUDCAP04.ZIP and a NEW output
directory, as described in REPRODUCE.TXT. The overlay omits baseline media.
Do not overwrite the accepted BUDCAP04 folder. From plain DOS in BUDENV02,
run RUN for the entire movie. RUNSONG plays the song range; RUNTEST plays
the 18-42 second speech/opening preview. Escape, Space, or Ctrl+C stops.

OPENING-CUE REVISION
This version keeps the approved BUDENV01 music exactly after the opening.
It adds a three-tone approximation of the opening doo-wop harmony at
0.28-3.30 seconds: A-flat major, then B-flat major, with a soft swelling
instrument. No opening speech sample or noise percussion is added.
Stable pitch families support the two chords; the announcer and overlapping
harmonics obscure exact singer octaves and vowel reattacks. The three-voice
reduction is approximate, and 3.30 seconds is a chosen excerpt endpoint,
not a verified end of all music. See OPENING.TXT and the source evidence.

BUDENV01 MUSIC (PRESERVED)
The DOS player now shapes all three tone voices and the one noise voice.
Eleven newly authored instruments include rounded verse lead, forward
chorus lead, picked solo, swelling bridge lead, bass, tight rhythm guitar,
sustained harmony, guitar response, and kick/snare/hat noise envelopes.
An 81-entry map changes the assigned instrument by section or note role.
The new role is selected on the next note attack; other held voices keep
their current envelope. Tone pitch does not wobble in this first candidate.

The phrase-aware arrangement restores 15 explicitly written solo/backing
notes and keeps 15 existing outro harmony notes. Generic guitar pulses
are lighter beneath verse melody and stronger in genuine written rests.
Some verse/solo hats are removed or softened to leave room. These are
source-grounded changes to the supplied reduction, not a fresh audio
transcription of unwritten guitar parts. Arrangement source and per-note
evidence are in arrangement/.

The original melody and bass notes, timing, velocity, and register remain
exact. Video, captions, speech, cue bytes, and the 241.4-second movie clock
are unchanged. There are still only three tone oscillators plus noise.

TIMING AND HARDWARE
Envelope steps use the existing BIOS tick (~18.2065 Hz / 54.925 ms), not
TMNT's faster 100 Hz interrupt system. This deliberately coarse first
version adds no PIT programming, timer/IRQ ownership, Windows callback,
PC-speaker bass, or extra voices. The original speech still uses PIT 2
and PC-speaker PWM. Run outside Windows and sound/timer TSRs.

Updates inspect four fixed-size voice states and emit at most four changed
volume registers per tick. No file reads, allocation, pitch calculation,
or unbounded backlog of envelope ticks occur in that update. Overdue
score records merge each voice's latest onset/profile and restore its age;
old notes are not played in a burst. Instrument note-off never adds a tail
past the scored stop or a dialogue gate. Cleanup mutes all PSG channels,
stops the speaker, and restores the previous display mode.

LISTENING LIMIT
Historical A/B WAVs were lossless emulator captures with constant gain only. No EQ,
denoising, whine removal, compression, reverb, or other audio enhancement
is applied. Loudness matching was a preview operation, not a DOS feature.
Those reference recordings are not included in this public overlay.
The original speaker whine remains unresolved. These tests do not prove
physical Tandy 1000 EX sound quality, CPU speed, or electrical behavior.
The emulator cycle budgets are not MHz and have not been calibrated to
a physical 4.77 MHz 8088. Decide whether the new arrangement sounds better
by listening; passing tests establishes compatibility, not musical taste.

SOURCE
SOURCE contains full C89/MASM8086 source. HOST/BUILD.PY takes an existing
MSC6 toolchain root and DOSBox-X path; no compiler is bundled. See
REPRODUCE.TXT and QA.TXT for commands, exact hashes, tests and limits.
