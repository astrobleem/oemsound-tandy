# Current sound timing and ownership

Consumer setup and controls are in [the app guide](APPS.MD). This page describes
the current playback paths; [the earlier UX3 report](SNDTIME-UX3.MD) retains its
original timing measurements and version scope.

| Path | How output is scheduled |
| --- | --- |
| Winamp MIDI | The MIDI service renders prepared PSG/speaker states for the existing RAWCLK deadline transport. |
| Winamp VGM/VGZ | Foreground parsing produces ordered PSG writes; RAWCLK keeps their admitted timestamps and write order. |
| Winamp MOD | The foreground arranger produces PSG square-wave states, using the same transport. |
| Beats/Fiddle and the instrument kit | Their paired app-local MIDI support uses the shared SOUND provider and each client's existing foreground cadence. |
| Effects Lab | The app compiles the complete bounded effect into 60 Hz PSG or speaker states before starting RAWCLK playback. |
| Windows SOUND calls and alerts | The unified driver owns the legacy queues and its SYSTEM timer, independently of a client's prepared format renderer. |

The prepared interrupt path does no file I/O, input parsing, synthesis or heap
allocation. Queued deadlines can continue while a USER message is delayed;
foreground refill, UI response and cleanup still depend on Windows yielding.
Finite queues can run out during a long stall.

Ordered VGM groups hold at most 16 writes and require at least 9,546 PIT
cycles between distinct groups (the ceiling of 8 ms). Unsupported streams
are refused rather than silently retimed. [The format contract](FORMATS.MD)
describes the current file and timing bounds.

The shared owner prevents cooperating clients from taking each other's sound.
Pause preserves the relevant playback state and silences output; normal Stop,
end and Close release the owned sound path. Direct foreign port writers are
outside that cooperative contract. Speaker generation uses PIT2; deadline
scheduling and speaker pitch are separate counters.

[Current native/source receipts](../tools/WINAMP/TESTS.JSN) and the app-specific
validation files separate host tests, emulator execution and physical reports.
Emulator cycle settings do not establish physical latency or analog fidelity.
