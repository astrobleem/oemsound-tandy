MINIMIDI BOUNDED SMF CORE
=======================

Files
-----
SMF.H / SMF.C     Standalone ANSI C89 core for Microsoft C 6.0 Win16.
HOSTTEST.C       Portable file-to-event-log harness (compile with SMF_HOST).
TESTSMF.PY      Generates original fixtures and runs deterministic tests.
FUZZTEST.C       Host-only API/invariant tests and 100000-case mutation fuzz.
TESTFUZZ.PY     GCC AddressSanitizer/UndefinedBehaviorSanitizer fuzz runner.
fixtures/       TYPE0.MID and TYPE1.MID plus exact expected text event logs.

Host test
---------
    python3 TESTSMF.PY
    python3 TESTFUZZ.PY

Requires Python 3 and GCC, no third-party packages. GCC receives -x c because
uppercase .C otherwise selects C++ on Unix. Strict C89 and conversion warnings
are errors. SMF_HOST uses 32-bit unsigned int, checked at compilation, instead
of the Win16 unsigned long. Neither the core nor its arithmetic uses a 64-bit
integer type. The separate Python timing oracle uses exact Fraction values.

Win16 integration
-----------------
Compile SMF.C with the application's Microsoft C 6.0 flags and include SMF.H.
Do not define SMF_HOST for the Win16 build. unsigned is 16 bits and unsigned
long is 32 bits on that target. SMF_FAR defaults to the Microsoft far keyword;
it can be supplied by the build if necessary.

Use a single <=32768-byte GlobalAlloc allocation, GlobalLock it, and retain
the lock until playback finishes. smf_open BORROWS that immutable buffer; it
does not copy or free it. The FAR buffer must fit in a single segment without
offset wrap (GlobalLock normally supplies an offset-zero pointer). The core
contains no Windows calls, heap allocation, floating point or recursion.
It uses one static state and is neither reentrant nor an interrupt callback.

    if (smf_open(file, size)) {
        while ((status = smf_next(&event)) > 0) {
            /* Schedule event at playback_start + event.ms. */
        }
    } else {
        /* Display smf_error(). No playback has occurred. */
    }

smf_open returns 1 only after validating every event and the complete merged
timeline, then rewinds. smf_next returns 1/channel message, 0/EOF, -1/error.
smf_rewind returns 1/0 and resets tempo, running status, exact timing carry,
and all track cursors. It relies on the already-validated immutable buffer.
smf_close releases the borrowed pointer from core state; the caller owns the
actual unlock/free. smf_error returns static error text. smf_event_count gives
validated channel-message count; smf_duration_ms includes the final EOT and
is floor(exact duration in ms). Track count and PPQN accessors are available.

SMF_EVENT contains absolute tick and deadline in ms, full channel status,
two data bytes, message length including status (2 or 3), and zero-based
track index. data2 is zero for two-byte messages. A note-on with velocity zero
remains a note-on with velocity zero. The parent must convert/filter as its
MIDI output driver requires. Program changes, pressure, pitch bends and all
controllers are emitted even if the target hardware driver ignores them.

Supported files and deliberate limits
------------------------------------
* Standard MIDI Files format 0 (one track) and format 1, 1..16 tracks.
* PPQN 1..32767. Format 2 and SMPTE divisions are rejected.
* File size <=32768 bytes, <=16384 total events (including ignored meta,
  SysEx and EOT), absolute ticks <=0xffffffff, exact duration <=one hour.
* Header extensions and unknown, length-bounded chunks are skipped.
  All declared tracks must be present, additional MTrk chunks are rejected,
  and even unknown chunks must fit entirely inside the supplied file.
* Every track must contain a zero-length EOT as its final event. Missing EOT,
  any bytes after EOT, bad lengths, truncated data and >4-byte VLQs fail.
  Non-minimal but <=4-byte VLQs are accepted.
* Channel running status is independent per track; meta and SysEx cancel it.
  Illegal status/data bytes and non-SMF system statuses are rejected.
* Every legal meta type 00..7f is length-checked and skipped except tempo
  (51, exactly three bytes, nonzero) and EOT (2f, zero bytes).
* F0 and F7 SysEx events are length-checked and skipped, NEVER transmitted.
  Their bounded payload is opaque, including divided/escaped SysEx data.
  Therefore files requiring manufacturer setup messages may sound different.
* Lowest track index wins all equal-tick ties. Original event order inside
  each track is preserved, including repeated zero-delta events. Tempo
  events may appear in any track and obey this same deterministic order.

Integer timing proof
--------------------
Default tempo is 500000 microseconds per quarter note. Let d=PPQN,
delta=a*d+b, tempo=q*d+r. An elapsed interval is:

    a*tempo + b*q + floor((b*r + previous_remainder)/d) microseconds

with new remainder (b*r + previous_remainder) % d. The remainder is retained
across tempo changes: the denominator PPQN never changes. b,r<32767, so their
product plus carry fits in 32 bits. b*q plus the divided fraction is at most
the 24-bit tempo. a*tempo is guarded against the remaining one-hour budget
before multiplication. Cumulative whole microseconds <=3600000000 fits in
unsigned 32 bits. Nonzero remainder beyond the exact one-hour endpoint is
rejected too. Conversion to ms occurs only when reporting deadlines or
duration, so fractional ticks/tempo changes cannot accumulate rounding drift.

This intentionally rounds deadlines down to the millisecond. Actual sound
timing still depends on the parent's timer granularity and output latency.

Fixture provenance
------------------
Both musical fixtures were generated for this project from original small
note sequences, not copied or transcribed from copyrighted compositions.
TYPE0: eight-note exercise, two tempos, note-on velocity-zero note release,
program/controller events; duration 3600 ms.
TYPE1: conductor, independent melody and percussion tracks, simultaneous
events, a tempo change, running status and CC120/123; duration 2500 ms.

Coverage
--------
The script tests expected logs against independently computed rational
timestamps; all channel message sizes; deterministic merging and per-track
running status; tempo changes/carry; PPQN and file/track boundaries; legal
opaque meta/SysEx; exactly one hour and fractional overrun; 28-bit delta and
32-bit absolute-tick bounds; malformed/truncated files, track/event lengths,
statuses, tempo and EOT; an artificially lowered event budget; repeated
rewind/EOF; 180 randomized valid files; and 400 mutation smoke cases.
The additional sanitizer harness exercises 100000 deterministic arbitrary/
mutated buffers, success/failure API invariants, monotonic time/tick output,
event-budget bounds, repeated EOF, and identical replay event hashes.

Host C89 tests do not prove Microsoft C6 code generation or real 8088/Win3.0
playback. Those remain integration/build/runtime checks for the application.
