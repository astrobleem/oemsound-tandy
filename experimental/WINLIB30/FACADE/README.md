# Proposed audio facade

`LWAUDIO.H` is a declaration-only include facade for the existing app-local
MCI and raw-player interfaces. It forwards the original `MCIAPI.H` and
`RAWAPP.H` headers; it adds no API, ownership behavior, queue logic, format,
or timing policy. Include the Windows 3.0 `WINDOWS.H` declarations first and
make the source headers available on the include path. `SOURCE/RAWAPP.H` has
its own include guard and preserves the raw-player declarations from the frozen
R3 interface. The compact R3 header evidence is 155 bytes (SHA-256
`25e6b19b797dec8d5a96a08aa5dae604b6c6f17db088ccc349e43d939d4eab82`); the
guarded R4 declaration header is 206 bytes (SHA-256
`d9aaac4b291096c374b3bd19fde729ddb4ec647e9e6b86286b75a6495a93aba4`). The
R4 header is an expanded guarded form, not a byte-identical copy.

The host layout check in `../TESTS/FACADE` compiles the original
`MDAPI.H`, `TIMECLK.H`, and `MCIAPI.H` declarations using C89, 2-byte maximum
structure alignment, 16-bit `WORD`, 32-bit `DWORD`, and a 32-bit token in place
of each Win16 far pointer. It checks fixed layouts such as `MDINFO` (40 bytes)
and `TCFRAME` (16 bytes). This validates declared field widths and layouts
under the host fixture only. It does not establish a Win16 compiler ABI, native
binary compatibility, or runtime/audio behavior.


The UI/application shell can remain hardware-neutral. The existing R4 TPLAY and
TSTART entry points explicitly whitelist Tandy HX BIOS/IRQ 0, while the bridge
uses direct I/O port 0C0h. PCjr audio support is pending and unqualified; this
facade makes no PCjr compatibility claim.
