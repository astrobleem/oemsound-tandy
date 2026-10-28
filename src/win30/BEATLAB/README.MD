# Tandy Beats Lab

Make an eight-step song with three PSG tone lanes, noise percussion and a PC
speaker lane. Import Effects Lab patches as instruments and effects, then save
one editable song.

Extract the complete BEATSFX folder. Exit Windows to DOS and run its included
TWSETUP.EXE for shared sound setup. Run `GO` (or `GO C:\WINDOWS`) from DOS to
start Beats. `GOFX` starts the included Effects Lab in the same folder. Both
apps share the installed SOUND driver. Stop one before playing the other.

Click Demo to cycle the five original patterns and **Effects groove**. The
last demo combines a laser, pitched-noise explosion and speaker jump with
normal notes. Click Play. Pause/F6 holds the position silently; Resume/F6
continues. Stop/Esc releases sound. Play restarts at step1; F5 toggles Play/Stop.

Click a grid cell to toggle a note/rest. Arrows select; Space/Enter toggles;
Pitch-/Pitch+ or +/- changes pitch; Delete makes a rest. Click tone labels to
cycle the original eight presets for normal notes. PIT switches row5 on/off.
Tempo +/- or Page Down/Up changes BPM. Edits are heard on the next Play.

To use a patch:

1. Select the destination cell. Tone patches go in rows1-3, noise in row4,
   speaker in row5.
2. Enter `COIN.FXP` in the filename box, or a path such as
   `C:\EFFECTS\COIN.FXP`, then click **Import**. That cell becomes an FX
   trigger and the patch is assigned to its lane. Speaker imports enable PIT.
3. Select more cells and click **FX step** to trigger the same patch. **Note**
   turns the selected FX cell back into a normal note. `F60` means the original
   patch pitch (C4). Pitch buttons transpose it; fixed noise ignores pitch.
4. Enter a song name such as `MYSONG.BTS`, then Save/Ctrl+S. The song contains
   the patch parameters and triggers, so the source FXP is no longer needed.

A new trigger replaces the previous effect in that lane. A rest lets an effect
finish. Pitched noise temporarily borrows tone2 and mutes its audible output;
its current note/effect returns afterward. Speaker effects temporarily replace
speaker notes. Stop/Pause apply to the whole song. A busy speaker refuses Play;
loss of its lease stops playback. Songs loop for up to25 minutes per Play.

Load/Ctrl+O restores the complete song. Old .BTS patterns still load. Save asks
before replacement and keeps the previous file as .BAK. Invalid input leaves
the current edit intact. The supplied FXGROOVE.BTS is ready to play.

MML/Grid shows the score for normal patterns; Copy and Export work as before.
Imported patches require .BTS playback: MML cannot represent their trajectories,
so export clearly refuses songs with active FX steps. Fiddle and MIDIMAP.DRV
are included for normal MML scores. About/F1 shows key controls; Tab/Shift+Tab
moves between buttons.

[Integration and source details](FXINTEG.MD) - [Original demo reference](REFERENCE.MD)
