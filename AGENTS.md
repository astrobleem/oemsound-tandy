# AI contributor instructions

This repository hosts a Windows 3.x sound driver and related utilities for Tandy 1000 machines.
The code targets 16-bit real-mode DOS/Windows and expects the Microsoft C 6.0 toolchain.

## Coding style
- Stick to **C89** syntax; avoid C99/C++ extensions and `//` comments.
- Indent with **4 spaces**; no tabs.
- Keep lines at **80 columns** or fewer.
- Use descriptive `static` functions and all-uppercase macros.
- Prefer explicit integer widths (`unsigned`, `WORD`, `BYTE`) and `const` when possible.
- Functions with no parameters should use `void` in the signature.

## Build & test
- Builds require Microsoft C 6.0 or 7.0 and `link` running in a DOS environment.
- Compile the demo program by running `build.bat` inside DOS (e.g. DOSBox‑X):

```
build
```

- `build.bat` produces `psgtest.exe`, which plays a short triad to verify the PSG logic.
- The project also depends on `CONIO.H` and `DOS.H` in `src/`, matching the legacy headers.
- When the driver sources are added, use the Windows 3.1 DDK tools to build the `.DRV`.

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

