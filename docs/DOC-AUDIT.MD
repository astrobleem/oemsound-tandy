# Consumer documentation audit

Documentation branch: `docs/consumer-sound-guide`, based on main
`5a7366a1431ce189b5215a457d242dd58f0406d8`. This work changes documentation
and one lossless screenshot frame; source code, installed runtimes, archive
bytes and native receipts remain unchanged.

## Scope

The front page now introduces the shared driver and MIDI software before
presenting the apps as clients. One installation guide owns normal DOS Setup,
launch, Repair and Uninstall instructions. Downloads and app launch paths were
checked against the actual current ZIPs and SETUP.C's generated GO command.

Older direct-port, instrument 0.1, speaker-candidate and timing material is
retained in explicitly historical/reference pages. Frozen published source,
package READMEs and their manifests are preserved. Current source-folder
README pages lead to ready-built kits rather than asking consumers to compile
apps or collect drivers.

The current kit does not register a general WinMM midiOut device. Windows
SOUND services, app-local MIDI rendering and prepared playback are described
separately. Current speaker support is no longer labeled a private candidate.

## Findings corrected

- The release URL for TWSETUP.EXE returned 404. The Setup button now uses the
  existing public repository artifact, verified byte-for-byte against the
  qualified 389,040-byte installer.
- The old MiniMIDI guide referred to top-level TINY.MID/TYPE1.MID files that
  are absent from current PITEXPR. The quick start uses the installed
  C:\WINAMP\STREAM.MID instead.
- Old screenshots depicted UX3 rather than the current Winamp. The new
  consumer image is a lossless frame from an existing qualified capture of
  the exact current executable. Beats and Effects images were already current
  and are reused. [Image provenance](SHOTS/CURRENT/README.MD).

## Verification

- All changed/new Markdown relative file links and heading fragments checked.
- Fifteen distinct public links returned HTTP 200. Current Setup, TWXT2,
  MML3AUTH, PITEXPR and EFFECTS bytes match the local qualified files.
  The separate PITDOS ZIP is valid and contains PSGTEST.EXE.
- Runtime names, kit directory layouts, original example names, F3 speaker
  controls, default installation path and GO generation checked from the
  actual archives/source.
- `tools/WINAMP/VERIFY.PY --self-test`: 512 exact identities and corruption
  refusals passed; `TESTBASE.PY`: five tests passed; `TESTPIT.PY`: 81 checks passed.
- `src/win30/BEATLAB/PACKTEST.PY`: 32 tests passed.
- `tools/RELEASE/TESTPKG.PY`: five tests passed in a private exact-Git-byte
  fixture matching CI's `core.autocrlf=false` checkout. The first ordinary
  Windows checkout run hit pre-existing CRLF pin differences; no source
  inventory, expected hash or package was changed to accommodate it.

No new emulator session was needed or launched. No new native or physical
hardware qualification is claimed. Publication requires separate approval.
