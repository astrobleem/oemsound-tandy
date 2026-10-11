# Optional shared PIT layer in DOS PSGTEST

PSGTEST without arguments plays its existing C4/E4/G4 arpeggio, noise tick
and fade. PSGTEST /pit adds one distinct C5 (MIDI72, canonical523Hz) while
the PSG triad holds. PIT turns off before the PSG fade; its fixed amplitude
has no volume envelope. Escape stops playback and releases owned outputs.
Unknown options reject before sound acquisition. No default promotion.

All sound I/O goes through DOSSND v1, unchanged from the accepted adapter.
The app requests DS_PSG by default and DS_PSG|DS_PIT only with /pit. Acquire
checks the known Tandy ROM, refuses tested Windows sessions and an active
speaker, and claims one cooperative foreground owner. There is no TSR,
cross-process or BIOS/system-beep exclusion. Use an exclusive DOS session.

The baseline's 54 PSG output bytes and BIOS tick waits are preserved. Owner
release adds four final mute bytes; no baseline PIT programming/gate write.
No PIT0/1 or interrupt cadence programming. Port61 changes only its low two
bits through the owner's current-read/modify/write. A speaker that becomes
active after acquire and before PIT onset causes refusal; the app releases
its PSG and leaves the foreign gate intact. DOS signal handlers release on
Ctrl-C/Ctrl-Break; physical key delivery is not covered by the test receipt.

Build with the existing licensed C6 toolchain in native DOS:

    CL /G0 /AS /W3 /Od /c src\PSGTEST.C src\DOSSND.C
    LINK4 /NOE /NOD /STACK:4096 PSGTEST DOSSND,PSGTEST,,SLIBCE;

PSGTHQA.C includes the production app and adapter with counted I/O. Compile
with PSGTEST_NATIVE in native DOS or without it for mocked host tests.
The accepted evidence covers baseline byte identity, opt-in output, Escape
during PIT, pre-acquire busy refusal, post-acquire conflict, idempotent
cleanup, allowed ports, immediate preservation of port61 upper bits and
unchanged IRQ8/IRQ1C vectors. Actual Windows3 real-mode DOS-child cases
refuse both baseline and /pit before any sound I/O. Independent actual
PSGTEST.EXE captures demonstrate PSG/speaker overlap, not physical fidelity.

No sampled speech/PWM path is added. WAVHYB's legacy IRQ0/PIT0 sample path
remains unchanged and unqualified. Physical EX audio/routing, other Windows
modes and non-cooperating software remain outside this qualification.

Final qualification: 31 host checks; 41 native checks across five counted
contexts plus two actual-executable captures (seven runs). Native C6 build
passes without warnings. Optional capture has 20789 simultaneous speaker
and PSG frames; baseline has none. No physical listening claim.
