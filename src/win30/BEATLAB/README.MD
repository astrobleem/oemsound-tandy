# Tandy Beats Lab

Build an eight-step pattern with three PSG tone lanes, noise percussion and
an optional PC-speaker lane. Save it, vary it and export a score for Fiddle.

![Five-lane Beats Lab editor](../../../docs/SHOTS/BEATS/EDITOR.PNG)

[Download a music kit](../../../docs/DOWNLOADS.MD) ·
[Shared setup and launch](../../../docs/INSTALL.MD#add-music-tools)

## Start with a demo

Click **Demo**, then **Play**. The original patterns cover Pocket drums,
Lowrider lead, Speaker bass, Late-night keys and Starter. Each is editable.
The Speaker bass demo turns the PIT speaker lane on automatically.

Click a grid cell to toggle a note/rest. Arrow keys select a cell;
Space/Enter toggles it. **Pitch−/Pitch+** or `+`/`−` changes its pitch, and
Delete makes a rest. Click a tone lane's label to cycle its instrument preset.

Use **PIT** to enable the separately editable PC-speaker row and the tempo
buttons to change speed. **Play** starts at step 1; **Stop/Esc** releases
sound. F5 toggles playback. Tab/Shift+Tab navigates the buttons.

## Save a pattern or share a score

Enter a name such as `MYGROOVE.BTS`. **Save/Ctrl+S** stores all 40 cells,
tempo, tone presets and speaker setting beside the app. **Load/Ctrl+O** brings
it back. Replacing a pattern asks first and keeps the previous file as `.BAK`.

**MML** shows the generated score; **Grid** returns to the cells. **Copy** puts
the score on the clipboard. **Export** writes a same-name `.MML` file for
[Fiddle](../FIDDLE/README.MD), using MML3 when the speaker part is enabled.
Choose another name if an export already exists.

Both MML3AUTH and PITEXPR contain this same editor and its original examples.
MML3AUTH adds Fiddle; PITEXPR adds the instruments. **About/F1** shows the key
controls and the app's DWA: DAWs With Attitude label.

[Detailed demo and pitch reference](REFERENCE.MD) ·
[Source/tests](TESTS/README.md) · [Native validation](VALIDATE.MD)
