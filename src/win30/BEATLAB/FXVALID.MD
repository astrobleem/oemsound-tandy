# Integration validation status

The Effects integration and foreground-refill correction are implemented.
The preceding candidate compiles with MSC6 /G0 and LINK4 for Windows3.0
real mode. The latest refill/repaint source passes a Win16 syntax check; its
MSC6 rebuild and full native acceptance are pending the shared emulator slot.
Do not publish or describe the candidate as native-qualified yet.

Passing host checks include:

- FXTEST: 18,054 compiler/merger/producer/codec checks; 35 noise-latch writes
  across 1,200 rendered states, rather than a reset on every level update.
- FXCLOCK: 12,573 checks using the actual producer and actual RAWCLK with the
  library's existing Win16/port mocks. The test reproduces native underrun 4
  when a 512-frame queue is never replenished. Foreground refill then runs
  1,800 frames without WM_TIMER, including a 350ms foreground gap and pause.
- Original five-demo pattern/MML and transactional-file tests, actual PIT
  bounds/service checks, current/archived mapper regression and 144 intended
  original demo attacks with no silent attacks or clipped reference samples.
- Effects compiler/export suite: 177,682 checks and 27,279 trajectory states.
- RAWCLK ownership, burst, paired speaker and new shared merger tests pass,
  with zero unsafe IRQ calls under mocks.
- All 32 historical authoring-packaging regressions pass.
- Portable MIDI/bank tests retain Default trace 70D11B5D (6,634 events,
 9,798 renders), and the per-note audition suite retains154 contributions.

The rebuilt MIDIMAP.DRV and MMLFIDL.EXE are byte-identical to the shipped
authoring kit. SOUND.DRV, RAWCLK, MIDI banks and existing driver source were
not changed. The new merger source is shared with libtandy and libwintandy.

Private HX8088 /7.16MHz /640KB /Windows3.0 runs exercised preceding
candidates. Native checks passed in-app relative and absolute FXP import,
target rejection, embedded song save/load after deleting the source FXP,
the mixed demo's initial queue/speaker claim, replay, deactivate, failure
release and close. Native playback/pause assertions failed after the queue
drained: earlier `sent512->512 error4 run0`, latest `sent160->160 error4 run0`.
Those recordings are diagnostic evidence, not continuous playback passes.

The final source restores512 prepared startup frames, uses PM_NOYIELD for
input checks, restores queue lookahead before cooperative Yield, and updates
only five one-pixel play marks instead of repainting40 text cells every step.
Native diagnostics now measure maximum service gap and full-paint cycles.
The GUI-stall assertion measures RAWCLK cycles and buffers its output in RAM.
The producer/actual-RAWCLK host model passes with the current512-frame policy.
Independent review resolved launcher specialization, native-test reentry,
shutdown close refusal and extracted-source launcher validation findings.

Private build/run directories and source-pinned receipts are retained in the
task workspace. Earlier failed receipts remain unchanged. FXPACK.PY requires
matching host, clock, native and recorded-audio PASS receipts, and refuses an unqualified
consumer kit. No release or publication operation has been performed.
