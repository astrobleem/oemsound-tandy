# Tandy Winamp formats

[Download Tandy Winamp](https://github.com/astrobleem/oemdisplay-tandy/releases/download/windows-xt/TWXT2.ZIP)
and follow the [native Setup guide](../tools/WINAMP/INSTALL.TXT). Setup installs
the player, shared SOUND driver, DLLs and launcher. Start with `C:\WINAMP\GO`.

| Input | Playback |
| --- | --- |
| MIDI | Standard MIDI File types 0/1; sustain, volume, expression, pitch bend and controller resets. Three PSG tones plus percussion, with low bass routed through the leased PC speaker and automatic PSG fallback. |
| VGM | Ordered Tandy SN76496 writes and waits, streamed from disk. The initial traversal and one loop repeat retain source timing and explicit noise writes. |
| VGZ | The same VGM path after automatic gzip expansion. Opening shows progress and supports Cancel; temporary files require no user configuration. |
| MOD | `M.K.` and `4CHN` music arranged for PSG square waves, using up to three audible tone lanes. Sample timbres are not reproduced. |
| TND / AGI SND | The current bounded raw-format readers and their existing schemas. |

VGM/VGZ/MOD support Pause and Resume. Stop, EOF, switching files and Close
release sound ownership. Close also safely cancels an open in progress.

## Supported VGM stream

The parser accepts the Tandy SN76496 profile at 3,579,545 Hz, feedback 6,
15-bit shift register and zero variant flags, with VGM 1.00–1.71 syntax.
Other chips, stereo commands, data blocks and PCM are refused before sound
acquisition. There is no automatic chip conversion.

The plain or expanded stream may be up to 16 MiB. Total playback fits the
existing RAWCLK timestamp horizon, about 30 minutes including the loop.
Ordered writes at a timestamp form one group of at most 16 bytes; distinct
groups require at least 9,546 PIT cycles, the ceiling of 8 ms. Unsupported
commands or timing produce a specific refusal.

VGZ checks gzip CRC32 and decoded size. Stored, fixed-Huffman and dynamic-
Huffman DEFLATE are supported. Concatenated members, trailing data and gzip
header CRC16 are outside this profile. Expansion uses a 32 KiB FAR history
ring only during opening, then replays the managed disk file.

## Evidence and source

The [current package test record](../tools/WINAMP/TESTS.JSN) binds the native
player, source, exact ZIP installation, full generated large-VGZ loop and
cleanup checks. Host results and emulator results retain separate scopes;
physical hardware and listening are separate from those records.

[Canonical format implementation](https://github.com/astrobleem/libwintandy/blob/main/libtandywin/AUDIO/FORMATS30.MD)
describes the current parser, buffering, MOD conventions and build costs.
Original songs and accepted musical data are unchanged.
