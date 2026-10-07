# Optional fourth Mini Piano pitch

F3 explicitly enables or disables the extra PIT2 lane. The default remains
three PSG voices. F3 clears all held notes before changing the preference;
older SOUND drivers decline the feature with "Extra unavailable".

The ownership core PIANO.C is unchanged. After its MIDI events, the app
compares held-note references with MIDIMAP DEBUGSTATE queries 4 through 6.
One held pitch absent from those three slots plays through the shared v1
PIT API. Duplicate mouse/keyboard references share the same pitch; repeated
keydown does not produce a new note. If more pitches are held than the
four available melodic voices, only one missing pitch can occupy PIT.
The lowest missing MIDI pitch is selected deterministically. This does
not change the paired driver's existing PSG voice-stealing order.

Escape, focus/deactivation, mouse cancellation, octave changes, Panic,
final release, session end and close all clear PIT before releasing the
MIDIMAP/SOUND lease. Failed MIDI or PIT sends panic the local state and
reset/close the driver. PIT runs as an autonomous square wave with fixed
amplitude. The app neither programs ports directly nor shares PIT with
PWM speech; no hardware or physical-fidelity claim is made.

## Shared client

../SHARED/PITCLNT.H is a C89 header copied into the build output under its
DOS 8.3 name. Include windows.h first. PIT_CLIENT stores a discovered
procedure and one cached note.

- PitClientDiscover(client): resolve SOUND/TANDYPITVOICE dynamically.
  Call while no client note is active; an old SOUND export returns -16.
- PitClientNote(client, owned, note): use version 1 with MIDI 45 through 96
  mapped to rounded equal-tempered Hz. Valid ownership and unchanged
  cached note avoid redundant programming. Out-of-range input returns -13.
- PitClientOff(client, owned): send version 1, Hz 0, level 0, then clear
  cache. Ownerless calls return -1 and never invoke the procedure.
- PitClientForget(client): invalidate the cache without hardware access.

The caller supplies true ownership only while retaining its SOUND lease.
Off precedes every reset/close; Forget follows ownership loss or external
reset. Discovery does not acquire the lease. Every failure requires the
caller to panic/reset/close; cache invalidation alone does not stop sound.
No fallback port writes or baseline promotion occurs.

## Tests and builds

BUILD.PY copies the shared header. BUILD.BAT uses Microsoft C /G0 /Gw /AS
/W3 /Od and the existing licensed Win16 linker/libraries. Keep emulator
runs serial under the common exclusive FileStream lock.

MINIPIAN /pittest automatically checks default-off, F3 discovery, ownerless
refusal, MIDI 44/97 bounds, three unchanged PSG slots, fourth distinct
pitch, repeated key, duplicate mouse/key references, last release, replay,
focus, octave, Escape, toggle reset and close cleanup. MINIPIAN /pitold
expects an old SOUND export to be unavailable while PSG still functions.
Existing /test, /busytest, /missing, /ownertest and /sendtest remain.

PITQA.C and PITQABLD.BAT provide native DOS checks of the shared client
against a mocked API. They test old export decline, no-owner refusal,
cache invalidation, redundant-note suppression, pitch bounds, failed send,
Off, Forget and monotonic frequency mapping. No real ports are accessed.
The shared client tests do not certify system-wide BIOS beep ownership.
