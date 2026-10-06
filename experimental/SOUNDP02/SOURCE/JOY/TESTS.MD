JOYMIDI 0.1 verification, 2026-10-04

Fresh publication checks
- C6/MASM/LINK4 /G0/.8086 build is warning-free and byte-identical to the tested 16975-byte executable. SHA256: e24be35af4695f4ce36d6d0b3e82291f787672962107ddcf27cc464b5fe83519.
- Strict-C89 ASan/UBSan host oracle: 194837 checks, including 50000 randomized ticks and 10231 Note Ons. Leak detection off; no heap allocations.
- Exact-message checks cover press/release, endpoint pitch/velocity, jitter silence, old-off-before-new-on, panic, timeout, early input, explicit rearm, bounce, rate cap, button 2, dispatch stall and 32-bit clock wrap.
- Scoped ISA audit: 4458 reachable instructions, zero post-8088 suspects; __wflags data excluded. Imported routines and unresolved indirect paths are not exhaustive.

Reproduce host checks from this directory
cc -std=c89 -pedantic -Wall -Wextra -Werror -fsanitize=address,undefined -g -I . -x c HOSTTEST.C JMCORE.C JOYCORE.C -o hosttest
ASAN_OPTIONS=detect_leaks=0 ./hosttest

Retained native qualification
Windows 3.0 /R, 640 KB, no XMS/EMS/UMB, 8086_prefetch, fixed 5000 emulator cycles. Full lifecycle runs at 160x200 color and 640x200 mono emitted exactly 22 expected messages: selection/calibration/held-at-arm gate, jitter, pitch/velocity changes, release/panic, timeout/recovery, focus loss, duplicate activation and sounding Close. Busy-owner refusal/retry, missing driver, injected send failure and cross-task nonowner rejection passed. Eight bounded no-device port reads sent no MIDI. Normal cleanup reset voices and released ownership. TESTWIN.PY and ORACLE.PY retain reproduction support; test modes exit Windows, so use disposable guests only.

Physical joystick input, connector labeling, electrical endpoints, timing, noise immunity, latency/playability and physical sound remain untested. The 500 ms safety check executes only when Windows dispatch resumes. Forced termination, crashes, abnormal shutdown and independent direct-PSG writers remain unqualified. No new driver/system registration/interrupt hook/timer hook is introduced. MIDIMAP is the unchanged app-local Beats Lab driver. No compiled binaries, media, OS/toolchain files or private receipts are added.
