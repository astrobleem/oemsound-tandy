# DWA Beats Lab

**DWA: DAWs With Attitude.** Straight Outta LLMompton.
Produced by Dr. DRAM. Mixed on five channels. Mastered on absolutely none.

A small, native Windows 3.0 music tool for the Tandy 1000 EX/HX: edit an
eight-step pattern with three PSG tones, noise percussion and an optional
PC-speaker lane. The editor fits a 320x200 desktop. No samples or extra
sound card are needed.

## Start making noise

Use the complete Fiddle/Beats authoring kit from the
[current downloads](../../../docs/DOWNLOADS.MD). Extract the kit, exit
Windows to DOS and run its included `TWSETUP.EXE` once. Start Windows
through your normal display launcher and run `BEATS\BEATS.EXE` from the
extracted kit. [Shared setup instructions](../../../experimental/PITPUBLIC/INSTALL.TXT).

Keep the supplied application folders intact. `MIDIMAP.DRV` is app-local
MIDI synthesis support, not an alternative system sound-driver installation.
It cooperates with the single installed SOUND driver for sound ownership
and uses that driver's optional PC-speaker service. Setup supplies the
shared driver; no file gathering, driver swapping or INI editing is needed.
Stop other music/speech producers before playing.

## The grid

Each column is one straight eighth note. Click a cell to toggle its note
and rest; arrow keys select a cell, Space or Enter toggles it, and `+`/`-`
changes its pitch. Delete makes a rest. Click the three tone labels to
cycle through the existing presets. The fourth row is noise percussion;
`PCspkr` is the fifth, separately editable row.

The visible PIT control enables or disables the PC-speaker row. A pattern
and its demos load that setting automatically; Save/Load preserves it.
Four-voice patterns keep it off. The speaker has fixed hardware level;
it does not use the PSG preset envelopes.

Play starts at step 1. Stop or Escape mutes and releases sound; `P` and `S`
work when the grid has focus, and F5 toggles playback. Losing activation or
closing the app also stops playback. Tab/Shift+Tab navigate native controls.
Use the tempo `-`/`+` buttons or Page Down/Page Up (40-240 BPM).

## Demos with a pocket

Click Demo to cycle through these original, editable examples. Their names
appear in the title; their filenames appear beside Load and Save.

| Demo | File | What to listen for |
| --- | --- | --- |
| Pocket drums | `POCKET.BTS` | A relaxed 90 BPM kick, hat and snare groove using the noise row. |
| Lowrider lead | `LOWRIDER.BTS` | Sparse high lead, organ replies and low PSG bass at 92 BPM. |
| Speaker bass | `FIVE.BTS` | Five voices together, with low speaker-only answers. PC speaker loads **on**. |
| Late-night keys | `NIGHT.BTS` | Short lead phrases, bell accents, bass and crisp percussion. |
| Starter | `STARTER.BTS` | The original four-voice starter pattern. |

The demos take inspiration from the relaxed, sparse G-funk feel of
*The Chronic*. These are original educational patterns, not album
recordings, recognizable song melodies or lyrics.

This compact sequencer has straight eighths and a fixed 75% gate, without
swing, glide, ties or per-cell velocity. Its shipped mapper accepts melodic
notes A2-C7; the lowest speaker note is A2 (110 Hz). The bass demo uses that
actual lower limit and leaves space around it. It does not silently double
or fold the bass into a higher voice.

## Save, load and share

Enter an 8.3 filename such as `MYGROOVE.BTS` in the filename box. Save stores
all 40 cells, tempo, tone presets and the speaker setting beside the app.
Load opens that pattern for further editing. Ctrl+S saves; Ctrl+O loads.
Replacing a pattern asks first and keeps the previous file as `.BAK`.
A rejected or damaged file leaves the current pattern intact.

MML switches to the generated, read-only score preview; Grid returns to the
cells. Copy puts that score on the clipboard. Export writes a same-stem
`.MML` file for Fiddle: MML2 with speaker off, MML3 plus `[P]` with it on.
Existing MML files are protected from overwriting; choose another filename.
The grid loads `.BTS` patterns, not arbitrary MML scores.

The status line explains missing files, busy sound and disk errors. If
Setup is needed, use the one supplied with the kit. About/F1 shows the key
controls without a splash screen or startup delay.
