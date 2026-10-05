BUDTALK 0.4 - CORRECTED FISH + FULL-VIDEO SUBTITLES

Extract the new BUDCAP folder beside your previous versions. In plain DOS,
CD into BUDCAP and run RUN (or RUNFULL). BUDTALK with no options also
plays the complete movie with subtitles.

RUN / RUNFULL - entire movie, full supplied subtitles and short speech.
RUNTEST - 18-42 second opening preview, then the first music passage.
RUNSONG - song-only range with subtitles; no sampled speech in that range.
NOLYRICS - entire movie and short speech, with subtitles disabled.
Escape, Space or Ctrl+C stops. Relaunch to restart.

CORRECTED SPEECH
The old 19.8-23.8 second excerpt was the introduction, not the fish line.
It is removed. You confirmed both complete words in the new source edit:
23.900-24.800  Weezer! (0.90 seconds)
28.500-31.950  Oh, hey, hey, please try the fish. (3.45 seconds)
The applause-only gap is omitted from sampled audio. These clips keep
original movie timestamps; the video continues through the gap.

The short closing clips remain:
219.950-224.700  closing fish-question / lead-in (4.75 seconds)
225.150-226.650  closing reply (1.50 seconds)
Closing clip boundaries have not had the same word-exact listening check.
There are four clips, 10.60 seconds / 63,600 PCM bytes total.

SUBTITLES THROUGH THE WHOLE VIDEO
The full supplied transcription is now included: opening announcements,
introductions, fish line, count-in, all song lyrics, middle Happy Days
bumper, closing exchange, and the supplied cheering/laughter/music labels.
Text stays in the bottom border. Song phrase timing is preserved; dialogue
is approximate except for the corrected opening speech-window anchors.
Long sentences appear in smaller readable phrases, without karaoke effects.
See SUBTITLE.TXT for timing/format details.

Subtitles now change inside speech clips as well as during the music.
At a text boundary, the player briefly stops PCM, draws the new caption,
and catches the speech sample position up to the unchanged movie clock.
A short interruption is possible at that boundary. The picture still holds
through each voice clip (maximum 4.75 seconds), then catches back up.

SOUND LIMIT
The reported coil-like whine remains unresolved. PC-speaker PWM is still
6 kHz; this revision fixes the selected words and caption coverage.
Any EQ-processed listening demo uses offline filters that do not run in
DOS. It does not establish how clear or noisy the physical EX will sound.

SAFETY / RESOURCES
Original BUDSWEET video (256x160, 4 fps), PSG score and movie clock are
preserved. No speech or subtitle file reads occur during playback.
PIT 0 / BIOS timer IRQ are unchanged; no new interrupt hooks are installed.
Speech temporarily uses PIT 2 / the speaker and restores the inactive state.
Music keeps priority; late heads may be skipped, and a deadline guard may
shorten a clip rather than delay the PSG music. BUDTALK.LOG records this,
including the build marker BUDCAP04, intentional picture holds, captions,
and cleanup. Use DOS 3+ on Tandy 1000, fully outside Windows and without
sound/PIT TSRs. No installer or AUTOEXEC/CONFIG changes are needed.

Stop and handled errors silence speaker/PSG, restore previous display mode
(not screen contents), free buffers, close files and restore DOS handlers.
Missing/malformed subtitle files disable captions without stopping playback.
This remains a private test candidate. Emulator budgets are not MHz or a
substitute for physical hardware listening. Source is included; compiler
is not. QA.TXT and VERIFY contain final test evidence.
