# Effects in Beats

Beats imports the editable TFX1 `.FXP` format from Effects Lab. Patch parameters
and seed are embedded in DWA2 `.BTS` songs, together with five lane assignments,
40 trigger flags, note/transposition values, tempo, programs and PIT enable.
Loads validate everything before replacing the edit. The existing backup and
recovery save path is retained. Songs without imported patches keep the original
DWA1 bytes; DWA1 songs load with no patch slots. C headers remain a programmer
export for the existing `FXPLAY.H` adapter, rather than an app import format.

Tone patches fit rows1..3, noise patches row4, and speaker patches row5.
An FX cell's MIDI note60 (C4) means the original imported trajectory. Other notes
transpose period/divisor by semitone, with hardware clamping. Slide and vibrato
transpose with the trajectory; fixed-rate noise ignores the note. New triggers
replace an effect in the same lane. A rest does not cut an already triggered
effect: it finishes its envelope unless retriggered or Stop is pressed.

`BEATPLAY.C` snapshots the edit at Play and compiles all assigned patches with
the unchanged `FXCompile`. It prepares each merged 60Hz state in task context.
Normal notes use the existing `INSTR.C` curves, programs and vibrato, with
75 percent gate, sampled on that same grid. A step boundary is the first 60Hz
sample at or after its exact eighth-note deadline. `BBStamp` uses rational PIT
cycles without accumulated rounding drift or 32-bit intermediate overflow.
Edits affect the next Play. The eight-step song loops until stopped. The old
25-minute fail-silent timestamp guard was artificial. Beats now periodically
subtracts a phase-aligned four-minute origin from its producer and RAWCLK's
logical timestamps before that guard. TCBRebase is additive ordinal37: under
one IRQ lock it advances a shared DWORD burst origin and adjusts logical clock
fields, leaving queued physical stamps, timer epoch and lastStamp untouched.
There is no queue walk, restart or sound write. Normal frame-mode consumers
retain their original behavior and ordinals1-36. Modulo32 physical stamps can
wrap while the song remains within the small logical horizon.

Portable `FXMERGE.C/H` is canonical in libtandy `sound/`, with identical copies
in libwintandy's Windows AUDIO/SOURCE and OEMSound's engine mirror. It combines
normal voices and all effect slots before any output. Tone effects replace only
their row. Pitched noise owns noise plus tone2, keeping tone2 audibly muted even
on gated rows. Tone2's hidden note/effect continues advancing and its current
state returns when borrowing ends. Fixed noise leaves tone2 alone. Speaker FX
replace the song's current speaker divisor, then restore its current note.
Noise control is emitted only for a real attack, mode change or restore;
level-only rows do not reset the LFSR. `FXBytes` is never called by Beats.

Beats owns one RAWCLK session. It opens TC[B/X], prepares512 bursts
(8.5 seconds) in16-state foreground batches, then starts the existing
timer/bridge. Input is checked between batches, so Stop/Esc, deactivate and
close can cancel preparation before the epoch. Cancellation clears the job
even if transport close refuses and needs a retry. At full lookahead, one
settling Yield and a return precede the next turn's ownership/PIT recheck and
clock start; the servicing guard remains set across that Yield. A known-initialized producer
reuses compiled clips only when all loaded patch parameters and seeds match;
notes and tempo can change independently. Cold compilation still runs in the
foreground. Expensive dirty-cell text drawing waits until the audio buffer is
ready, so selecting or editing a pattern immediately before Play does not
make grid rendering compete with preparation. Status painting and input remain
active; dirty cells are retained and resume drawing after start or cancellation. The main PeekMessage loop uses PM_NOYIELD while active and restores
lookahead before cooperative yields. This retains the full buffer; it does
not pretend that batching removes the8088 CPU cost. The moving play marks update five one-pixel strips rather than
repainting40 text cells. Status updates invalidate only the status line.
Full grid exposes mark a five-byte pending-cell mask. The message loop draws
one current cell per turn and checks pending input before rendering the next.
It also skips unchanged button/caption text. Status-only paint does not restart
the grid; no self-posted feeder or new timer is used. It services audio while WM_TIMER is delayed.
GUI timer timing never controls note deadlines. RAWCLK emits only
prepared bytes/PIT values, with its existing bounded IRQ path. No additional
driver, MIDI owner, timer, callbacks into the app, PCM or consumer preparation
is introduced. An exhausted queue or timing error fails silent and releases
on the next foreground service. The speaker is exclusive: unavailable/busy
PIT refuses Play; losing that lease later stops the song. The existing
bridge may temporarily suspend for DOS; reentry waits for idle quiescence and
precedes the busy check. The old app-local
mapper, Fiddle, MIDI banks and SOUND driver sources are unchanged.

