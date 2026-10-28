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
Edits affect the next Play. This is an eight-step looping song; RAWCLK's bounded
literal-burst horizon ends a session with explicit silence at 25 minutes.

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
(8.5 seconds), then starts the existing timer/bridge. The main PeekMessage
loop checks messages between16-state preparation batches. It uses PM_NOYIELD
and restores lookahead before yielding, also yielding while the finite final
queue drains. The moving play marks update five one-pixel strips rather than
repainting40 text cells. Status updates invalidate only the status line.
Full grid exposes batch text into five row calls and replenish prepared audio
between rows. It services audio while WM_TIMER is delayed.
GUI timer timing never controls note deadlines. RAWCLK emits only
prepared bytes/PIT values, with its existing bounded IRQ path. No additional
driver, MIDI owner, timer, callbacks into the app, PCM or consumer preparation
is introduced. An exhausted queue or timing error fails silent and releases
on the next foreground service. The speaker is exclusive: unavailable/busy
PIT refuses Play; losing that lease later stops the song. The existing
bridge may temporarily suspend for DOS; reentry precedes the busy check. The old app-local
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
`FXPACK.PY --build ... --host ... --clock ... --native ... --out ...`. It includes both
editors, launchers, example patches, the mixed demo, shared Setup, source and
source-pinned acceptance. It never publishes.

Focused checks: `FXTEST.PY --out <new-output>` exercises the compiler, portable
merger, transposition, delta decoder, embedded songs, legacy load and tempo
boundaries. `FXCLOCK.PY --libwintandy <root> --out <new-output>` reproduces the observed
native empty-queue error4, then verifies30 seconds of actual producer/RAWCLK
streaming with no GUI timer, a350ms foreground gap and pause/resume.
It also models the observed9.5-second full-paint delay across five rows,
including5.5ms of producer CPU per frame and bounded refills between rows.
`HOSTTEST.PY` retains all five original demo/MML, transactional
save, current/archived mapper and actual PIT core regressions. `FXNATIVE.PY`
accepts private licensed fixture paths and reuses the shared emulator lease,
process inventory, private disk, bounded audio and owned cleanup.
`FXAUDIO.PY --native <run>` verifies sustained PSG and speaker activity in three
parts of a12-second playback interval, resumed activity, and pause/stop silence.
`FXPACK.PY` requires that audio receipt and verifies its recording hashes.
Host checks
alone do not establish Win16 ABI, memory use, native timing or audible quality.
