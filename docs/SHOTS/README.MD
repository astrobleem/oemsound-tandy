# README screenshot provenance

Historical README captures below came from the repository and Mini Piano
0.1 package. Their still PNGs were reused byte-for-byte; reuse adds no new
runtime test, redraw, retouching or physical-hardware evidence. The BUDCAP
animated preview and the new MINIPLAY source-adoption capture each have
separate provenance below. All reused and newly captured content was
visually inspected. Guest DOS example paths are not host user paths.

Historical Source revision: `d7fd911d6f0213244cd3f89d01c83b739d4297c4`.

| Asset | Original source | Scope |
| --- | --- | --- |
| [BEATS.PNG](BEATS.PNG) | [artifacts/win30/BEATS/EVIDENCE/beats-baseline.png](../../artifacts/win30/BEATS/EVIDENCE/beats-baseline.png) | Native 320x200x16 sequencer GUI; audio qualified separately in VALIDATE.MD. |
| [MOUTH.PNG](MOUTH.PNG) | [artifacts/win30/MOUTH/SCREEN.PNG](../../artifacts/win30/MOUTH/SCREEN.PNG) | Native mascot GUI capture; no clear-speech claim. |
| [PSGPLAY.PNG](PSGPLAY.PNG) | [artifacts/win30/EVIDENCE/FINALGUI/kernel_000.raw1.png](../../artifacts/win30/EVIDENCE/FINALGUI/kernel_000.raw1.png) | Native final stopped/muted GUI frame; captured audio and register evidence are separate. |

Source evidence: [Beats qualification](../../artifacts/win30/BEATS/VALIDATE.MD), [Mouth instructions](../../artifacts/win30/MOUTH/README.TXT), and [PSG milestone](../../artifacts/win30/README.md).

The pictures establish only the pictured UI state, not audio, complete
application coverage, physical hardware, unpictured updates or timing.
Historical frames retain their original scope. Stock Windows applications
shown in captures are compatibility examples; their binaries are not included.

The new MINIPLAY section below records an actual source-adoption runtime
capture and its lossless rectangle crop. Historical PNG identities and
version labels above remain unchanged.

## SHA-256 identities

The source and copied PNG hashes match exactly:

```text
4e8830c9fb7c69cbcbb055e5ffbc4e819eeed8ba67891855e5dec09eb99709f4  BEATS.PNG
b0b3fa3808da26b1c3f3500813fc5cd60aafb3c239dc8e466a0c66ba136ce8c4  MOUTH.PNG
444a3ae03ff72b153acaab8327e37f95b001b6fac2534cc8733e72f8b61c91da  PSGPLAY.PNG
```

## Mini Piano 0.1

[PIANO.PNG](PIANO.PNG) is byte-identical to the final Mini Piano package's
SCREEN160.PNG. DOSBox-X, Windows 3.0 real mode, logical 160x200x16 (BR160),
320x200 image from emulator horizontal doubling. The thirteen keys, octave
buttons, Panic and held-note indicator are native rendered UI. This frame
alone does not establish audio, physical keyboard input or hardware behavior.

