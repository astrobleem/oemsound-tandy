# Integration validation

The final Effects integration passed MSC6 /G0 and LINK4 build11 and the full
native09 acceptance on an emulated Tandy HX8088 /7.16MHz /640KB /Windows3.0
real mode at320x200. It passed55 scripted assertions, then normal launcher,
physical-key Effects groove play/pause/resume/stop and DOS return. The owned
emulator stopped; the shared lease was released and independently checked.
No physical-hardware or subjective listening qualification is claimed.

BEATS.EXE is47,157 bytes, SHA256
236e5d661c929de4a6ab8c7a191441e29d2e459c525360aada17105ce3229de4.
CODE is38,592 bytes. Minimum DGROUP is35,520 bytes; with declared heap2,048
and stack4,096, its segment budget is41,664 bytes, leaving23,872 bytes.
MIDIMAP.DRV and MMLFIDL.EXE are byte-identical to the shipped authoring kit.
SOUND.DRV, RAWCLK and the original MIDI banks were not changed.

Native checks cover relative/absolute TFX1 import, lane rejection, embedded
BTS save/load after removing its FXP, mixed PSG/PIT playback,350ms without GUI
service, pause position, resume, Stop/replay/deactivate/failure/close release.
Playback advanced2,974->2,995 frames during the no-service interval with no
transport error. A42-second full paint also retained prepared audio; the
moving cursor and transport status use small updates during ordinary playback.

The actual48,000Hz PSG and49,716Hz speaker recordings passed14 objective
segment checks: three active playback portions, an active last-second tail
beyond the512-frame startup horizon, active resume and exactly-zero pause/stop
samples for each device. The playback capture interval exceeded11 emulated
seconds. These recordings are evidence, never consumer playback inputs.

Passing host checks include:

- FXTEST:18,054 compiler/merger/producer/codec checks, with35 noise-latch writes
  over1,200 states rather than resetting noise on every level update.
- FXCLOCK:17,195 actual-producer/RAWCLK checks, reproducing empty-queue error4,
  then continuous refill,350ms gap, pause and a conservative20-second paint
  model with bounded per-cell refill and5.5ms producer CPU per frame.
- Original five-demo/MML, transactional-file, PIT bounds/service and current/
  archived mapper regressions;144 intended original demo attacks, no silent
  attacks or clipped software reference samples.
- Effects compiler/export:177,682 checks and27,279 states.
- RAWCLK ownership/burst/paired-speaker/merger regressions with zero unsafe
  IRQ calls under mocks;32 historical authoring-package tests.
- Portable Default trace70D11B5D with6,634 events/9,798 renders;154 per-note
  audition contributions retained.

Independent review resolved launcher specialization, test reentry, close
refusal, package source closure, grid redraw, bridge quiescence and consumer
reference findings. Earlier failed receipts remain diagnostic history; only
matching final host/clock/native/audio PASS receipts can qualify FXPACK.
The short developer diagnostic's DIAGNOSTIC_PASS cannot qualify a package.
Publication still requires explicit approval; no publication was performed.
