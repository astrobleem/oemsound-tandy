# Effects Lab first native milestone

Base: OEMSound main `1c1d066`, isolated branch `feat/effects-lab`. The maintained
implementation is `src/win30/EFFECTS`. The shared launcher source stays exact.
The app-local launcher snapshot adds only a defaulted TP_PLAYER macro;
LAUNCPIN.PY verifies it against canonical TPLAY with that exact transformation.
FXSTART selects EFFECTS and retains every existing driver/ROM/IRQ/bridge check.
No shared SOUND, RAWCLK, MIDI bank or accepted example sound bytes changed.

The final host run `effects-host-07` passed 177,682 checks over 27,279 compiled
trajectory states using actual `lt_audio.c`/`lt_chip.c` and FXPLAY.H, through
the managed GCC wrapper. It exercises all five presets, both targets, all
noise modes, maximum durations, deterministic Random/Mutate, codec rejection,
save/backup/load, final silence and the PIT frequency limits. It compares the
raw preview shadow against libtandy's observable register state and noise
restart count at every tick. Shared setup/cached redundant writes differ;
this is state equality, not a promise of identical redundant byte sequences.

An additional cross-build check loads the actual native-saved NATIVE.FXP on
the host and recompiles it to a byte-identical native-exported NATIVE.H.
That check found a mixed signed/unsigned multiplication which changed negative
slides only on the 32-bit host. Explicit signed-long tick multiplication fixes
it; the resulting MSC6 executable is byte-identical to the tested native build.

`effects-build-08` compiled/linked with MSC6, MASM and LINK4, `/G0 /Gw /AS /W3
/Od`, using the guarded shared emulator lease. The process returned 0 and
released its lease. Final EFFECTS.EXE is 24,773 bytes, SHA256
`2ca7d0ea699ecf87e6bdc41847be2572c335eb113c16f6ef96669fe2ac1b62a7`.
NE segments: CODE 19,818 bytes; DGROUP minimum 6,848 bytes (3,781 file bytes),
plus declared 2,048-byte local heap and 4,096-byte stack. The native patch
uses 26 bytes and compiled clip 2,414 bytes; no synthesis heap or PCM buffer.
FXSTART.EXE is 18,141 bytes, SHA256
`dbfd59e4efabc6c9a87d5f34e5eadc12c095bab15e7c8a5ab924803b3a0376e4`.

`effects-win-02` passed 22 native assertions against those exact executable
bytes in actual Windows 3.0 real mode under guarded DOSBox-X: 8086 prefetch,
640 KB, no XMS/EMS, qualified TR53216 fixed 320x200. Tests cover window fit,
five presets, save/reopen equality, export, keyboard edit, preview, silent
pause/stable cursor, resume, Stop, speaker routing and leased divisor,
speaker pause/Stop, deactivation and close release. The guarded launcher
reported normal TBRIDGE uninstall with BIOS/INT2F vectors restored and Windows
return 0. The final byte-identical source receipt is build08; runtime receipt
is win02, which executed build06.

Actual emulator video frames were inspected: EDITOR.PNG and SPEAKER.PNG show
the full native 320x200 UI, including disabled PSG-only fields on speaker.
They are extracted native capture frames, not mockups or host UI recreations.
The native WAV has nonzero output during playback and a silent final half
second after cleanup. No physical EX/HX, analog-output fidelity, or reference
SN76489 noise equality is claimed.

Two independent source review passes covered finite parsing/narrowing,
Win16 binding failure, transport ownership, save/backup recovery, export
reservations and state equality. Findings corrected include 16-bit integer
narrowing, partial DLL binding, status repainting and invalid pause assertions
against logical shadows. The second pass found no remaining blockers; final
cross-width arithmetic was subsequently fixed and verified as above.

An attempted separate 86Box session on a copied legacy Beats disk refused
at the unchanged launcher preflight: its IRQ0 was 0070:022c, outside the
qualified ROM-owned policy. Removing optional private CONFIG drivers did not
change that. No bypass was added. These two refused receipts remain failures;
they are not substituted for the passing Windows DOSBox-X run. Every owned
emulator child was stopped, and every acquired shared lease was released.

The package contains ready binaries, original PSG and speaker patches, the
unchanged current TWSETUP/TWXT2 setup dependency, matching source and MIT
libtandy export core. It contains no Windows, ROMs or proprietary toolchain.
The installer itself is an unchanged qualified dependency, not a new installer
qualification. The bounded first version has no PCM synth or software speaker
volume. Publication remains pending separate approval; no push, PR or merge.
