# WININST 1.2 validation

## Result and exact scope

The candidate passed 23 accepted Windows/DOSBox-X runs. The final
mapper is 5948 bytes with SHA-256
`a59ddd0c121d89a7d446eccc00e03f9646c3372e9505b9dcd48f6a44cbb5ac39`.
The four client binaries and mapper are byte-identical between build03 and
build04; build04 only changes the test harness's relative-stage deadlines.
All reported production runtime tests therefore cover the packaged bytes.

All native runs used Windows 3.0 real mode, the normal 640 KB conventional
memory configuration, an 8088-compatible `8086_prefetch` CPU core, and no
EMS/XMS/UMB/FPU. CPU probes verify pre-186 semantics; Windows reports flags
64. A 16 KB video-overlap reservation is the existing display setup. The
stock system SOUND.DRV was unchanged and its OpenSound lease was respected.
No claim of physical 8088 cycle calibration, CPU percentage, physical PSG
sound quality, hardware joystick timing or real-hardware qualification is made.

## Passed checks

- 791,501 host assertions on the actual mapper/instrument source: every
  program on every channel, all original profiles, all supported dividers
  and velocity offsets, random overlap/retrigger/expiry, long collapsed
  updates, time wrap, stealing, channel cleanup, owner rejection and legacy
  mode. AddressSanitizer/UndefinedBehaviorSanitizer pass with leak scanning
  disabled. LeakSanitizer itself is unavailable under the execution host's
  tracing environment; no leak-check pass is claimed.
- 200,012 legacy mapper source assertions, including age-counter rollover.
- Existing Piano independent oracle: 100,036 cases and 65,894 matching MIDI
  events; 200,000-operation allocation-free core stress also completed.
- Existing joystick decoder/mapping host suite: 194,837 assertions and
  10,231 randomized Note Ons. This is software-input coverage, not hardware
  joystick acceptance.
- 149 instrument-specific checks in each native instrument run: onset and
  decay, per-channel Program Change and held snapshots, optional vibrato,
  immediate Note Off, stealing and stale-off handling, channel All Notes
  Off, drum expiry, flags/program reset semantics, deliberate starvation,
  bounded catch-up, 16 repeat open/close cycles and SOUND lease release.
- Native instrument coverage includes 160x200x16 at 25000 and 3000 emulator
  cycles, 320x200x16, and 640x200x2. It does not claim all clients in every
  display mode.
- Actual Mini Piano controls, shared mouse/key holds, glissando, note range,
  overlap/stealing, octave changes, focus/deactivate/menu/cancel cleanup,
  mouse watchdog, Panic, duplicate instance and nonowner intruder, plus busy
  and injected send-failure recovery. Exact note-event oracle passes.
- Actual JOYMIDI controls and simulated raw input, release gate, pitch and
  velocity behavior, focus/port/Panic safety, duplicate/intruder ownership,
  busy retry and injected send-failure cleanup. Exact event oracle passes.
- Actual MiniMIDI plays the original program-changing file: 16 sent channel
  messages, zero skipped. Repeat Play/Stop, interrupted Close, Browse cancel
  and selection, directory restoration and low-budget playback pass.
- Actual Beats UI sequence, pattern edits, tempo changes, repeated Play/Stop,
  Clear/Demo and final close/lease cleanup pass. Its unchanged original
  public EXE also passes with the new mapper in default legacy mode.
- Six scoped 8088 disassembly audits cover the driver, four clients and QA
  harness. The mapper audit follows 1,748 reachable instructions, finds no
  post-8088 suspect, and finds only OUT 0xC0 as direct I/O. Imported Windows
  routines, unreachable bytes and unresolved control-flow paths are outside
  this audit; it is not a physical-machine certification.

## Audio and timing evidence

AUDIO contains digital TANDY mixer recordings from the same original
nine-second audition and final mapper in flags 0, 1 and 3 modes. Equal gain
is retained; recordings are not independently loudness-normalized.

- Legacy Keys-note late/early RMS ratio: 0.9928.
- Envelope Keys-note late/early RMS ratio: 0.1589, about -16 dB. This directly
  confirms the held-note amplitude change rather than just a new label.
- With vibrato disabled the observed C4 divider stays 428; enabled Pad
  rendering visits 426, 427, 428, 429 and 430. This trace is driver state,
  not a hardware-bus trace. The recording also changes accordingly.
- The Tandy DAC and PC-speaker mixer streams are exactly zero throughout
  all three audition captures. No extra tonal source was added.
- Unloaded audition service intervals after the first service have median
  55 ms, 95th percentile 109 ms and maximum 109 ms (148 measured intervals
  per recording). The initial service includes window startup delay.
- Deliberate heavy redraw plus an 880 ms busy/non-yielding block produces
  up to 1483 ms service gaps at 25000 cycles in 160 mode and 5712 ms at 3000
  cycles. The 320x200x16 stress case reaches 1923 ms. These are intentionally
  hostile UI conditions; the sound holds stale values until service resumes.
- Recovery visits four voices once and writes no more than ten PSG bytes.
  Expired drums/bells are not replayed, held voices catch up, and cleanup
  remains immediate when the owner next runs.
- MiniMIDI's measured worst event lateness is 98 ms at 25000 cycles and
  764 ms at 3000 in this fixture. Its existing one-second stale-event stop
  remains enabled. These results do not promise hardware playability.

The separate short MP4 uses labeled stills of the actual native test window
with these recordings. It is an audio comparison, not continuous UI footage.

## Development issues found and resolved

1. Independent review caught a real Win16 near-pointer hazard before
   expressive acceptance: DLL DS can differ from the caller's stack SS.
   Near output pointers now address fixed DGROUP scratch. The compiled
   call/read sequence was verified and native expressive tests rerun.
2. The old API test assumed OpenSound returned three voices. The untouched
   stock system driver returns one; tests now retain and compare the actual
   successful lease result rather than replacing the system driver.
3. The first low-budget test used absolute stage deadlines, making its
   envelope assertions premature after slow window startup. Relative
   note-on/stage deadlines fixed the harness; the same mapper then passed
   all 149 native checks at the lower budget.
4. Initial runs wrote their DOS return marker in Windows' changed working
   directory. Completion was verified there; the final runner uses an
   absolute C:\ return-marker path.

The superseded failed builds/tests are not labeled accepted or shipped.
The final manifest lists only completed accepted runs. This kit does not
qualify crash recovery, arbitrary forced DLL unload, protected/enhanced mode,
GM timbral fidelity, ordinary MIDI expression/pitch bend/SysEx, release tails,
or hard real-time synthesis.

## Reproduction

Build the C sources with lawful Microsoft C6/MASM/LINK4 and Win3 headers and
libraries in DOS/DOSBox-X; each SOURCE subdirectory retains BUILD.BAT/PY.
Always use /G0, not a 186/286 code-generation switch. The mapper build also
compiles INSTR.C. No compiler/OS binaries are bundled.

Run `sh QA/HOSTTEST.SH` from an unpacked kit with a host C compiler. These are
host logic checks, not the native build. QA/RUNWIN.PY --help describes the
self-contained native runner. Supply a disposable lawful fixture, display
driver, the RUNTIME directory, DOSBox-X executable, new output directory
and a shared emulator lock. The runner clones the fixture, leaves its
SOUND.DRV unchanged, enforces 640 KB/real-mode CPU settings and exits the
isolated Windows session. It was itself verified by the portable-runner
receipt. EVIDENCE retains the exact configurations used for accepted runs.
