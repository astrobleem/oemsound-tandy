# Win16 Buddy lookahead client preparation

This is a client adaptation of the WZM1/WZV2 player in this directory. It does
not change the Windows x64 PCM player, DOS players, mapper or SOUND/PIT owners.
It is not native Win16 timing qualification. The real queue binding and paired
backend tests remain required before enabling timestamped playback.

## Current behavior and prepared changes

The available MIDIMAP.DRV exports MIDIMESSAGE but no documented timestamped
queue interface. A normal build therefore retains immediate MIDI output and
the existing 55 ms USER timer. It now preserves every due WZM snapshot, with
bounded foreground work, instead of collapsing overdue snapshots to the last
one. It drains overdue audio before drawing pictures. This fixes event loss;
it cannot make immediate callbacks precise or remove their audible lateness.

Both paths validate the complete bounded score before opening sound. Cue start,
resume and Left/Right seek reconstruct the current snapshot with binary search,
without replaying historical attacks. Space pauses/resumes; Enter replays the
full movie; Escape stops. Pause/focus/minimize stops and closes the same owner,
so resume reconstructs and starts a fresh timeline. No new device acquisition,
direct PSG write, PIT programming, IRQ callback or timer is introduced.

With a real bridge, `BQSCHED.C` uses the backend clock for movie position and
absolute event deadlines. The foreground 55 ms timer refills up to 250 ms ahead,
then draws the picture selected by that same clock. A 55 ms initial lead allows
the first buffer fill before the movie origin; UI delay does not shift accepted
event deadlines. Pictures may be dropped independently. This lead/lookahead
must be verified and tuned with the real queue capacity and native I/O stalls.

Each WZM snapshot expands to up to eight queue messages: all changed/retriggered
note-offs first, then note-ons, in lane order. A lane-three noise note maps to
MIDI channel 9. Queue capacity counts messages, not snapshots. If capacity runs
out partway through a snapshot, the client retains the exact unaccepted suffix
and source cursor. Retrying never duplicates its already accepted prefix and
never replaces it with a newer state. Drawing yields priority to audio refill.

EOF explicitly queues silence at the selected end, even when the source has no
final off state there. Completion waits until movie time reaches the end AND
the backend acknowledges that all submitted messages, including final offs,
were consumed. An error, pause, seek, replay, focus loss or close flushes pending
work before a fresh seed/timeline can be submitted.

## Internal adapter contract for the backend owner

`BQOPS` in `BQSCHED.H` is an internal, near-C callback table. It is not an export
ABI or a claim that these names exist in MIDIMAP/SOUND. The bridge must wrap the
actual versioned FAR PASCAL exports, structure packing and status semantics.
It reuses the owner/cookie already acquired by `acquireSound`; it must not open
another sound device or acquire another SOUND/PIT lease.

| Callback | Required behavior |
| --- | --- |
| `now(context, &ms)` | Modulo-2^32 monotonic milliseconds in exactly the queue deadline epoch. Works without USER dispatch. Clock failure is explicit. |
| `put(context, due, packed)` | Enqueue one absolute-deadline packed MIDI message. `BQ_FULL` means nothing was accepted. FIFO order for equal deadlines; late events are retained and applied in order. |
| `reset(context)` | Return success only after queued work is flushed and owned output silenced; no old callback/event may survive the acknowledgement. |
| `pending(context, &count)` | Count remains nonzero until all accepted messages have actually been applied, including final note-offs. It must not report only producer-side emptiness. |

All return `BQ_OK` (0) or an error. Only `put` may return `BQ_FULL` (1).
The client never blocks/spins waiting for space: it returns to Windows and
retries at a later foreground dispatch. Queue errors are fatal and route through
existing reset/close cleanup. Clock values may wrap; a backward movie position
is an explicit error rather than a visual/audio rewind.

The bridge is opt-in at build time: supply `BQBRIDGE.H` and define
`BUDDY_QUEUE_BRIDGE`. That header must implement the local function:

```c
static int BuddyQueueBind(HINSTANCE module, BQOPS *ops);
```

It returns nonzero only after validating all actual exports/capabilities and
populating all callbacks. A bridge-enabled build refuses playback when binding
fails; it does not silently fall back to immediate output. No synthetic bridge
is shipped or selected by the production player. Without an agreed backend ABI,
defining the switch fails to compile because the real bridge header is absent.
The backend owner must add the header/build switch to its licensed native build
kit; `BUILD.PY` copies the portable core for the ordinary build.

## Tests and qualification boundaries

Run the Windows host logic suites in a new output directory:

```powershell
python TESTBQ.PY --cc C:/path/to/gcc.exe --out C:/path/to/new-results
```

The C89 core tests use a synthetic bounded backend: partial acceptance, dense
one-millisecond retriggers, ordering, initial lead, clock wrap, lookahead,
pause/reset acknowledgement, backward seek, EOF drain and error cleanup.
The integration suite extracts and compiles the actual `fetchEvent`,
`applyState`, `musicAt`, `movieNow` and `updateMusic` bodies from TINYVID.C. It
exercises the real source cursor against synthetic memory I/O, immediate backlog
preservation and queue backpressure. No duplicated scheduler model substitutes
for those production function bodies.

Local results: 1,029 core checks and 1,623 production-function checks passed with
Windows GCC C89 warnings-as-errors. The ordinary source also compiled and linked
to a Win16 NE executable with installed Open Watcom 1.9, `-bt=windows -0 -ms -zw`.
Its existing WndProc WORD/WPARAM type warning remains with those headers. This
does not establish MSC6 equivalence, real FAR PASCAL bridge compatibility,
precise timing, audible behavior or actual focus/keyboard delivery. No emulator
was started; generated executables and logs stay outside the repository.

Before readiness/merge, the parent/backend owner must bind the actual clock,
queue/full, reset and pending APIs, compile with the licensed MSC6 build kit,
and run paired native tests under the shared emulator lease. Include dense
same-time bursts, queue saturation during GDI/file I/O, pause/seek/replay across
clock wrap, EOF final-off drain, owner contention, focus/session/close teardown,
and silence after flush. The backend must have a sufficient pump budget and
stable deadline clock; client tests cannot qualify either property.
