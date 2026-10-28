# Contributor guidance

This is the canonical DOS/portable C89 library. For selectable MIDI PSG
instrument banks, start in `sound/INSTR.C` and `sound/MIDISYN.C`, not the
native `src/lt_audio.c` / `lt_score.c` / `lt_sfx.c` APIs. Read
[`docs/PSG-BANKS.md`](docs/PSG-BANKS.md) before changing the bank contract.

- Keep the existing bank as Default, with identical output. Chamber adds one contrasting bank; the developer host A/B audition uses
  identical notes, timing, programs and controllers. No accepted song edits,
  SoundFont/sample claims, extra sound driver or consumer Python preparation.
- `sound/` owns portable instrument behavior; libwintandy owns Windows
  integration and vendors a newer renderer/parser variant. Port focused
  changes deliberately; do not overwrite either tree wholesale.
- Use C89, fixed-size state and integer arithmetic. No heap, OS calls, port
  I/O, timer ownership or interrupt work in the portable renderer. Preserve
  16-bit near/far contracts and bounded work after long foreground stalls.
- `MSReset`, `MSSpeakerMode` and flags control existing behavior; the
  selector is `MSBank` (Default/Chamber). Preserve controller, allocation, reset and mute
  semantics. Direct state changes require `MSInvalidate` before rendering.
- Host GCC checks do not qualify 8088 code size, Win16 DGROUP/ABI, audibility
  or hardware timing. Report those separately and measure added table/state
  bytes. Build commands are in `README.md`; bank-specific checks are in
  `docs/PSG-BANKS.md`.
- On Chad's Windows host, run GCC only through
  `C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py`; never start raw
  GCC/cc1. Uppercase `.C` inputs need `-x c`. Developer Python is allowed.
- For portable bank changes, do not launch emulators or touch the separate
  Windows player, transport or format/decompression task. Keep changes in
  isolated checkouts. Publication requires explicit approval.

There is no `BUILDING` file: `README.md`, `tools/build.py`, and
`sound/tests/run.py` are the current build/test entry points.