Pause uses TCPause/TCUnpause, preserving cursor and merged PSG/PIT state.
Stop, deactivate, cancel, close and normal session exit release ownership.
Close refusal leaves the app resident for a Stop retry. The native acceptance
also blocks the GUI briefly and verifies that RAWCLK still advances sound.

Build with the existing guarded `BUILD.PY`. Its additional output is
`BTSTART.EXE`, the normal DOS bridge/Windows launcher specialized for Beats.
Use `GO.BAT` from the complete kit. Direct Windows launch works only when the
normal timer bridge is already active. The older `PACK.PY` is retained for
historical authoring-release verification; new integration builds use
`FXPACK.PY --build ... --raw-build ... --raw-host ... --libwintandy ...
--host ... --clock ... --native ... --out ...`.
The matching backend is built with guarded
`experimental/WINLIB30/SOURCE/BLDRAW.PY`, using the same toolchain/lease
arguments. The kit contains this source-pinned RAWCLK and its build receipt;
the original SOUND driver, bridge and Setup remain unchanged. An already
loaded older RAWCLK cannot supply ordinal37; Beats gives a restart-Windows
message. Starting with GO from DOS naturally loads the matching module. It includes both
editors, launchers, example patches, the mixed demo, shared Setup, source and
source-pinned acceptance. It never publishes.

Focused checks: `FXTEST.PY --out <new-output>` exercises the compiler, portable
merger, transposition, delta decoder, embedded songs, legacy load and tempo
boundaries. `FXCLOCK.PY --libwintandy <root> --out <new-output>` reproduces the observed
native empty-queue error4, then verifies30 seconds of actual producer/RAWCLK
streaming with no GUI timer, a350ms foreground gap and pause/resume.
It also conservatively models20 seconds of grid paint across40 cells,
including5.5ms of producer CPU per frame and bounded refills between cells.
`HOSTTEST.PY` retains all five original demo/MML, transactional
save, current/archived mapper and actual PIT core regressions. `FXNATIVE.PY`
requires `--raw-build` plus private licensed fixture paths and reuses the shared emulator lease,
process inventory, private disk, bounded audio and owned cleanup. Its developer
`--play-only` iteration mode returns DIAGNOSTIC_PASS, which cannot qualify
consumer packaging. The full UI/playback/recording run remains required.
`FXAUDIO.PY --native <run>` verifies sustained PSG and speaker activity in three
parts of a playback interval exceeding11 seconds, resumed activity, and pause/stop silence.
`AUDTEST.PY --out <new-output>` checks the synthetic audio/onset gates.
`FXPACK.PY` requires that audio receipt and verifies its recording hashes.
Host checks
alone do not establish Win16 ABI, memory use, native timing or audible quality.

The focused rolling/cache regressions compare exact merged bytes, PIT values
and deadlines across rebases at tempos99/100/240, including pitched-noise
borrowing and restoration. The actual RAWCLK host model runs three virtual
hours with44 rebases and physical DWORD wrap, checks partial/full queue and
pending FULL retry, pause and invalid-owner/frame calls, with no added sound
writes at a rebase. Native test mode uses a phase-aligned shorter interval to
exercise the same API promptly; ordinary playback uses the four-minute shift.

Windows3.0 GetInputState includes timer events in the system queue. With the
30ms UI timer it cannot gate cooperative Yield while PM_NOYIELD is active.
The loop now yields at its audio high-water boundary regardless of timer/input
state, retaining its Peek-before-cell ordering. Primary reference:
https://www.pcjs.org/documents/books/mspl13/win/w3sdkr1/
The --visual-only native and --diagnostic audio options are development probes;
they produce DIAGNOSTIC_PASS and cannot qualify FXPACK.

The response validator measures actual exact-zero onset within2 captured seconds
and requires at least0.5s continued zero samples, confirmed by2.5 wall seconds.
It retains the original fixed-window observations, which may start before mute.
Ten synthetic fixtures include within-deadline overlap, late-response and
short-zero-tail cases. Request screenshots are not settled-state evidence.
