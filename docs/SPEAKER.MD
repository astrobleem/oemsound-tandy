# Current PC-speaker support

The shared SOUND driver provides a task-owned square-wave voice and a
prepared PIT2 lease. Existing MIDI/RAWCLK integration schedules that output
alongside PSG sound; Effects Lab reused those interfaces without changing them.

For normal use, see [the app guide](APPS.MD#which-voices-do-i-get).
Beats/Fiddle explicitly author a fifth part; Mini Piano/JoyMIDI/MiniMIDI expose
their kit's speaker control. Current Winamp uses speaker bass with PSG fallback.
Effects selects PSG or speaker as separate targets.

The speaker has fixed physical amplitude and on/off gating. This support is
square-wave music, not PCM/PWM sample playback. Speaker music and sampled
speaker speech share PIT2/gates and cannot own them simultaneously.

[Driver API](../src/win30/DRIVER/README.MD#pit2-extensions) ·
[Current Winamp receipts](../tools/WINAMP/TESTS.JSN) ·
[Effects validation](../src/win30/EFFECTS/VALIDATE.MD)

The [earlier UX3 integration report](SPEAKER-UX3.MD) is preserved as historical
evidence; its description of an unpublished candidate does not describe the
current downloads.
