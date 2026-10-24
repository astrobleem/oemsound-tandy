# TPLAY Windows XT display and reservation policy

The display selects a framebuffer memory contract. It is not an audio API.

| Selected display | Reservation before Windows |
| --- | --- |
| CGA.DRV | None |
| HPXY.DRV (current XR1) | Exact first-party RESERVE,16 KB overlap |
| XRSYNC.DRV (reviewed PR155 e84a17d candidate) | Same guard |
| TANDY88.DRV (reviewed XRBUILD output alias) | Same guard |
| TR51616, TR53216, TR53204, TR56402, TR56404.DRV | Same guard |
| TNDY160, TNDY3204, TNDY6402, TNDY6404.DRV | Same guard |
| Any other name, including TXTMODE.DRV | Refuse |

Names are case insensitive; duplicate boot sections/selections still refuse.
The five shipped Windows XT TR5 profiles and their original TANDY88/TNDY
names share the normal 640K reservation contract in oemdisplay-tandy's
AGENTS.md, docs/MODES.MD and tools/FLASHDISK package. XR1's shipped
PROFILES.TXT/OWNERS.TXT and WINXT.BAT use HPXY and the same helper.
experiments/XYRRECT/XRBUILD.PY explicitly copies TANDY88.DRV to HPXY.DRV;
that reviewed candidate alias needs exactly the same guard. The reviewed
PR155 commit e84a17d1562a3f938b5268b8a92fc5e47b176deb supplies its synchronous
candidate as XRSYNC.DRV. Its 60,800-byte driver is byte-identical to that build's
HPXY output (SHA256 ae2d06f69ff68e64f099c3e0af79028e40e7dca89bd442de23eb18d36be4dca1).
XRBUILD retains the same 640x256 logical desktop, 320x200 viewport, 64,000/17,920
shadow bands and 1,440-byte stage; its OEM packet pins the same RESERVE.COM.
XRBATCH changes synchronous Output/cursor presentation, not the memory guard.
Only this explicit reviewed name is added; XRSYNC2.DRV still refuses. This is an
explicit list, not a fallback for arbitrary graphics drivers.

RESERVE.COM is the unchanged XR1 helper:1512 bytes, CRC32 c794fcea,
SHA256 644290e234cab9a6ac3799fa32959bb493d82d566501a8925720bb5a28c6afa8.
RESERVE.ASM and RSV-MIT.TXT preserve its first-party source and notice.
The helper validates ROM,624KB BIOS value, text mode, DOS version, MCB
arena/overlap and its versioned owner marker. It restores allocation strategy,
retains no interrupt hooks and only reuses its exact matching reservation.
TPLAY invokes this app-local exact file once; it never chains another launcher
or searches PATH. Helper refusal prevents Windows and bridge installation.

SYSTEM.DRV ordinal/code ownership, exact paired TSOUND.DRV checks, boot-entry
uniqueness, root-shadow refusals, ROM markers, ROM IRQ0 ownership, bridge
identity/version/parent ownership and cleanup are retained. Canonical source
also retains shipped UX3's EX/HX and variable WIN.COM bootstrap policy. DOS 3+
now has an explicit early refusal instead of a misleading path error.

## Reproduce

Use external licensed MSC6/MASM5.10/LINK4 and the installed DOSBox-X. Every
native command requires an available shared emulator lease. BUILD.PY builds
in a fresh private directory; RUNDOS.PY checks processes, holds the byte lock,
limits the run and stops only its own child. Pass --dosbox, --toolchain,
--lease and --out to BUILD.PY. Run TESTPOL.C with a C89 host compiler.
TESTDOS.PY takes --windows (owned paired profile), --packet (candidate folder),
--dosbox, --lease and --out; it exercises the actual DOS executable 31 ways.
BUILDQA.BAT/TPQA.C/TPQA.DEF/TPQA.RSP build the native observer as RAWGUI.EXE.
In a disposable C:\MMLAB fixture, TPLAY's existing /TEST starts that observer,
which launches unchanged MINIPLAY, plays STREAM.MID, moves/repaints the window,
checks foreground audio service, stops, checks sound release and closes Windows.
For CGA set [TPQA] height=200 in the disposable WIN.INI; HPXY defaults 640x256.

TESTS.JSON and EVIDENCE contain the final launcher build and exact native
results. Both HPXY and CGA play/graphics/stop/exit runs pass. The HPXY guard
owner, MCB count, strategy and BIOS size are identical before/after; audio
capture is nonzero. Missing/wrong helper, unknown/duplicate/incorrect drivers,
existing bridge, occupied overlap, DOS 2 and wrong hardware refuse. A synthetic
Windows bootstrap returning 1 exercises owned bridge removal after failed start.
The native observer, logs and fixtures are tests, not runtime packet programs.

## XRSYNC integration follow-up

The current launcher passes 49 host policy checks and 31 native preflight cases,
including XRSYNC upper/lowercase acceptance and XRSYNC2 refusal. EVIDENCE/XRSYNC.JSON
pins the reviewed candidate commit, actual driver, helper, new launcher and run.
The observer obtains the loaded DISPLAY module filename from Windows and records
C:\WINDOWS\SYSTEM\XRSYNC.DRV before playback. Actual XRSYNC playback during
window movement/repainting, stop, sound release, normal Windows return and owned
bridge cleanup pass. A fresh video reservation was allocated by TPLAY itself.
Earlier HPXY/CGA and helper-reuse receipts remain explicitly tied to their
previous launcher bytes; those unchanged paths were not given new GUI claims.
