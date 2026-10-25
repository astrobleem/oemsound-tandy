# Contributor guidance

This repository owns the unified Windows 3.0 real-mode SOUND driver,
applications and packaged source/runtime. Production targets 8086/8088 with
Microsoft C 6.0, MASM and LINK4. Start with `README.md` and the component's
build instructions; root `BUILD.BAT` builds only the legacy DOS PSG test.

## Ownership and the next PSG bank task

- `src/win30/DRIVER/` is the canonical unified `SOUND.DRV`. Published
  Winamp uses MCITNDY -> MIDICLK -> RAWCLK and the existing speaker
  transport. Instruments do not require another sound driver or timer.
- Implement portable bank behavior in astrobleem/libtandy `sound/INSTR.C`
  and `MIDISYN.C` first; read its `docs/PSG-BANKS.md`. Windows integration
  belongs in astrobleem/libwintandy `libtandywin/AUDIO/SOURCE/`.
- `experimental/WINLIB30/SOURCE/` is the current Windows engine mirror
  consumed by `tools/WINAMP/PACK.PY` / `VERIFY.PY`, despite its directory
  name. Preserve newer paired-speaker fallback, controllers and streamed
  SMF behavior. Coordinate focused vendor/mirror updates, not wholesale
  replacement with the older standalone core. Packaged source snapshots
  are outputs, not independent places to design instrument behavior.
- Preserve Default exactly, add one contrasting bank, and provide a host
  A/B audition of identical notes/events. There is no current bank selector;
  envelope/vibrato flags are not banks. No accepted song edits, SoundFont
  claims or consumer Python/modern-PC preparation. Developer Python is fine.
- The separate VGM task is changing player/format/decompression code.
  This documentation/bank handoff must not change those files or branch.
  Work in isolated current-main checkouts; do not launch emulators.
  Local commits are allowed; publication needs explicit approval.

## Build and verification

The component builders require an externally owned toolchain, new private
output and (where supported) a shared emulator lease. Do not assume old
`BIN`, `LIB`, `INCLUDE` or `DDK` trees are the current build setup. Open
Watcom is not a proven drop-in replacement for the published Win16 DLLs.

- Driver: `src/win30/DRIVER/BUILD.PY` (`--dosbox`, `--toolchain`, `--lease`,
  `--out`); backend: `experimental/WINLIB30/SOURCE/BLDRAW.PY`
  with the same options. Both launch DOSBox-X; do not run them in this task.
- The current three-DLL native batch is
  `experimental/WINLIB30/SOURCE/BLDMIDI.BAT`. There is no `BLDMIDI.PY`.
  Earlier `experimental/WINLIB30/BUILD/PLAYER30.PY` is historical and
  does not acquire the current shared lease. Do not launch it as a shortcut.
- Player UI: `experimental/WINAMP30/BUILDING-UI.MD`. Native packet/source
  verification: `python tools/WINAMP/VERIFY.PY --self-test`. Packaging
  regressions: `tools/WINAMP/TESTMIDI.PY`, `TESTBASE.PY`, `TESTPIT.PY`.
- Driver host checks: `python src/win30/DRIVER/HOSTTEST.PY --out <new-output>`.
  Windows MIDI renderer/controller checks are documented in libwintandy
  `docs/BUILD.TXT`; host CI coverage is in `.github/workflows/tplay.yml`.
- On Chad's Windows host compile through
  `C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py`; never start
  raw GCC/cc1. Uppercase `.C` files need `-x c`. Host tests do not prove
  Win16 ABI, native segment sizes, real-time playback or audible quality.

Use C89 production code and preserve DOS 8.3 runtime names and existing
line-ending conventions. Keep fixed storage, 16-bit near/far contracts and
the 640 KB real-mode budget. Measure CODE/DGROUP changes against the current
MIDI8 receipt in libwintandy before claiming a bank fits. Preserve export
ordinals, shared SOUND/PIT ownership and existing timer/IRQ boundaries.

Keep commits focused. Never clean with `del *.txt` or delete tracked runtime,
source, manuals or evidence by wildcard. Generated outputs belong in new
private directories. Label existing native receipts as recorded evidence;
do not claim a new native or physical-hardware test from host checks.
