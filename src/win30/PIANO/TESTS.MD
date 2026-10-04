# Mini Piano 0.1 verification

## Fresh publication checks, 2026-10-04

- Period C6/LINK4 build: no warnings; byte-identical to the tested release.
  MINIPIAN.EXE is 15327 bytes, SHA256
  22cab48dce6baba821a72689fcaee848b3bd3fe4e9b9da659f470047abe6b4fc.
- Strict-C89 production-core oracle: 100036 cases, 65894 emitted events
  matched the independent Python reference. Includes 13 scan mappings,
  repeated presses, overlap, glissando, all 14 owners, bounds, chord
  release, panic, octave clamping and old-note cleanup.
- ASan/UBSan: 200000 deterministic ownership/output-invariant iterations.
  Leak detection disabled; the core does not allocate memory.
- Scoped ISA audit: 3754 reachable instructions, zero post-8088 suspects.
  Imported routines, unresolved indirect paths and unreachable bytes
  are not exhaustively covered. See BUILD.MD for repeatable commands.

## Retained native qualification

Seven final native Win3.0 /R DOSBox-X runs passed with clean return to DOS
and the same release executable hash. The 160x200 lifecycle ran at 5000
cycles; 640x200 mono at 20000. Additional runs covered busy-owner retry,
missing driver, injected send failure, separate nonowner intrusion and
an original demonstration. XMS, EMS and UMB were disabled; conventional
memory was 640 KB. These settings are not calibrated physical CPU speed.

Three complete lifecycle flows each matched 36 independently expected
messages. Busy/retry matched two; missing/send-failure zero; demo 16.
Checks cover note on/off, repeat suppression, overlapping mouse/key
ownership, glissando, outside release and drag-back, button-up, three
voices and oldest-voice stealing, harmless stale off, octave changes,
held-repeat suppression, focus/deactivation, capture watchdog,
WM_CANCELMODE, menu-entry cleanup, Esc, duplicate launch, octave limits
and Close while sounding. Normal cleanup checks reset success, zero
voice mask, close success and released ownership. Separate MIDIINTR
nonowner checks also passed.

These are native message-driven tests, not a human keyboard/mouse pass.
Conservative mute precedes nonclient/menu/system-command entry, but
physical key release inside Windows modal loops remains unqualified.
The capture watchdog is cooperative; 100 ms is not a hard bound.

Actual final native captures were inspected for legibility at both
resolutions. Pressed markers, black/white keys, buttons and help text
are visible. Initial low-resolution painting is slow; ordinary updates
invalidate only changed markers/labels and one status row. Captured demo
fundamentals were approximately 260/330/390/525/495/390/330/260 Hz, matching
the intended notes at PSG/coarse spectral resolution. Media and raw
machine-specific receipts are not committed.

## Qualification limits

Physical Tandy behavior, keyboard rollover/layout differences, manual
modal input, electrical sound, latency/playability, other video drivers,
protected mode, forced termination and abnormal shutdown are pending or
unqualified. No physical acceptance is implied. No new IRQ hook, timer
ISR, system registration or direct application PSG I/O is introduced.
The unchanged driver is the existing 3715-byte app-local Beats MIDIMAP.
