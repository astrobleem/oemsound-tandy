# Experimental bounded Win3.0 PSG driver

This is a restricted three-voice SOUND ABI implementation for known Tandy
hardware in Windows 3.0 real mode. It is not a MIDI/wave driver and is not
ready to install over a working system without the isolated acceptance tests.

## Supported contract

- Exclusive task ownership; OpenSound returns three voices.
- Three fixed ring queues, 32 records by default and 64 maximum each.
- Six-byte records; QueueSize accepts 0..384 bytes per empty voice queue,
  rounded down to a record. Zero disables enqueueing. Queue-full failure
  does not overwrite or partially append a note.
- SetVoiceNote: 0 rest, 1..84 notes; length divisor 1..64, dots 0..15.
- SetVoiceSound: integer-Hz subset of the 16.16 API; nonzero fractional
  words explicitly return -13. Duration 1..32767 in 2.5-ms units.
- PSG-unrepresentable low pitches return -13, rather than silently clamp.
- Accent: tempo 32..255, articulation 0 normal / 1 legato / 2 staccato, pitch 0..83.
  Volume unsigned 0..65534 maps monotonically to 16 attenuation steps;
  65535 selects the default attenuation 4. Parameters apply to subsequently
  enqueued notes; queued records retain their volume and timing.
- Start is idempotent; Stop flushes and silences; Close releases ownership.
- Count includes the active record through its final note-off.
- Threshold event bits latch crossings from count N to N-1. GetStatus
  atomically reads and clears; event-pointer storage remains fixed.
- WaitSoundState yields; waits for current below-threshold conditions.
  State 1 returns a qualifying one-based voice; state 2 waits for all three.
  An impossible wait while not running returns -16 instead of hanging.

## Explicitly unsupported

Envelope returns -11; noise returns -15; SyncAllVoices returns -16.
DoBeep returns 0 (failure/no beep); it does not claim to produce system beeps.
These are deliberate restrictions, not placeholder successful stubs.
Nonowner/invalid-voice mutating operations return -1; query counts return0.
Invalid or failed calls do not grant another task ownership.
Abrupt owner termination without CloseSound has not been qualified. Applications
must pair OpenSound/CloseSound; normal library teardown mutes in WEP.

## Scheduling and safety

SYSTEM.2 and SYSTEM.3 are the stock CreateSystemTimer/KillSystemTimer entry
points verified in the target Win3.0 binary. A 55-ms request runs at the stock
~54.925-ms cadence. Our callback preserves all registers, segment registers
and flags, uses fixed code/data, and never calls DOS, USER, GDI or allocation.
There are no driver-owned interrupt-vector, PIT or PIC writes. PSG writes
use only port C0h after real-mode and ROM-signature checks.

Note timing is intentionally coarse. Fractional tick remainder carries
between notes; catch-up is limited to four transitions per voice per callback
so very short dense sequences may lag. This is not a precise MIDI clock.
The allocated SYSTEM timer is retained after a queue naturally finishes and
removed by Stop/Close/WEP, before any callback code could be unloaded.

## Build and validation

Use BUILD.PY with --repo pointing at the existing licensed toolchain checkout,
--dosbox pointing at the Linux DOSBox-X launcher, and a new --out directory.
Build flags: /G0 /Gw /Gs /AS /W3 /Od; MASM .8086; fixed single data segment.
The intentional linker warning is library name SOUND versus TSOUND.DRV.
BUILD also builds DRVTEST.EXE, an automatic isolated-guest smoke test that
writes C:\DRVTEST.LOG and exits Windows after validation.

For testing only, clone an existing guest and replace its stock SOUND.DRV
with the candidate, retaining all original installations unchanged. Do not
publish the cloned guest, Windows binaries or stock DDK/toolchain files.
The code is not accepted merely because compilation passes: retain driver
log and captured audio evidence for the exact binary tested.

See ABI.MD for source/binary findings and limitations of the bundled sample.

## Current isolated result

The exact candidate in TESTS.JSON passed 105 automatic native checks with
640KB, disabled XMS/EMS/UMB and the emulator's 8086-prefetch CPU core. Captured
PCM contains the three simultaneous tone fundamentals near 262/330/392 Hz,
repeated 440 Hz test tones and a quiet final tail. This establishes the tested
subset in that emulator, not physical-hardware or complete SOUND compliance.
A separate task could not open, start, stop, resize, retune or close the
owner's playback. Active Stop and Close removed their timer; callback counts
stayed unchanged after Stop while Windows continued running. Abrupt owner
exit and SYSTEM timer exhaustion remain explicitly unqualified. The test guest and stock binaries are excluded.

## Isolated install and rollback procedure

1. Shut down the emulator. Copy a known-working Windows 3.0 guest directory
   to a new test directory. Never run these steps on the only working guest
   or a physical machine before separate hardware qualification.
2. Keep the original guest untouched. In the clone, confirm SYSTEM.INI's
   existing sound.drv entry resolves to WINDOWS\SYSTEM\SOUND.DRV. If its
   layout differs, resolve the actual file rather than guessing a new path.
3. Preserve a copy of that cloned stock driver outside the clone. Copy the
   built TSOUND.DRV into the clone as WINDOWS\SYSTEM\SOUND.DRV. Internal
   module identity must remain SOUND; merely loading a differently named
   file alongside the stock module can select the wrong module.
4. Copy the matching DRVTEST.EXE and INTRUD.EXE into the clone's C: root.
   Retain the clone's accepted display configuration and RESERVE launcher.
5. Use an absolute DOSBox-X config path (its wrapper changes directories).
   Configure machine=tandy, 8086_prefetch, 640 KB, no XMS/EMS/UMB and a
   separate capture directory. Run the native test through win /r.
6. Check both DRVTEST.LOG and INTRUD.LOG for TOTAL_FAILURES=0, and inspect
   the finalized PCM capture. A missing/partial log is not a passing run.
7. For rollback, stop the emulator and restore the saved stock SOUND.DRV
   to the clone, or abandon the clone and resume the untouched original.
   No live file replacement or main installation changes are necessary.

Do not install this candidate as a general-purpose replacement while its
unsupported features matter: Windows MessageBeep/DoBeep will be silent,
noise and envelopes are unavailable, and SyncAllVoices rejects the request.
The native demo remains available independently of this experimental driver.
