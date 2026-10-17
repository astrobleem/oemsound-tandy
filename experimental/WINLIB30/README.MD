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

The bounded [native preview archive](WINLIB30.ZIP?raw=1) now includes the
tested program set, corresponding source, install/build guidance, original
synthetic scores, native logs and an actual capture. It requires the exact
external TSOUND already published in PITWIN; that driver is not bundled.
This directory does not replace old Mini MIDI, public PIT kits or their
pinned files. It makes no permanent Windows configuration changes and includes
no Microsoft OS/SDK files, tools, fonts, ROMs, songs or samples. Build with
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
Piano/QPIANO implementation or original songs are included.
