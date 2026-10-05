# Reproduce and verify BUDCAP04

Run host commands from this directory with Python 3.11+ on a POSIX host.
Host paths are explicit; no private workspace paths or custom SDL adapter are
required. Use new output directories for build and test commands. Runtime and
source files are the accepted original bytes; host tools are publication aids.

## 1. Verify the exact runtime archive

```sh
python HOST/VALIDATE.PY ../BUDCAP04.ZIP
unzip ../BUDCAP04.ZIP -d /tmp/budcap-runtime
python HOST/VALIDATE.PY /tmp/budcap-runtime/BUDCAP
```

The validator checks the archive identity, all 32 internal manifest hashes,
WZV2/WZM1/CUE structure, speech sample windows and hashes, 84 caption cues and
the two archived complete-run logs. SHA256.TXT uses DOS backslashes; the host
validator normalizes those separators. It intentionally validates this exact
release rather than accepting arbitrary modified media.

## 2. Build the executable

Provide a DOS toolchain root containing `BIN/CL.EXE`, `BIN/MASM.EXE`,
`BIN/LINK4.EXE`, `INCLUDE/` and `LIB/SLIBCE.LIB`. The compiler and assembler are
not included. The build uses unchanged `SOURCE/BUILD.BAT`: MSC6 `/G0 /AS /W3
/Os`, MASM `.8086`, and a 4096-byte link stack. Source is C89-compatible.

```sh
python HOST/BUILD.PY /tmp/budcap-build --repo /path/to/toolchain --dosbox /path/to/dosbox-x
cmp /tmp/budcap-build/BUDTALK.EXE /tmp/budcap-runtime/BUDCAP/BUDTALK.EXE
python HOST/AUDIT.PY /tmp/budcap-build/BUDTALK.EXE
```

The audit additionally needs GNU `objdump` with i8086 support and the generated
`.MAP` file beside the EXE. Reproduction on 2026-10-05 matched the release EXE
byte-for-byte and inspected 6,062 reachable instructions with no post-8088
suspects. This follows direct control flow from entry and MAP code symbols;
indirect targets, timing and physical hardware are outside its scope.

## 3. Run new emulator tests

```sh
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-preview --dosbox /path/to/dosbox-x
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-full --dosbox /path/to/dosbox-x --case full --cycles 3000
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-low --dosbox /path/to/dosbox-x --case full --cycles 240
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-badpcm --dosbox /path/to/dosbox-x --case bad-pcm
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-badcap --dosbox /path/to/dosbox-x --case bad-captions
python HOST/TEST.PY /tmp/budcap-runtime/BUDCAP /tmp/budcap-nocap --dosbox /path/to/dosbox-x --case no-captions
```

Each test copies media into its own isolated guest, writes configuration/logs
and `RESULT.JSON`, checks return to DOS, display restoration, muted PSG/speaker
and expected success/error. Full runs check all 84 updates, four completed
speech clips, no music skips or unexpected drops. Allow about four minutes for
a full movie (default timeout 360 seconds). The portable harness is headless
and does not inject stop keys or take screenshots; accepted interactive stop
and screenshot evidence stays in VERIFY. It does not test perceived sound.

Publication rechecks are saved separately in `RECHECK/` so the accepted
`VERIFY/` evidence is unchanged. Fresh tests cover the corrected 18–42-second
caption/speech preview, truncated-PCM rejection, and missing/malformed
caption-file fallback. No new full 241.4-second
run or physical listening test was performed during publication; those full-run
results above come from the verified archived release logs.

## 4. Regenerate converted media (optional)

Requires a lawfully supplied matching local source AVI, FFmpeg/ffprobe and
Pillow. Nothing downloads the source. No original AVI or full WAV is included.

```sh
python HOST/CONVERT.PY /path/to/WEEZER.AVI /tmp/new-media/WEEZER.WZV
python HOST/MAKEPCM.PY /path/to/WEEZER.AVI /tmp/new-speech
```

The WZV2 converter uses 256×160 at 4 fps, area scaling, packed standard Tandy
RGBI and no dithering. It also writes a contact sheet and conversion manifest.
The speech converter decodes from the beginning, applies 150 Hz high-pass and
2700 Hz low-pass filters, resamples to 6 kHz, then trims the four exact sample
windows. This avoids seek-related short-clip loss. It writes DIALOG.PCM and
CLIPS.JSON without saving an intermediate full WAV. Compare output hashes
against the accepted package; decoder/filter versions can affect bytes.

The unchanged WZM score, CUE and authored caption track are included in the
runtime. The earlier PSG arrangement tools remain under
`../WIN30/host/ARRANGE.PY`; no new automatic transcription or lyric generator
is claimed here. Media conversion was not rerun during publication: identity
of the accepted converted media was verified from package hashes instead.
