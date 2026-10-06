MINI MIDI 0.1 verification, 2026-10-04

Fresh publication checks
- C6 /G0 /Gw /AS /W3 /Od and LINK4 rebuild is warning-free and byte-identical to the tested 20339-byte EXE. SHA256: 27be6cb983e14cb851cfb583f462b75b9c1e618e02deffc3fc648eb888736a44.
- PARSER/TESTSMF.PY: 677 deterministic cases, including exact independent Python Fraction timing/merge expectations.
- PARSER/TESTFUZZ.PY: 100000 ASan/UBSan mutation/API-invariant cases. Leak detection is disabled in the traced host; the core allocates no heap.
- AUDIT.PY: 5957 reachable instructions, zero post-8088 suspects. Imported Windows code, unresolved indirect continuations and unreachable bytes remain outside the scoped audit.

Retained native qualification
Eleven Windows 3.0 real-mode emulator flows passed with clean DOS return: type-0/type-1 playback at 160x200 color and 640x200 mono, repeated Play/Stop/replay, active Close, malformed/missing file refusal before ownership, Browse cancel/open/directory restoration, duplicate instance, cross-task ownership rejection, busy-owner retry, and missing app-local driver. Four playback logs matched independent filtered event sequences (17 type-0 and 26 type-1 messages). These are message-driven native tests, not human input coverage. TESTWIN.PY reproduces tests with a lawful disposable guest.

Limits
The requested 55 ms cooperative timer is workload-sensitive. Recorded worst event lateness was 49..299 ms under accelerated, uncalibrated DOSBox settings. The 64-message dispatch cap and >1-second stale-event mute avoid unlimited catch-up; they do not make a precision sequencer. Physical playback has been reported working with limited musical quality. Dense songs, calibrated speed, complete manual input coverage, other modes, crashes and abnormal shutdown remain unqualified.

MIDIMAP is unchanged from src/win30/BEATS: 3715 bytes, SHA256 4b960b2918561b077c5b1deb6bf8461ac36851b6023f226145c15d4d7bb5dd81. No driver duplication, compiled binaries, media, OS/toolchain files, screenshots or private test receipts are added.