MINIPIAN.EXE is 15,327 bytes, SHA-256
`22cab48dce6baba821a72689fcaee848b3bd3fe4e9b9da659f470047abe6b4fc`.
The package's VALIDATE.TXT identifies the final native screenshots and records
seven DOSBox-X flows. Source was published in
[Mini Piano PR 23](https://github.com/astrobleem/oemsound-tandy/pull/23),
commit `41c0e96477eaaeae45bd686e2360a73889f5439d`.

PNG SHA-256:
`c071d6a93d25338af3d37675d652847c9068a5d5175759c871e37de8122b594b`.


## BUDCAP 0.4

[BUDCAP04.PNG](BUDCAP04.PNG) is a lossless PNG encoding of the accepted
[BUDCAP 0.4 FISHCAP.BMP](../../examples/BUDDY/BUDCAP/VERIFY/FISHCAP.BMP),
retained at source revision `c7641deeaf9234116993465064e8dd9c2c23cd35`.
The 320x200 frame shows the 256x160 video area and opening fish-dialogue
caption during sampled speech in the emulator. Its dimensions and decoded
RGB pixels are identical to the BMP; no crop, scaling, redraw or retouching
was applied. This format conversion adds no new runtime test, listening
result, physical-hardware evidence or timing claim. See the
[accepted QA scope](../../examples/BUDDY/BUDCAP/QA.TXT).

- Source BMP SHA-256:
  `bba6f027f0600955b97f3572a251d442d89ddda9ec4abce17e2966f9b72bb1fb`
- PNG SHA-256:
  `82551c993463711807953240ddb45643404e9f8f39eef9ee862a2ee11dc4355b`
- Decoded RGB pixel SHA-256 (both files):
  `be43571a09d8d7ad7fb28976330957b040f15f04282581979edf3d8edbb65e04`


## BUDCAP 0.4 animated preview

[BUDCAP04.GIF](BUDCAP04.GIF) is a six-second silent excerpt from a genuine
DOSBox-X runtime capture made on 6 October 2026. It uses the published
[BUDCAP04.ZIP](../../examples/BUDDY/BUDCAP04.ZIP), validated against its
32 internal manifest hashes, from public main revision
`b25d5bce188cf63a3eb7a5ae9bab68147a862cd2`.

- Executable SHA-256:
  `2e866172774626deec2c699ba7df7738671feb9357e752977429c5aebbb38986`.
- DOSBox-X 2025.12.01 Windows SDL1, `machine=tandy`, normal core,
  `cputype=8086_prefetch`, fixed 3000 cycles, 640 KB, XMS/EMS/UMB disabled.
- Command: `DX-CAPTURE /V BUDTALK /B /P /L` in a fresh extracted guest.
  Capture excerpt: 17.8-23.8 seconds into the 18-42-second preview run
  (approximately movie time 35.8-41.8 seconds).
- Native 320x200 framebuffer, sampled at 4 fps: 24 frames, 250 ms each,
  looping, 96,530 bytes. All decoded GIF RGB pixels equal the sampled
  lossless ZMBV capture frames. No crop, scaling, dithering or retouching.
- GIF SHA-256:
  `58c9193e3f8454326d1563e215a9972d8e0557cde936f97f1430c6535dbfab99`.
- Source runtime AVI SHA-256:
  `0144c622f44329f26560047a8ae9c98f77ef5b08207df69e703d0eb73756fb56`.

The new preview completed with error 0, ten caption updates, two speech
clips, zero skipped music events and zero unexpected video drops. Normal
cleanup muted PSG/speaker and restored display mode. This is a short emulator
visual check, not a new full-run, listening or physical-hardware qualification.
The GIF contains no sound. The existing [media attribution and rights
notice](../../examples/BUDDY/ATTRIB.TXT) applies to its underlying video.

## MINIPLAY source adoption candidate

[MINIPLAY.PNG](MINIPLAY.PNG) shows the actual "Tandy Queued" window with
STRESS.TND active. It was captured on 17 October 2026 during the fresh
MINIPLAY/MCITNDY/MIDICLK/RAWCLK adoption's native GUI run. The original test
score is synthetic; no third-party song or sampled media is shown.

- Windows 3.0 real mode, stock CGA 640x200, 640 KB Tandy HX 8088 profile
  configured at 4,772,728 Hz in the private 86Box-based FIXBOX fixture.
  Emulator configuration is not calibrated physical timing.
- MINIPLAY.EXE: 22,109 bytes, SHA-256
  `00dc26526fd78be1c2f18f713e51390e3f07229697ee4f03d8e7e971dd740654`.
- Actual captured frame: 656x400 pixels, SHA-256
  `098f7388d2097597b9b784a43ba8526d9e747602f65342b6e4aea51af48b3c53`.
- Lossless crop: source rectangle `(8,38)-(304,234)`, right/bottom exclusive,
  producing 296x196 pixels. No scaling, redraw, retouching or generated UI.
- Published PNG SHA-256:
  `f43aefae8dc0a5c395fca8dd783a3deaa275ddff6ad3e6a3a74f6c4e0df48d2c`.

The separate native log records zero failures in 58 lifecycle/UI checks,
equal free memory before/after (292,992 bytes), successful ownership release,
restored BIOS/INT2F vectors and clean DOS return. The image itself establishes
only the pictured state. No listening result or physical-hardware capture is
claimed. Source is in [libwintandy](https://github.com/astrobleem/libwintandy);
the bounded native preview is under review in
[PR 37](https://github.com/astrobleem/oemsound-tandy/pull/37).
