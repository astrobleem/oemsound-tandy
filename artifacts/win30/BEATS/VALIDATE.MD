# Qualification of the preview

Native binaries tested:

- BEATS.EXE: 10,959 bytes, SHA256
  40b78a5e319793407c3e14d4ff08c61ad200b05a313f3485680f385258bfd1fb
- MIDIMAP.DRV: 3,715 bytes, SHA256
  4b960b2918561b077c5b1deb6bf8461ac36851b6023f226145c15d4d7bb5dd81

Built with the existing licensed Microsoft C6/MASM toolchain inside DOSBox-X:
/G0 /Gw /AS /W3 /Od, plus /Gs for the fixed driver. Two C4135 integral
conversion warnings remain in BEATS.C's bounded byte-pattern assignments;
there are no compiler/linker errors. A repeated clean build is checked in
REBUILD.JSON. The existing accepted TSOUND.DRV source/binary is not changed.

## Native results

All runs use Windows 3.0 real mode, machine=tandy, 8086_prefetch (DOSBox-X's
8088-compatible prefetch core), 640KB and disabled XMS/EMS/UMB. The app passed
in all five existing guest display configurations: 160, 3204, 6402, 6404,
and baseline. Each /test completed two loops (17 step starts including the
next loop's first step), reset to silence, closed and released ownership,
then returned to DOS with TOTAL_FAILURES=0.

The 160 run also passed at cycles=fixed 5000. Other display cases used 20000.
These cycle settings are emulation parameters, not a physical 4.77 MHz
performance certification. Guest video frames were inspected in all five
modes: controls, grid, selected note and status fit. The 160 framebuffer
capture is horizontally doubled by the emulator (320x200 capture of the
160x200 mode); 640-wide captures may be vertically doubled by its video path.

Automatic /uitest passed cell-toggle and Space messages, pitch increment,
tempo decrement/increment, repeated Play and Stop, Clear, Demo, close/mute
and ownership release. /closetest passed closing while playing. These use
native Windows messages and actual controls/model, not physical/manual CUA.

MIDITEST passed 119 checks and its separate MIDIINTR task passed all five
ownership rejection checks. Coverage includes capability-size bounds,
existing SOUND ownership, invalid device/cookie, silent-on-error, note ranges,
velocity extremes and zero-off, three-voice polyphony, fourth-note stealing,
stale note-off, noise percussion, per-channel all-off, unsupported-message
rejection, active close, repeated resets and twelve open/play/close cycles,
lease release and library unload/reload. The exact same driver binary was
used for this suite and the app's final matrix.

The app also completed its two-loop native test alongside the untouched stock
SOUND.DRV. Therefore the paired BEATS.EXE/MIDIMAP.DRV can run side by side
without replacing Windows drivers. Concurrent SOUND ownership was tested
with the accepted TSOUND.DRV; arbitrary direct-I/O clients are outside it.

## Additional checks

HOSTTEST.C includes the actual driver C source with mock Windows/port calls.
It passed 200,012 logic assertions, including 100,000 note allocation events
and a specifically non-slot-ordered counter-wrap regression. AddressSanitizer
and UndefinedBehaviorSanitizer were enabled, with leak detection disabled
because this execution environment's ptrace setup prevents LeakSanitizer.
This is a host logic check, not a 16-bit ABI or Windows binary build.

The scoped NE disassembly audit found no post8088 suspects in 3,067 reached
app instructions or 396 reached driver instructions. Its scope explicitly
excludes unresolved indirect jump-table paths and imported Windows code;
it is not exhaustive ISA certification. The driver has one audited OUT
instruction to port C0h and the source has no IRQ/PIT/PIC writes. The app
contains no direct PSG OUT instructions.

## Audio

Tandy-Beats-Lab.wav is original native emulator PCM from the paired app and
driver, with only leading startup silence trimmed. No synthesized replacement,
normalization, pitch correction or mixing was applied. AUDIO.JSON records
the crop and coarse FFT bins near the first chord's131/262/330 Hz tones.
Native emulator filter/DC decay after Stop is retained; explicit driver
state assertions independently establish all channels were muted.

## Limits

No physical Tandy test, manual pointer/keyboard CUA, standard MMSYSTEM/WinMM
integration, Media Player/MIDI-file playback, abrupt owner termination,
hardware busy arbitration against arbitrary direct-I/O programs, timer
resource-exhaustion injection or long-running timing-precision qualification
is claimed. Library unload occurs only after successful MODM_CLOSE in normal
paths. WEP mutes locally without calling linked DLLs; an abnormally abandoned
SOUND lease may require restarting the isolated Windows session.
