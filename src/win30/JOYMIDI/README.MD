# JoyMIDI

Turn the Tandy joystick into a note-and-velocity controller for the shared
sound driver. The ready-built app is included in **PITEXPR.ZIP**.

[Download](../../../docs/DOWNLOADS.MD) · [Shared setup](../../../docs/INSTALL.MD#add-music-tools)

Run `C:\PITEXPR\JOYMIDI.EXE` in Program Manager.

1. Press **P** to choose the joystick port if needed.
2. Choose **Range** and slowly sweep the joystick through its full range.
3. Put it at the center, then choose **Center**.
4. Choose **Arm**, release button 1 once, then hold button 1 to play.

Left/right selects pitch; up/down selects velocity for the next attack.
Release button 1 to stop. **Panic**, Esc or button 2 mutes and disarms.
F3 selects the optional speaker support in the current kit.

Calibration is per session. An absent joystick, invalid range or timing
timeout leaves the instrument muted; recenter and Arm to resume. It sends
notes to the included Tandy renderer rather than an external MIDI port.

Current matching source is in the kit. The original 0.1 [source and detailed
input notes](LEGACY.MD) retain their versioned validation limits.
