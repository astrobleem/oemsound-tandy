# Native Winamp Setup

Developer reference. For normal installation, use [the DOS setup guide](../../../docs/INSTALL.MD).

The direct consumer download is the self-contained native `TWSETUP.EXE`.
Run it in DOS; it needs no archive extractor or preparation program. The ZIP
also contains a plain `SETUP.EXE` and the same runtime files. Setup finds
compatible Windows installations, selects the only
match or offers a numbered choice, installs the canonical `SOUND.DRV`, and
installs Winamp in `C:\WINAMP`. It updates the existing sound selection while
preserving unrelated `SYSTEM.INI` bytes. The installed menu offers Launch,
Repair, and Uninstall. `GO` starts the installed player normally.

Setup validates its own package, installed file identities, and the launcher's
real-mode preflight before completing installation. It performs recovery
internally if a write fails or the operation is interrupted. Uninstall removes
Winamp and retains the shared canonical sound driver, sound selection, and
Windows XT clients. Unrelated files and settings are preserved.

## Native command interface

```
SETUP
SETUP /INSTALL /WIN:C:\WINDOWS /APP:C:\WINAMP /QUIET
SETUP /REPAIR /WIN:C:\WINDOWS /APP:C:\WINAMP /QUIET
SETUP /LAUNCH /APP:C:\WINAMP
SETUP /UNINSTALL /APP:C:\WINAMP /QUIET
```

`/WIN` and `/APP` provide unattended test selections or alternate install
directories. The default interaction needs neither option. An installed
`SETUP.EXE` finds its own installation state even in a custom app directory.
An intact installation repairs itself using its installed application files
and canonical Windows sound driver. Running Setup from the complete download
also restores missing or damaged payload files.

## Package interface

The package generator supplies a plain ASCII `PAYLOAD.LST` with CRLF endings:

```
TWSETUP1
A|SETUP.EXE|SETUP.EXE|12345|12345678
S|SOUND.DRV|SOUND.DRV|12345|12345678
A|MINIPLAY.EXE|MINIPLAY.EXE|12345|12345678
```

These example identities are placeholders, not usable package pins. Each row
has kind, flat DOS 8.3 source name, destination name, decimal byte count, and
eight hexadecimal CRC32 digits. `A` installs in the application directory.
The single `S` row installs the canonical sound driver in Windows `SYSTEM`.
The driver's length and CRC must additionally equal generated `SOUNDID.H`.
All launcher, player, DLL, bridge, and reservation components are required.
`W` and `X` rows carry the current Windows XT clients identified by generated
`XTCLIENT.H`. `W` targets the selected Windows directory and `X` its drive's
`WINXT` directory. Existing exact current client files and their existing
`WINXT\FILES` mirrors are updated transactionally. Absent optional targets are
skipped; an unrelated file is preserved and installation refused. The same
client bytes are cached in the app directory for installed Repair.
Setup records only the optional global and mirror paths it actually updated,
up to fourteen current client destinations. Repair can restore a missing or
damaged recorded client. A target that was absent and never installed remains
absent.
Current TSHELL's paired `TSINPUT.DLL` is included unchanged. When recognized
TSHELL is installed or repaired, Setup supplies its DLL even if missing.
Stock Windows with no TSHELL receives neither global component. An unknown
existing DLL is preserved and installation refused.
Recognized current FIXXT kits also receive the same canonical driver at
`WINXT\FILES\SOUND.DRV`, which FIXXT uses for Check/Repair. A fresh install
preserves an unknown source file there; recorded kit repair restores its
missing or damaged canonical mirror. This shared file remains after Winamp
removal.

Setup installs the payload index and generates `GO.BAT`; neither belongs in
the index. The installed `TWSTATE.DAT` records the selected Windows and app
paths, current canonical sound identity, owned application/cache file pins,
and the current shared client destinations it updated. Repair and Uninstall
use those ownership records.

`TWSTAGE` holds a journal and transient copies of files affected by the current
attempt. Failed or interrupted writes restore the pre-attempt files; successful
validation removes the journal and staging files. First-install failure also
removes newly created private directories. Uninstall preserves changed app
files and unrelated files/settings while retaining shared sound and clients.

## Build and checks

`BUILD.PY` stages the canonical `../DRIVER/SOUNDID.H`,
`../LAUNCH/TPLAYPOL.H`, `XTCLIENT.H`, and `SFXCORE.H`. It uses the existing guarded DOS runner, installed
DOSBox-X, an explicit external Microsoft C 6.0/LINK4 toolchain, and the shared
emulator lease. Both production and failure-injection builds use `/G0 /AS`
and an 8 KB stack. The consumer package contains production `SETUP.EXE` only.

```
python BUILD.PY --dosbox <installed-dosbox-x> --toolchain <owned-tools> \
  --lease <shared-lease> --out <new-build-directory>
```

`SETUPTST.EXE` accepts `/FAIL:n`, injecting a failed commit after `n` file
writes. Production rejects this option. Native tests should cover clean
discovery with Windows absent from PATH, install, same-version repair,
missing/damaged payload refusal, failed commit and exact rollback,
interrupted-write recovery, foreign-path preservation, and uninstall after
unrelated INI edits. Shared sound and client files remain installed after app
removal. The installer's internal `TPLAY /CHECK` must succeed
before its recovery journal is released.

`python TESTHOST.PY` compiles the real source into a host filesystem harness
using the shared `gcc-ucrt64.py` preflight launcher. It checks INI preservation,
duplicate-key normalization, DOS path/device handling, transaction commit and
rollback, interrupted-file recovery, staging ownership, current owned-file
identities, and installed state. This is separate from native DOS/Windows
acceptance.

## Native self-extracting format

The consumer EXE is the unchanged plain installer body followed by a stored
flat file bundle and a 32-byte footer. DOS loads only the original MZ body.
`SFXCORE.H` implements the portable bounds/name checks used by native Setup
and the host tests. All integer fields below are unsigned little endian.

The footer contains eight bytes `TWINSFX1`, then six DWORD fields: original
EXE length, bundle length, file count, table CRC32, bundle CRC32, and zero.
The bundle starts with `count * 32` table bytes. Each 32-byte entry contains
a zero-padded NUL-terminated 13-byte DOS filename, three zero bytes, and four
DWORD fields: data offset relative to bundle start, byte count, file CRC32,
and zero. File data follows contiguously in table order.

The native reader requires 1-64 distinct flat DOS names, rejects device names,
paths, overlaps, gaps, unknown fields, malformed MZ lengths, missing/truncated
footers, and bad checksums. It validates the complete bundle before creating
an extraction directory. A plain cached installer is accepted only when its
length equals its MZ-declared length; an overlay always requires a valid
footer, including when an adjacent payload list exists.

Extraction exclusively creates a fresh `C:\TWI####` directory, never adopts an
existing directory, verifies each extracted file, and removes only its own
files on normal exit or failure. The embedded package includes the plain
`SETUP.EXE` that is copied into the installed application. Installed launch,
repair, and removal use the same native code and transaction path.
