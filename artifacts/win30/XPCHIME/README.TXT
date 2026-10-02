# XP-note PSG startup chime

XPCHIME.EXE is an optional native Windows 3.0 real-mode / 8088 alternative
startup chime for known original Tandy 1000 EX/HX PSG hardware. It plays a
three-voice square-wave reduction of the user's supplied Windows XP score,
then mutes all four channels and exits. It does not play sampled audio.

The original TCHIME.EXE remains a separate choice. Run only one chime or
other PSG-writing application at a time; cleanup mutes every PSG channel.
No replacement SOUND.DRV is required.

## Melody and arrangement

Source credit: Windows XP Startup Sound, Bill Brown; supplied arrangement
by David Caldarella. SCORE.TXT records the exact supplied-score reading.
Melody: Eb6, Eb5, Bb5, Ab5, Eb6, Bb6 at nominal 0, 300, 450, 900, 1500,
1800 ms. Upper accompaniment follows Ab4 to G4. Bass follows F3, Ab3, Eb3;
the first two cello pitches are raised an octave to fit the PSG range.
Melody and upper harmony end at 2400 ms; bass ends at 2700 ms.
Three square waves cannot reproduce the original piano/string timbre.
This is a PSG arrangement of those notes, not an identical XP recording.

## Try manually, then optionally enable

1. Copy XPCHIME.EXE to C:\TANDY88\XPCHIME.EXE on a known compatible Tandy.
2. In Windows Program Manager choose File, Run and enter that full path.
3. It has no visible window. Listen for the phrase and its silent ending.
4. For optional startup, back up WINDOWS\WIN.INI first. In [windows], add
   C:\TANDY88\XPCHIME.EXE once to the existing run= line, preserving other
   entries. If TCHIME.EXE is present, remove only that chime path first.
5. To disable, remove only the XPCHIME path from run=, or restore the backup.

The app itself changes no startup settings and creates no data files.
Do not add it to AUTOEXEC.BAT or install it as SOUND.DRV. No SYSTEM.INI
change is needed. The example modifies no working Windows installation;
all automated startup tests use fresh disposable cloned fixtures.

## Timing and safety limits

A hidden message window uses a 55 ms Windows timer. The first delivered
timer starts the phrase, allowing initial desktop painting to finish
silently. GetMessage yields; there is no busy wait, interrupt hook, PIT
change, or resident helper. At most one musical transition occurs per
callback. A transition more than 165 ms late aborts quietly rather than
playing overdue notes in a burst. Cleanup covers normal completion, close,
destroy, confirmed session end, and timer/window creation failure.

Cooperative Windows can delay delivery while another task blocks. There
is no hard wall-clock silence guarantee during a frozen/non-yielding
Windows session. Forced task termination and physical Tandy behavior are
not qualified. The ROM signature guard is fail-closed compatibility
checking, not proof of an exact EX/HX model. Protected mode is rejected.

## Build and test

Use an existing licensed 16-bit Windows toolchain, headers and libraries:

python3 BUILD.PY --repo /path/to/toolchain --dosbox /path/to/dosbox-x \
  --out /new/xp-build

Build uses Microsoft C /G0 /Gw /AS /W3 /Od and the segmented Windows linker.
XPCHIME.EXE is 4,543 bytes; a separate repeat build is byte-identical.
No Windows/DDK/compiler payload is included in this addition.

TESTBOOT.PY clones clean fixtures using --runtime-root, --exe, --observer,
--dosbox and --out. The included test-only BOOTTEST observer starts beside
the chime, records hidden module/window state, later checks unloading,
and closes that isolated Windows session. Never install BOOTTEST as a
normal startup item. AUDIO.PY checks ordered melody bands and final silence.

The package includes native startup captures, observer logs, 155 host-only
logic assertions, deterministic mocked PSG writes, and a scoped audit of
1,092 reachable 8088 instructions. Imported Windows routines and unresolved
indirect CRT paths are outside that audit; it is not exhaustive proof.
Physical listening remains unverified. SOURCE and EVIDENCE contain the
reproduction inputs and exact hashes. Existing repository licensing
notices are preserved; its README/LICENSE mismatch is not resolved here.
