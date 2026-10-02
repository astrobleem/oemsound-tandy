# Accepted bounded milestone

PSGPLAY.EXE: 6287 bytes, SHA256
f7a4c9bd479c0ce5ee82398a919e7c77795f646600461800c41af73525190a5d
Clean rebuild matched byte-for-byte.

Seven native Windows cases passed: full playback in five existing display
profiles, Stop during first tone, and close during first tone. Exact output
bytes, recorded audio fundamentals, final four-channel mute, advancing clocks,
unchanged IRQ8/IRQ1C snapshots, and trace-overflow status were checked.
A final 160-pixel GUI pass checked fit, Play, quick Stop and normal Close.
The quick Stop trace muted 385 ms after the C4 note-on, before E4 began.

Cookie coexistence under 640 KB/8086-prefetch advanced its saved cookie count
100->114 at 1 CPS while all three PSG voices ran and then stopped. This is
coexistence evidence, not a frame-rate or physical-machine performance claim.

157 host-stub logic checks cover initial/rearm timer failures, delayed stages,
session-exit handling, close-before-destroy cleanup, trace rollover/saturation
and timer tick wrap. These mocks do not establish real Windows/hardware behavior.
The binary audit covers 1,738 directly reachable instructions; imported Windows
routines and indirect CRT continuation targets remain outside that audit.

DRIVER/TESTS.JSON records 105 native driver/intruder checks, zero failures,
for the exact 5,656-byte restricted driver. See DRIVER/README.MD for limitations.

The deliverable ZIP retains raw traces, PCM captures and the test/audit sources.
It excludes OS installations, stock DDK/Windows code and toolchain binaries.
Publication is a draft milestone for review. Physical Tandy hardware remains untested.
