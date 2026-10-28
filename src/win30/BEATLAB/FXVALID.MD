# Integration validation

Build19 passed full native19 qualification:65 scripted checks, normal launcher,
physical controls, recorded hardware audio and normal DOS return.
BEATS.EXE is51,845 bytes, SHA256
91d46ebd0da787fc8c4ecc3fc7ddd0da785efb61ba91d7b0545490ec2849aec7.
NE CODE minimum42,448 and DGROUP36,720 bytes; declared heap2048/stack4096
bring near demand to42,864 with22,672 bytes below64KB. MIDIMAP, Fiddle and
BTSTART still match the previously qualified bytes.

The focused native18 trace probe returned normal DOS and passed16 audio checks:
cold onset6.805/6.811 seconds (PSG/speaker), warm3.570/3.582 seconds, and exactly
zero Pause/Stop samples in windows beginning1.32/1.33 wall seconds after forced
repaint/control requests. This is DIAGNOSTIC_PASS, not consumer qualification.
Full native19 repeated scripted/physical/audio acceptance with tracing disabled
and passed16 audio checks. Actual cold onset6.240/6.241 seconds and warm
3.726/3.727 seconds (PSG/speaker) followed immediate Demo-to-Play. The heavy
scripted workflow measured15.324s preparation; this is not every Play.
No publication is authorized or performed.

The artificial25-minute stop was the90000-state safety guard. Normal playback
now rolls its logical origin before reaching that guard. The producer shift
preserves every supported tempo's eight-step phase and exact rational60Hz
PIT deadlines. RAWCLK additive ordinal37 adjusts one burst origin and logical
clock fields under an IRQ lock; queued physical DWORD stamps, physical timer
epoch/lastStamp, PSG/PIT state and timer ownership remain intact. No ring walk,
sound write, clock restart or second sound owner is introduced.

The earlier approximately20-second preparation figure followed the heavy
import/export/MML/clipboard/redraw workflow. Investigation also reproduced
30-second preparation when queued cell painting competed with buffer filling;
that can affect ordinary editing followed immediately by Play. The candidate
therefore defers cell text while preparing, retains dirty bits, paints status,
and polls input between16-state batches. It keeps the complete512-frame
lookahead (8.533 seconds), a guarded settling Yield and a subsequent input turn
before starting the clock. Cancellation remains cancelled if close needs a
retry. Identical loaded patch parameters/seeds reuse compiled trajectories.
Final ordinary cold/warm onset is measured from captured samples after the
immediate-Demo Play request and verified again in the full run.

RAWCLK compiled with MSC6/MASM/LINK4: file15,976 bytes, SHA256
2e0a3658bc7582a1965c71fd755df35c38e658e5fbadb568fcc61cb8e6eefe8c.
NE fixed CODE minimum14,301 and DGROUP27,172 bytes: +501 code/+4 data against
the qualified MIDI8 baseline. Existing ordinals1-36, queue capacities, original
SOUND driver, MIDI banks, mapper, Fiddle and bridge/launcher bytes are retained.
The new kit supplies the matching DLL beside the apps; an older module already
loaded in Windows requires closing older players and restarting Windows.

Current passing host checks:

- FXTEST:67,401 compiler/merger/producer/codec checks, including exact warm/cold
  restart traces and exact bytes/PIT/deadlines across rolling at99/100/240BPM,
  with pitched noise borrowing and returning tone2.35 noise controls over1200
  states, rather than a control write for every attenuation change.
- FXCLOCK:17,195 actual-producer/RAWCLK checks. It reproduces underrun4 without
  replenishment, then verifies continuous refill,350ms foreground gap, pause,
  and conservative20-second cell painting with5.5ms producer cost per state.
- RAWCLK:328,160 checks in the new rolling test, three virtual hours,44 rolls,
  physical DWORD wrap, partial/full ring and pending FULL retry, pause, invalid
  owner/frame mode, and no additional sound I/O at a roll. Existing burst,
  speaker, paired-speaker and pure merger regressions also pass, with zero
  unsafe IRQ calls under mocks.
- Ten synthetic audio/onset-gate cases accept sustained responsive playback
  and reject silence, startup-only/short recordings, slow wall/guest response,
  missing warm onset, silence after2s and less than0.5s zero-tail corroboration.
  A response within2s is accepted even when it overlaps the earlier fixed
  observation-window start.
- Unchanged-source baseline regressions: five original demo/MML/file/PIT and
  current/archived mapper checks;144 intended demo attacks; Effects compiler
  177,682 checks/27,279 states;32 authoring-package checks; portable Default
  trace70D11B5D,6634 events/9798 renders/154 note audition contributions.

Native testing uses an emulated Tandy HX8088/7.16MHz/640KB/Windows3.0 real mode,
320x200, fresh private setup and the shared byte lease. The full acceptance
requires normal launcher/UI, actual import and embedded-song persistence,
real rolling API, cold/partial/full-boundary cancellation including a refused
close,350ms without GUI service, pause/resume/Stop/replay/deactivate/failure/
close, normal DOS return, and owned-child stop/lease release/inventory0.
The visible phase selects the final Demo and immediately presses Play while
its grid is pending, measures cold/warm onset, records >11 emulated seconds
beyond the startup horizon, and forces repaint before F6/Escape. PSG48kHz and
speaker49,716Hz signed16 recordings must be active in three sections/tail and
resume/warm replay, and silent during Pause/Stop. Repaint-to-silent-window
bounds stay <=2 captured seconds to exact-zero onset, with at least0.5s
continued exact-zero samples and confirmation ending within2.5 wall seconds.
The actual full-run Pause onsets were1.736979/1.738072s and Stop0.991646/
0.951484s; zero tails were>=0.595s and>=1.348s. The old fixed1.33s window
started before Pause completed and rejected it; that failure is preserved,
not presented as silent. Active-segment analysis trims at most0.25s. These are observation bounds, not exact key-
handler timing. No physical-hardware or subjective listening claim is made.

Independent source review resolved cancellation on refused close, final
full-buffer settling, the duplicate WM_TIMER preparation call, and pending
cell drawing competing with preparation. Final ZIP review is delivered separately after packaging.
FXPACK binds37 staged app inputs,11 staged RAW inputs, native runtime hashes,
current host/RAW/clock sources, full native/audio PASS and recording hashes.
DIAGNOSTIC_PASS cannot qualify a consumer kit. Both Pause/Stop-request PNGs precede their status repaint; action marks and
recorded onset/tail measurements are the silence evidence.

The physical-control probes found late message handling while cells were dirty.
Windows3.0 GetInputState includes timer events, so it cannot suppress cooperative
Yield with a30ms UI timer. The loop now consumes actual keyboard messages before
general/timer messages and rechecks before another cell or optional Yield.
Paint rechecks window/preparation/dirty state after PeekMessage reentry. A fixed
32-event developer RAM trace distinguishes dequeuing from action entry/exit;
its flag file is absent in consumer/full qualification and it writes only after
sound is released. Diagnostic modes remain rejected by FXPACK.
Primary API reference: https://www.pcjs.org/documents/books/mspl13/win/w3sdkr1/
