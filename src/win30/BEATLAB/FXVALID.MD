# Integration validation

Final build13 passed full native12 acceptance on an emulated Tandy HX8088 at
7.16MHz,640KB, Windows3.0 real mode and320x200. All55 scripted checks passed,
followed by the normal launcher, physical Effects-groove controls during a
forced repaint, actual hardware recordings and normal DOS return. Owned
PID27484 stopped, the shared byte lease was released and independently checked.
No physical-hardware or subjective listening qualification is claimed.

BEATS.EXE is48,261 bytes, SHA256
3c0d8c4d2618f67c535098c66801d6c1746c6d0dfee9454ce41c4dd72d4dc2fb.
The native build uses MSC6 /G0, MASM and LINK4, targeting Windows3.0 real mode.
MIDIMAP.DRV and MMLFIDL.EXE retain the shipped authoring-kit bytes. SOUND.DRV,
RAWCLK, the original MIDI banks and their timing/IRQ paths were not changed.

The five-byte dirty-cell mask replaces synchronous full-grid rendering. The
loop checks input before each cell, services audio after input, skips unchanged
label text and releases each drawing DC. Full exposes remain gradual on8088;
transport input is checked between cells. Preparation updates its status and
yields before starting the audio epoch, so pending Windows work does not
consume prepared lookahead. The final run measured maximum service gap1.57s,
cell draw1.47s, header/status paint0.45s and cooperative yield0.017s.

Native checks cover relative/absolute TFX1 import, lane rejection, embedded
BTS save/load after removing its FXP, mixed PSG/PIT playback,350ms without GUI
service, pause position, resume, Stop/replay/deactivate/failure/close release.
Playback advanced492->513 frames in the no-service interval with no transport
error. The normal visible run selected Effects groove and forced a new grid
repaint before both F6 Pause and Escape Stop.

Actual48,000Hz PSG and49,716Hz speaker recordings passed14 segment checks:
three active playback portions, an active last-second tail beyond the512-frame
startup horizon, active resume, and exactly-zero pause/stop samples for both
devices. Playback capture exceeded11 emulated seconds. The silent observation
windows began1.32/1.33 wall seconds after the repaint/transport requests and
within2 captured seconds; analysis trims at most0.25s for capture buffering.
These are observation bounds, not submillisecond key-handler timing. The
Stop-request screenshot precedes completion of its status repaint; AUDIO.JSN
and action marks carry the actual silence/response evidence. Recordings are
evidence only and are never consumer playback inputs.

Passing host checks include:

- FXTEST:18,054 compiler/merger/producer/codec checks;35 noise-latch writes
  over1,200 states instead of resetting noise for every level update.
- FXCLOCK:17,195 actual-producer/RAWCLK checks: empty-queue reproduction,
  continuous refill,350ms gap, pause, and a conservative20-second paint model
  with bounded per-cell refill and5.5ms of producer CPU per frame.
- Original five-demo/MML, transactional-file, PIT bounds/service and current/
  archived mapper regressions;144 intended original demo attacks with no
  silent attacks or clipped software reference samples.
- Effects compiler/export:177,682 checks and27,279 states.
- RAWCLK ownership/burst/paired-speaker/merger regressions with zero unsafe
  IRQ calls under mocks;32 historical authoring-package tests.
- Portable Default trace70D11B5D with6,634 events/9,798 renders and154 per-note
  audition contributions retained.
- Six synthetic audio-gate cases accept sustained responsive playback and
  reject silence, startup-only audio, short capture and slow response bounds.

Independent review covered ownership, tone2 borrowing/restoration, PIT,
transposition, persistence, Win16 memory/API contracts, incremental rendering,
launcher specialization, package closure and documentation. Earlier failed
receipts remain diagnostic history. FXPACK requires matching current host,
clock, full native and audio PASS evidence; DIAGNOSTIC_PASS cannot qualify it.
Nothing has been published. Publication requires explicit approval.
