# Windows 3.0 player source adoption candidate

This isolated MINIPLAY (Tandy-Winamp) source uses the declaration-only
`LWAUDIO.H` facade and the existing app-local MCITNDY/MIDICLK/RAWCLK providers.
MIDI uses the MCI-compatible path; TND/AGI uses the raw player. The provider
and IRQ implementation, event formats and exclusive ownership are retained.
There is no replacement scheduler or new timer owner.

The source is copied byte-for-byte from the AUDIO component of
[libwintandy commit 80b1431](https://github.com/astrobleem/libwintandy/tree/80b1431b9407e7fb14392804d4fdc9b10d9d1bab/libtandywin/AUDIO),
with its notices and build/test recipes. See [STATUS.MD](STATUS.MD) for the
current adoption evidence. Historical FACADE notes retain their original
source-candidate status; they are not a new runtime qualification report.

Evidence has two dates/scopes: the source archive and `current_source` pins in
`PROVENANCE.JSON` describe the verified R4/source-only adoption snapshot. The
later paired source build and native preview are recorded separately in
`STATUS.MD` and `BUILD/PAIR-EVIDENCE.JSON`. `FACADE/EQUIV.JSON` remains a
preprocessor expansion comparison; its `native` field is scoped to that
comparison and does not supersede the later build records.
The exact original synthetic input identities used by the recorded GUI case,
including test-only files not in ordinary `RUNTIME`, are listed in
`TESTS/NATIVE/FIXTURE-INPUTS.JSON`. The 58-check harness also uses test-only `STRESS.TND`, `TYPE0.MID`,
`FX.TND`, `BAD.TND`, and `FAST.TND`. Run
`python TESTS/NATIVE/MAKEGUIQA.PY EMPTY_DIRECTORY` with an existing empty
directory to recreate those five inputs. The script verifies its fixed
SHA-256 pins and refuses missing or non-empty destinations. These are
original synthetic QA fixtures, not catalog or third-party media, and
remain outside ordinary `RUNTIME`.

The bounded [native preview archive](WINLIB30.ZIP?raw=1) now includes the
tested program set, corresponding source, install/build guidance, original
synthetic QA fixtures (`CHIME.SND`, `PLUCK.TND`, and `STREAM.MID`), native logs
and an actual capture. Those three files are original test material, not a
commercial-song library or representative catalog. It requires the exact
external TSOUND already published in PITWIN; that driver is not bundled.
This directory does not replace old Mini MIDI, public PIT kits or their
pinned files. It makes no permanent Windows configuration changes and includes
no Microsoft OS/SDK files, tools, fonts, ROMs, commercial or third-party songs,
or third-party sample material; its original synthetic QA fixtures are named
above. Build with
your separately owned period Microsoft C/MASM/LINK tools inside DOSBox-X,
following `BUILD/PLAYER30.PY --help`. Preserve the emulator lease rules in
a shared test environment. Host Python is a build/test tool, not a guest
runtime dependency.

The matching provider set and supported real-mode Tandy HX fixture are
required for native testing. PCjr audio remains pending. The UI is based
on USER/GDI; that does not qualify every sound target. MIDI, raw and queue
limits remain in the versioned API/provenance notes. MPLAYER/full MMSYSTEM,
MOD/VGM/VGZ player inputs and automatic speaker bass fallback are not
implemented by this source snapshot.

Project source uses the included MIT license and embedded notices. Retain
the Jan Knipperts format/table MIT notice in LICENSE-TNDY.TXT. No GPL Mini
Piano/QPIANO implementation, commercial song, or third-party song is included.

`BUILD/PLAYER30.PY` and `PLAYER30.BAT` are the six-product player/provider
recipe. The separate paired adoption record covers eleven products (the six
player/provider outputs plus five raw QA programs). Those are distinct build
scopes, not conflicting counts; the paired run is compile/link evidence and
does not by itself establish runtime or hardware behavior.

The evidence-corrected WINLIB30.ZIP includes TESTS/NATIVE/MAKEGUIQA.PY.
Its runtime files are byte-identical to the original 58-check preview. Native
validation is recorded author-run evidence, not public CI or a reviewer rerun;
no native test was rerun for this evidence/documentation correction.
