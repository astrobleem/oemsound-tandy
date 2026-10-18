# Current UX3 color captures

WAMPCOL.PNG and WAMPCBR.PNG are unmodified emulator frame PNGs captured on
2026-10-18 in the SPKCOL4 native run. No cropping, recoloring, compositing or
generated imagery was applied. They replace the README illustrations, without
replacing the preserved stock-CGA captures or published app binaries.

The disposable machine was an HX 8088 at 4.772728 MHz with 640 KiB, Windows 3.0
real mode, the actual EX WIN.COM copy and the previously qualified 320x200x16
driver selected as TR53216.DRV. The app/provider set is exactly released UX3.
This is separate from the new speaker candidate and pending combined panning
driver work. It does not qualify physical color or speaker mixing.

The helper opened the current player, held each capture state, opened Add song,
cancelled, closed the player and checked SOUND release. All 11 checks passed;
the run completed in 163.985 host seconds, returned to DOS text mode, removed
its owned bridge, stopped its owned guest and released the shared lease. The
source guest image remained byte-identical. Only a fresh disposable clone was
changed. No CF card was accessed.

The player is ready rather than playing in these images. The browser shows the
original synthetic STREAM.MID, PLUCK.TND and CHIME.SND examples. Earlier UX3
tests and the separate owner report provide the playback/UI evidence linked
from the README; these still screenshots do not establish audio continuity.

| Image | Original frame | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| WAMPCOL.PNG | SPKCOL4/FRAME04.PNG | 3137 | a26e41dcee5a65cb237c5a300b378055b7cd70b394301e59f032dac11d48c872 |
| WAMPCBR.PNG | SPKCOL4/FRAME07.PNG | 2908 | fa280ec75cfe17fc7a5949f7bf04050b1d884a44ceafe8a27eadfafa299ce5ab |

| Component | Bytes | SHA-256 |
| --- | ---: | --- |
| MINIPLAY.EXE | 52029 | 7285ee2e3f7ff321643e4efe233941e8eeda8f89a993ff9ae91e1eb998a54fdb |
| RAWCLK.DLL | 10814 | 0a3528ee7729b50b4bb69d2d4d3caca5c1b92c260030ac29b80ba148975948e0 |
| MIDICLK.DLL | 11062 | 0c7a6d5a104437d619bb7835fda28dde1266a16b8c3ab7a2d702487b51f4215c |
| MCITNDY.DLL | 11413 | e495d11353536720abdeb2fadb7ef8e5d40fa489a8ec4d4ca8dbd8d33911c258 |
| LWFILE.DLL | 14876 | c7041c86d9012795ddfba8de96ee225a7a709f86e5a1c39ecd516a5216cb5d5c |
| TSOUND.DRV | 6168 | 7461d227846830f3ff604c40cf31ad0e3fb34122a13de6269e86de02cc4209e5 |
| TR53216.DRV | 55040 | 8e103259da65334c1c374c5d5646384f03a3eea8f2831645e5062135e10c878e |
| Capture helper | 5357 | 9f1827fd7178bdb768d85a53b381e7a59e7b068bb8468bf2c475b993c8b7f2ca |

[Machine-readable image pins](WINAMPCL.JSON) ·
[UX3 source and validation](../../experimental/WINAMP30/VALIDATION-UI.MD)
