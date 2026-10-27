# Developer guide

For installation and normal use, start with [the consumer guide](../README.md).
These links cover source ownership, builds and measured validation.

## Source map

| Component | Entry point |
| --- | --- |
| Unified Windows sound provider | [DRIVER](../src/win30/DRIVER/README.MD), [ABI](../src/win30/DRIVER/ABI.MD) |
| Current player | `src/win30/PLAYER`; [native build batch](../src/win30/PLAYER/BLDPLAY.BAT), [current native receipts](../tools/WINAMP/TESTS.JSN) |
| Shared MIDI/transport and UI helpers | [libwintandy](https://github.com/astrobleem/libwintandy), [Windows engine mirror](../experimental/WINLIB30/README.MD) |
| Portable PSG core and export support | [libtandy](https://github.com/astrobleem/libtandy) |
| Beats and its paired mapper | [BEATLAB source/tests](../src/win30/BEATLAB/TESTS/README.md), [validation](../src/win30/BEATLAB/VALIDATE.MD) |
| Fiddle | [source/build notes](../src/win30/FIDDLE/BUILD.MD) |
| Effects Lab | [compiler/export/build notes](../src/win30/EFFECTS/BUILD.MD), [validation](../src/win30/EFFECTS/VALIDATE.MD) |
| Native DOS installer | [Setup interface and build](../src/win30/SETUP/README.MD) |

Read [AGENTS.md](../AGENTS.md) before changing source. Use licensed period
tools for native builds, fresh private output and the shared emulator lease.
On Chad's Windows workstation, host C checks use the managed GCC wrapper
specified there. Consumer installs do not require any of these tools.

## Behavior and evidence

[Formats](FORMATS.MD) · [Timing and ownership](SNDTIME.MD) ·
[Speaker support](SPEAKER.MD) · [Current screenshots](SHOTS/CURRENT/README.MD)

The [documentation audit](DOC-AUDIT.MD) records the current consumer-guide checks.

Host tests, native builds, emulator capture and physical-hardware reports
have different scopes. Keep exact runtime/source identities with claims.
Older experiment and package directories remain snapshots; use current
maintained source or the matching source inside the current ZIP.

## Licensing

Keep the [root license](../LICENSE) and each component's notices. The sound
driver and older clients retain their own licenses; Winamp/shared code,
first-party browser and libtandy components carry their included MIT notices
where applicable. Historical media has separate rights and is not part of
the normal sound/player download. Windows, ROMs and proprietary build tools
are not supplied.

## Earlier work

[Original direct-PSG milestone](../src/win30/PSGPLAY.MD) ·
[Earlier sound experiments](../experimental/HISTORY.MD) · [Demo projects](DEMOS.MD)
