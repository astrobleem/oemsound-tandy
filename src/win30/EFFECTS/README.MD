# Tandy Effects Lab

A native Windows 3.0 real-mode sound-effects editor for Tandy EX/HX and its
PC speaker. The compact window fits 320x200. It uses the existing unified
SOUND provider and RAWCLK transport; it adds no sound driver or timer owner.

Extract the complete EFFECTS folder. Exit Windows, run the supplied
TWSETUP.EXE once, then run `GO C:\WINDOWS` from the EFFECTS folder in DOS.
Use your actual Windows directory if different. GO starts Effects Lab through
the same guarded launcher used by Winamp and removes its bridge after Windows
returns. Close Effects Lab and exit Windows normally when finished. Stop other
sound producers first. No Python, browser, driver swapping or modern-PC
conversion is needed on the target computer.

Coin, Laser, Boom, Jump and Blip choose original editable presets and preview
them immediately. Random and Mutate advance the patch's saved 16-bit seed;
the same starting patch and action produce the same result. Presets preserve
the selected PSG/Speaker target. They are separate from MIDI instrument banks.

Click a field to select it. Left/Right or the -/+ buttons change it and preview
the result. Up/Down selects another enabled field. Tab/Shift+Tab navigates native
controls; Space activates a focused button. F5 plays from the beginning, Esc
stops and releases sound, F6 pauses/resumes. Pausing is silent and holds the
cursor and sound reservation. Closing or losing activation stops playback.

Pitch and vibrato depth are in Hz; slide is Hz per second. Vibrato is an
integer triangle trajectory, rate in Hz. Attack/Hold/Release and gate lengths
are in 60 Hz ticks. PSG attenuation is 0=loud through 15=silent, in actual 2 dB
steps. Attack ramps from silence to that level, then Hold, then Release back
to silence. The final tick explicitly mutes. Maximum PSG duration is 298 ticks
(4.97 seconds), plus its terminal state. Speaker uses Hold only, up to 100 ticks.

Noise offers white or periodic noise at three fixed rates or derived from tone
2. Pitched noise reserves tone2 + noise and keeps the tone2 output muted.
Fixed-rate noise disables pitch/slide/vibrato controls. The speaker has fixed
physical volume; Attack, Release, Attenuation and Noise are disabled there.
Speaker gating is real on/off gating, not software volume or PWM. Gate-off zero
keeps the sound continuously on until the end. PSG and speaker are separate
selectable targets, not simultaneous layers.

The compiler uses the 3,579,545 Hz PSG clock and its clock/32 tone divisors;
the speaker uses the clock/3 PIT input. Trajectories clamp to 110..7900 Hz for
PSG and 19..7900 Hz for speaker before rounded hardware divisor conversion.
Quantization affects the realized frequency. Noise follows the actual Tandy
hardware, whose feedback differs from some SN76489 emulations; no identical
noise timbre to the web reference is claimed.

Enter an 8.3 `.FXP` filename, then Save or Ctrl+S. Load/Ctrl+O restores the
editable patch including target and random seed. Save asks before replacement
and keeps the previous file as `.BAK`. Invalid loads leave the edit intact.
The text format starts with `TFX1`, then seed, then the twelve numeric fields
in the order declared in FXCORE.H. Values are finite and range checked.
Keep shipped examples as starting points and save your own name.

Export creates a same-stem `.H` file and refuses to overwrite an existing one.
It contains the same compiled 60 Hz states that preview consumes. Include
`lt_audio.h`, the supplied `FXPLAY.H`, then that header in a C89 program. Acquire
the declared `sound.mask` with libtandy for PSG; a speaker sound instead requires
your app's existing leased PIT callback (zero divisor mutes). Set a cursor to
zero and call `FXPTick(&sound,&cursor,audio,pit,context)` once per application
60 Hz tick. It returns 1 while running, 0 after its terminal mute, -1 on error.
Stop with FXPStop, then discard the cursor. Release your own claims afterward.
For a silent pause, freeze the cursor and use lta_layer_pause or your PIT mute;
resume the layer or reapply row cursor-1 with FXPApply before continuing.

The export includes no PCM and runs without the editor. `SOURCE/LIBTANDY`
contains the MIT core needed for the adapter. Preview owns RAWCLK's exclusive
whole PSG service; exported sounds declare mask1, mask8 or mask12 and can be
integrated with your application's ownership. The state trajectories match;
driver setup/mute writes and redundant cached writes can differ.

The web inspiration is [sn_jsfxr](https://harmlesslion.com/sn_sfxr/), with source
in [tursilion/jsfxr](https://github.com/tursilion/jsfxr). This implementation is
an original bounded C89 trajectory compiler, not a port of its PCM synthesizer.
It has no sine/saw, filters, duty, flanger, PCM export, sample-rate selection,
MIDI bank editing, or reference-noise emulation.

Build with `BUILD.PY --dosbox <installed DOSBox-X> --toolchain <licensed MSC6
root> --lease <shared lock> --out <new output>`. The toolchain must provide
MSC6, MASM, LINK4 and Win3 headers/libraries. Never use raw host GCC on this
workstation: `HOSTTEST.PY --libtandy <libtandy root> --out <new output>` uses the
managed GCC wrapper. `RUNWIN.PY` documents the focused native harness. It
requires a private qualified Windows seed; Windows, tools and ROMs are not
part of the downloadable kit. VALIDATE.MD records exact evidence and limits.
