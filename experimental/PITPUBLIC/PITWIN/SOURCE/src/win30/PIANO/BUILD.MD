# Build Mini Piano

Use Microsoft C6, LINK4 and the existing Win3.0 headers/libraries inside
DOSBox-X. BUILD.BAT uses /G0 /Gw /AS /W3 /Od. Do not compile the Win16
application with a native host compiler. Host core tests are separate.
Supply lawful copies of all period tools and Windows files.

From this source directory, on the orchestration host:

    python3 BUILD.PY --repo TOOLCHAIN_ROOT --dosbox DOSBOX --out NEW_DIR

NEW_DIR must not exist. The toolchain layout is BIN/CL.EXE,
BIN/LINK4.EXE, LIB/SLIBCEW.LIB, LIB/LIBW.LIB, DDK/286/INC and INCLUDE.
The script mounts the toolchain as C: and the fresh build folder as D:.
It never rebuilds or installs the existing paired MIDIMAP driver.
That driver's source/build recipe remains in ../BEATS.

## Separate host core checks

GCC or Clang plus Python 3, run from this directory:

    python3 ORACLE.PY
    cc -x c -std=c89 -pedantic-errors -Wall -Wextra -Wconversion -Werror \
      -fsanitize=address,undefined -fno-omit-frame-pointer -g \
      PIANO.C FUZZ.C -o hosttest
    ASAN_OPTIONS=detect_leaks=0 ./hosttest
    python3 AUDIT.PY NEW_DIR/MINIPIAN.EXE

ORACLE.PY compiles only the allocation-free PIANO.C ownership core.
LeakSanitizer is disabled because the validation host's tracing
configuration prevents it from starting. ASan and UBSan remain active.
AUDIT.PY requires objdump and the matching .MAP file. Its direct-path
coverage excludes imported Windows routines and unresolved indirect
continuations; __wflags is a known CS-resident data word, not a code root.

## Disposable native test runner

TESTWIN.PY clones a supplied lawful Win3.0 guest directory. The guest
must already have its known-working video/SOUND configuration and
TANDY88\RESERVE. It runs with 640 KB, no XMS/EMS/UMB, 8086_prefetch,
machine=tandy. It does not change the source guest.

    python3 TESTWIN.PY --guest GUEST --out TEST_OUT --build NEW_DIR \
      --driver MIDIMAP.DRV --dosbox DOSBOX --arg /test --cycles 5000
    python3 NATIVEOK.PY TEST_OUT/RESULT.JSON

Use separate --out directories for every run. Supply guests configured
for 160x200 and 640x200 to exercise both. --capture saves emulator audio
and video locally; these generated files are not repository inputs.
Other modes:
- /busytest: SOUND ownership refusal and retry
- /missing with --missing-driver: unavailable paired driver
- /sendtest: injected send failure and cleanup
- /ownertest with --intruder MIDIINTR.EXE: separate nonowner rejection
- /demo: original C4/E4/G4/C5/B4/G4/E4/C4 exercise

MIDIINTR comes from ../BEATS. NATIVEOK.PY accepts one or more runner
RESULT.JSON files and checks their complete event sequences independently.
All test modes write C:\MINIPIAN.LOG and exit Windows afterwards.
Never use them in an active working session. Normal use has no argument.

## Reproducibility and memory

The runtime manifest is in MANIFEST.MD. NE allocation: 12180 code bytes,
2015 initialized data bytes, 2192 minimum data bytes, 4096-byte stack
and 1024-byte local heap. Fixed ownership state is 145 bytes and the
largest event batch is 43 bytes; no application heap buffers are used.
Windows, runtime, control, driver and allocator overhead are additional.
These are not total conventional-memory requirements.

Production source line grouping is intentionally retained from the tested
release. A token-preserving 80-column reflow changed C6 /Od code generation
and no longer reproduced the approved binary. CRLF and four-space leading
indentation are used; the pre-existing long-line layout is a narrow style
exception to preserve exact tested output. No runtime logic was changed.
