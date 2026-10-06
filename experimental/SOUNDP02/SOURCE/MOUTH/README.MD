# Tandy Automatic Mouth: tiny first experiment

MOUTH.EXE is a native Windows 3.0 real-mode / 8086 application for known
Tandy PSG hardware. It is an experimental four-word sound toy, not general
text-to-speech. HELLO, TANDY, YES and NO have hand-authored sound/pose frames.
Choose one of those four words in the drop-down list, then Speak. There is
no free-text entry or suggestion that arbitrary phrases are supported.

Copy MOUTH.EXE into the guest and run it from Program Manager in Windows
3.0 real mode (WIN /R). It needs no custom sound-driver installation. The
ZIP omits Windows, Microsoft build tools and the supplied reference image.
The separate WAV is an actual emulator capture for listening on the host.

Use Speak or Enter; Stop or Escape mutes immediately when the message is
handled. Repeat Speak restarts. The mouth follows the same frame that writes
the sound registers. Closing, normal completion, timer failure and shutdown
mute all four channels before releasing the sound owner. Only one instance
is allowed. No installation or change to WIN.INI is required.

The drawn character is an original tiny GDI interpretation of the user's
Color Computer 3 mascot reference: a smiling CRT, keyboard body, arms and
shoes. In 16-color modes it has stepped rainbow bands. Low-color modes use a
plain background and black/white body. It uses Windows primitives, no bundled
bitmap or image decoding library. A reusable 148x172 device-compatible
bitmap caches the static character and five mouth poses in its unused top
rows (about 13 KB of pixels at 4 bpp), so
repainting does not repeatedly reconstruct every key, limb and curve. The
bitmap and its memory DC are released when the window is destroyed. The full window is 160x198; a 120x80 client
was not sufficient for usable native controls plus this character.

## Sound limits

The three PSG oscillators are square waves. They are NOT independently
filtered vocal-tract formants. The profiles combine rough pitch clusters and
short noise bursts to test vowel/consonant cues. Intelligibility is unproven;
expect buzzy electronic syllables, not a clear artificial voice. No sample
DAC, PWM speech, IRQ/PIT/PIC hook or CPU busy wait is used.

At an assumed 3.579545 MHz clock, tone frequency is clock/(32*period), with
10-bit periods 1..1023. Volume is a four-bit attenuation value; 15 is off.
Noise selects periodic/white character plus clock/512, /1024, /2048 or tone-3
clocking. This version uses only white noise at clock/512; it does not claim
arbitrary noise frequency. PSG variants in real Tandy models differ in noise
feedback. Physical Tandy hardware has not been tested.

Primary hardware reference: Texas Instruments SN76489AN data sheet scan,
https://lekstutis.com/Artie/CHROMAtrs/SN76489AN_Specs.pdf
Emulator implementation and variant notes:
https://dosbox-x.com/doxygen/html/sn76496_8cpp_source.html

Timing uses yielding Windows SetTimer messages with roughly 55-ms resolution.
The first frame waits until the startup/repaint work has yielded. Transitions
are stage-relative, so slow painting can lengthen a word. A frame delayed by
more than 1.1 seconds is aborted and muted instead of playing overdue bursts.
Small mouth-only invalidation and cached-pose BitBlt avoid reconstructing
the character or rasterizing an ellipse each tick. Cache allocation failure
falls back to primitive drawing, with the same late-frame safety cutoff.

## Sharing the PSG

Before direct PSG output MOUTH acquires OpenSound ownership and stops its
empty Windows sound queue. It refuses while the known PSGPLAY or TCHIME
windows exist. It releases ownership on Stop/end. This cooperates with the
validated TSOUND driver, but cannot prevent an unrelated direct-I/O program
started later from overwriting hardware. Do not run independent PSG demos
concurrently. A failed ownership request does not mute the other owner.
The Tandy ROM signature is a compatibility guard, not exact model detection.
Protected mode is rejected before hardware access.

## Build and tests

Use Microsoft C with /G0 /Gw /AS /W3 /Od and the Win16 SDK imports. BUILD.BAT
runs inside DOS with the existing toolchain mounted as C:. BUILD.PY wraps an
isolated DOSBox-X build and accepts --repo, --dosbox, --out (new directory).
It does not download or package Microsoft tools or Windows guest files.

TESTWIN.PY clones existing user-owned five-mode Windows guests; it never
modifies them. Cases /1 through /9 exercise completion, Stop, repeated Speak,
empty selection, active Close and all four word choices. The old invalid
text test modes now exercise the no-selection guard. VERIFY.PY
checks native register/frame logs, final four-channel mute, captured PCM and
absence of oscillation after playback. The emulator mixer can leave a small,
slowly decaying DC offset, so the tail check measures adjacent-sample changes
rather than incorrectly demanding every PCM sample be exactly zero. Diagnostic modes write C:\MOUTH.LOG and exit Windows;
normal launch writes no log and does not exit Windows.

The emulator uses 8086_prefetch, 640 KB and no XMS/EMS/UMB. Fixed cycle counts
are an emulator setting, not a claim of measured physical 4.77-MHz performance.
