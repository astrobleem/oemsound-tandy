BUILD AND MEMORY - JOYMIDI 0.1
=============================

Period toolchain: existing Microsoft C6, MASM 5.10, LINK4, Win3.0 DDK headers,
SLIBCEW.LIB and LIBW.LIB. Supply your own licensed copies.
BUILD.BAT uses /G0 /Gw /AS /W3 /Od. JOYCAP.ASM specifies .8086.
The build links JOYMIDI + JOYCORE + JMCORE + JOYCAP; it does not rebuild driver.

Host orchestration (new output directory required):
 python3 BUILD.PY --repo TOOLCHAIN_ROOT --dosbox DOSBOX --out NEW_DIR
It expects BIN/CL.EXE, BIN/LINK4.EXE, LIB/SLIBCEW.LIB and LIB/LIBW.LIB,
plus the headers and assembler used by BUILD.BAT. Tools are not included.

Host oracle with GCC/Clang (case-sensitive systems need -x c for .C):
 cc -std=c89 -pedantic -Wall -Wextra -Werror -fsanitize=address,undefined \
   -g -I . -x c HOSTTEST.C JMCORE.C JOYCORE.C \
   -o hosttest
 ASAN_OPTIONS=detect_leaks=0 ./hosttest
LeakSanitizer was disabled because the execution host's tracing environment
prevents its startup. AddressSanitizer and UndefinedBehaviorSanitizer passed.
The core and host harness do not allocate heap memory.

TESTWIN.PY clones a supplied lawful Win3.0 guest, copies the app into
C:\JMIDI, and runs it under DOSBox-X. It assumes the guest already contains
TANDY88\RESERVE and the desired known-working display/SOUND configuration.
640 KB conventional memory; XMS, EMS, UMB disabled; 8086_prefetch CPU.
--arg /test: synthetic raw-input lifecycle suite and native PSG checks.
--arg /busytest: existing SOUND owner refusal/retry.
--arg /missing --missing-driver: missing paired driver fails closed.
--arg /sendtest: injected MIDI-send error, fail-closed cleanup and Arm label.
--arg /porttest: bounded real port capture, default muted, no MIDI sends.
--arg /ownertest --intruder MIDIINTR.EXE: cross-task owner rejection harness.
The existing MIDIINTR source/build recipe is in src/win30/BEATS/SOURCE; its test binary
is not needed by the normal application. The runner supplies its root-path
MIDIMAP dependency only in the disposable test guest.

IMPORTANT: these /test modes are developer-only and call ExitWindows after
logging C:\JOYMIDI.LOG. Do not invoke them in an active working session.
Run the normal EXE with no argument for hardware use.

ORACLE.PY accepts native RESULT.JSON paths and compares complete event
lists to an independently stated expected sequence. AUDIT.PY audits
reachable direct code paths; indirect switch continuations are not exhaustive.
It explicitly excludes the public __wflags data symbol from code roots.

Final EXE: 16975 bytes on disk. NE code segment: 13380 bytes.
Minimum data allocation: 2784 bytes; requested local heap: 1024 bytes;
requested stack: 4096 bytes. Raw samples occupy a fixed 512-byte array.
There is no song/file buffer and no dynamic allocation in the app's own code.
Windows, controls, loaded DLLs, drivers and allocator overhead are additional.
These numbers are NOT a total conventional-memory requirement guarantee.

The app depends only on the paired MIDIMAP ABI, not SMF parsing or WinMM.
Copy the whole JMIDI folder; never register the driver in SYSTEM.INI.
