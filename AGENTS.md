# AI contributor instructions

This repository hosts a Windows 3.x sound driver and related utilities for Tandy 1000 machines.
The code targets 16-bit real-mode DOS/Windows and expects the Microsoft C 6.0 toolchain.

## Critical build constraints
- **Do not attempt to build in a standard Linux shell.**
- Requires a 16-bit DOS environment with Microsoft C 6.0 or 7.0 (`cl.exe`, `link.exe`, `lib.exe`).
- Testing should use real Tandy 1000 hardware or accurate emulation (e.g. DOSBox‑X with `machine=tandy`).

## Environment setup
- Install **DOSBox‑X** or **PCem** and mount this repository and the compiler inside the emulator, e.g.:
  
  ```
  dosbox-x -c "mount c /path/to/repo" -c "mount d /path/to/msc60" -c "c:"
  ```
- Verify the toolchain is on the DOS path with `cl /?` and `link /?`.
- As an open-source alternative, install **Open Watcom** (`wcl`, `wlink`).

## Coding style
- Stick to **C89** syntax; avoid C99/C++ extensions and `//` comments.
- Indent with **4 spaces**; no tabs.
- Keep lines at **80 columns** or fewer.
- Use descriptive `static` functions and all-uppercase macros.
- Prefer explicit integer widths (`unsigned`, `WORD`, `BYTE`) and `const` when possible.
- Functions with no parameters should use `void` in the signature.

## Build & test
- Build by running `build.bat` inside the DOS environment:

  ```
  build
  ```
- The script compiles `src\psgtest.c` and links a tiny-model executable using Microsoft C:

  ```
  cl /O /AS /Fepsgtest.exe src\psgtest.c > output.txt
  link /NOI /TINY src\psgtest.obj, psgtest.exe,,; >link.txt
  ```
- Expected artifacts:
  - `psgtest.exe` (~1–2 KB) – plays a C major arpeggio, brief noise burst, and fade‑out (~1.5 s total).
  - `psgtest.obj`, `output.txt`, `link.txt`.
- To validate a clean build:
  1. `del *.exe *.obj *.txt`
  2. `build`
  3. `psgtest` (confirm audio sequence)
- When driver sources are added, build `.DRV` files with the Windows 3.1 DDK.

## Test environment / CI
- The default Linux container lacks a DOS emulator and the 16‑bit MS toolchain, so `build.bat` fails here.
- To run builds automatically, provide:
  - A DOS emulator such as **dosbox-x** or **pcem**.
  - Microsoft C 6.0/7.0 binaries (`cl.exe`, `link.exe`, `lib.exe`) and libraries.
  - Optionally, `Open Watcom` 16-bit tools (`wcl`, `wlink`) as an open-source alternative.
- CI could invoke: `dosbox-x -c "mount c . & c: & build.bat"` once these dependencies exist.

## Pull request expectations
- Keep commits focused and include relevant documentation updates.
- Verify the build runs in your DOS environment and, when possible, attach the program output.

