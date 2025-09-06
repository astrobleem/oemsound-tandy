# AI contributor instructions

This repository hosts a Windows 3.x sound driver and related utilities for Tandy 1000 machines.
The code targets 16-bit real-mode DOS/Windows and expects the Microsoft C 6.0 toolchain.

## Quick start checklist
- Run `sudo apt-get update && sudo apt-get install -y dosbox-x dos2unix file`
  to make sure DOSBox-X and line-ending tools are available.
- Launch DOSBox-X with this repository mounted and switch to `C:`:
  `dosbox-x -c "mount c /path/to/repo" -c "c:"`
- Inside DOSBox-X set `PATH`, `LIB`, and `INCLUDE` for the bundled Microsoft C
  toolchain (see below).
- Clean old build artifacts with `del *.exe *.obj *.txt`, then run `build`
  and execute `psgtest` to verify the expected audio output.

## Critical build constraints
- **Do not attempt to build in a standard Linux shell.**
- Requires a 16-bit DOS environment with Microsoft C 6.0 or 7.0 (`cl.exe`, `link.exe`, `lib.exe`).
- Testing should use real Tandy 1000 hardware or accurate emulation (e.g. DOSBox‑X with `machine=tandy`).

## Environment setup
- Install **dos2unix**, **unix2dos**, and the `file` utility in the host if editing outside DOS.
- The repository includes Microsoft C 6.0 toolchain and headers under `BIN`, `INCLUDE`, `LIB`, and Windows 3.1 DDK sampl
es under `DDK`.
- Install **DOSBox‑X** or **PCem** and mount this repository inside the emulator, e.g.:

```
dosbox-x -c "mount c /path/to/repo" -c "c:"
```
- Before building, set environment variables:

```
set PATH=C:\BIN
set LIB=C:\LIB
set INCLUDE=C:\INCLUDE
```
- Verify the toolchain with `cl /?` and `link /?`.
- As an open-source alternative, install **Open Watcom** (`wcl`, `wlink`).

## Coding style
- Stick to **C89** syntax; avoid C99/C++ extensions and `//` comments.
- Indent with **4 spaces**; no tabs.
- Keep lines at **80 columns** or fewer.
- Use descriptive `static` functions and all-uppercase macros.
- Prefer explicit integer widths (`unsigned`, `WORD`, `BYTE`) and `const` when possible.
- Functions with no parameters should use `void` in the signature.

## Build & test
Build by running `build.bat` inside the DOS environment:

```
build
```
The script compiles `src\psgtest.c` and links a small-model executable using Microsoft C:

```
cl /nologo /c /O /AS src\psgtest.c > output.txt
echo. | link /NOI psgtest.obj,psgtest.exe,,; > link.txt
```
Expected artifacts:
- `psgtest.exe` (~1–2 KB) – plays a C major arpeggio, brief noise burst, and fade‑out (~1.5 s total).
- `psgtest.obj`, `output.txt`, `link.txt`.
To validate a clean build:
`del *.exe *.obj *.txt`
`build`
`psgtest` (confirm audio sequence)
When driver sources are added, build `.DRV` files with the Windows 3.1 DDK.

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
- Remove compiled binaries and logs (`*.EXE`, `*.OBJ`, `*.TXT`) before committing.
- Use DOS 8.3 filenames and maintain CRLF line endings (`unix2dos` can help).

