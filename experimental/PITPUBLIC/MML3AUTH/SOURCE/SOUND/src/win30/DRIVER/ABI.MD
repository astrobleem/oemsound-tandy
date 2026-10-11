# Windows 3.0 sound ABI investigation

Status: source and binary inspection only. No driver implementation or runtime
SYSTEM-timer callback test is claimed here. No stock binary/source is included.

## Sources and target

- Existing target: Windows 3.0 real mode, 8086/8088, normal 640 KB.
- Bundled DDK/286/SOUND/SOUND.DEF, SOUND.ASM, SOUND.INC, MDD_CORE.ASM.
- Bundled DDK/286/COMM/COMM.DEF, IBMSETUP.ASM and IBMINT.ASM.
- Windows SDK WINDOWS.H sound declarations are authoritative for C prototypes.
- Actual guest SYSTEM.DRV SHA256:
  f9eca86a90c9c5ae3fb4c89cd481cbd6e71142799cd41fa3f541b6d7c46d39ed
- Public reference:
  https://github.com/astrobleem/oemsound-tandy/tree/main/DDK/286/SOUND

## Export contract

All public exports use FAR PASCAL Win16 calling conventions.
Library identity is SOUND. Code and callback-accessed data must be fixed.
The stock definition uses DATA FIXED SINGLE PRELOAD.

Ordinal | Name | Parameters (16-bit words unless noted)
1 | OpenSound | none; returns voice count or negative error
2 | CloseSound | none; void per SDK
3 | SetVoiceQueueSize | voice, bytes
4 | SetVoiceNote | voice, note, length divisor, dots
5 | SetVoiceAccent | voice, tempo, volume, articulation, pitch
6 | SetVoiceEnvelope | voice, shape, repeat
7 | SetSoundNoise | source, duration
8 | SetVoiceSound | voice, DWORD 16.16-Hz frequency, duration
9 | StartSound | none
10 | StopSound | none
11 | WaitSoundState | state
12 | SyncAllVoices | none
13 | CountVoiceNotes | voice
14 | GetThresholdEvent | none; FAR pointer returned in DX:AX
15 | GetThresholdStatus | none
16 | SetVoiceThreshold | voice, count
17 | DoBeep | one WORD, confirmed by stock ret 2
18 | MyOpenSound | none; stock internal OpenSound helper
WEP | Windows exit procedure | one WORD, ret 2

Important: older SOUND.H declares SetVoiceSound as four integers. SDK
WINDOWS.H and SOUND.ASM instead agree on (int, DWORD, int). These have the
same argument byte count but different C source meaning and word ordering.
Do not use the obsolete prototype.

## Verified stock behavior

- OpenSound is exclusive by GetCurrentTask; a second open fails with -1.
- Voice numbers are one-based. Stock implementation has only voice 1.
- Default tempo 120, pitch 0, normal articulation, threshold 0.
- Default queue contains 32 six-byte records (192 bytes), despite stale
  comments saying seven-byte records/224 bytes.
- Queue resize requires the queue to be empty, including active playback.
  It returns -3 while occupied; allocation failure is -2.
- StartSound is idempotent and preserves queues.
- StopSound silences and flushes all queues.
- CloseSound stops/flushed playback, releases memory and exclusive ownership.
- Stock nonowner/out-of-range operations are mostly ignored with success.
  A new implementation must document any deliberate stricter error policy.
- CountVoiceNotes returns remaining notes, and at least one while playback
  is still active. Returning zero must not precede the last note-off.
- Threshold event is a stable FAR pointer to a word. GetThresholdStatus
  atomically returns/clears that event word in the stock implementation.
- WaitSoundState states: 0 queue empty; 1 a threshold; 2 all thresholds.
  Stock wait yields; replacing this with a tight spin is unacceptable.

## Notes and duration

SetVoiceSound uses unsigned 16.16 Hz, with integer frequency in the high
word. Duration units are 2.5 ms. Stock ignores fractional frequency.

SetVoiceNote accepts note 0 as a rest; 1..84 are seven chromatic octaves.
High-octave C..B Hz table: 4186,4435,4699,4978,5274,5588,5920,6272,
6645,7040,7459,7902. Lower octaves are divided by powers of two and rounded.
For example note 25 is C near 262 Hz, note 34 is A440.
Pitch 0..83 offsets nonzero note values modulo 84.

Nominal full duration in 2.5-ms units:
  96000 / (length_divisor * tempo) * (2 - 2^(-dots))
Length divisor 1..64; tempo 32..255. Use bounded integer arithmetic and
explicitly bound dots before shifting. Normal articulation sounds 7/8 of
full duration; legato sounds all; staccato sounds 3/4; the rest is silence.
PSG periods must remain 1..1023. At the assumed 3.579545-MHz clock, the
lowest PSG tone is about 109.35 Hz. Reject unsupported low frequencies or
explicitly document a transposition policy; do not silently claim correct
pitch while clamping every low note to period 1023.

## SYSTEM timer: verified in actual Windows 3.0 guest binary

NE exports and disassembled code establish:
- SYSTEM.2 CreateSystemTimer(WORD milliseconds, FAR callback): FAR PASCAL,
  retf 6; returns nonzero WORD handle or zero on exhaustion.
- SYSTEM.3 KillSystemTimer(WORD handle): FAR PASCAL, retf 2. Do not rely on
  its undocumented return value.
- SYSTEM.6 GetSystemMsecCount(): FAR, DWORD result in DX:AX.
- Callback is a parameterless FAR call, not a USER TimerProc with arguments.
- Callback runs from the stock timer path, independently of message pumping.
- Cadence is approximately 54.925 ms. Conversion truncates requested
  milliseconds to BIOS ticks: requesting 100 ms gives one tick, not 100 ms.
- Dispatcher preserves outer registers but uses DS and SI across callbacks.
  Use a conservative assembly callback shim preserving all registers,
  segment registers and flags, and establishing the driver's data segment.
- Driver must never call USER/GDI, DOS, allocation or movable code from
  this callback. Precompute division/pitch/duration outside it.
- Driver should not invoke EnableSystemTimers or DisableSystemTimers.
  Those global controls affect the shared stock timer mechanism.
- No driver-owned IRQ vector, PIT or PIC writes are needed. The stock
  SYSTEM implementation itself manages its normal timer hook.
- Kill the exact handle before freeing/unloading callback code or data.
  Never leave a timer pointing into an unloaded library.

COMM source independently confirms interval then FAR pointer argument
order, zero-on-failure behavior, and a parameterless callback that restores
DS. This is stronger evidence than guessing from modern Win32 APIs.

## Recommended bounded next validation

First build a separate fixed-code/fixed-data library with a register-saving
callback shim and counter. Test Create/Kill, repeated cycles, exhaustion,
continued Windows clock/repaint activity and absence of callbacks after kill.
Only then integrate a three-voice bounded PSG queue backend.

Use fixed ring queues with producer publication protected by brief saved-
flags critical sections. Never unconditionally STI when restoring caller
interrupt state. The callback should consume bounded work, update periods,
volumes and counters, and do nothing else. Preserve accumulated fractional
milliseconds so note timing does not drift a full 55-ms tick per note.

Required semantic tests: ownership, full queue atomic failure, resize while
active, note/rest/dot/articulation boundaries, low-frequency rejection,
repeated Start, Stop/Close during playback, final note-off before count zero,
threshold crossing/read-clear, and safe teardown. Explicitly test behavior
if owner exits without CloseSound; stock code alone does not establish a
robust task-death recovery contract.

## Unresolved full-compatibility limits

The provided stock sources are not an exact behavioral specification for a
complete multivoice driver: SyncAllVoices, noise and envelope exports return
success without implementation; volume is ignored; the dots branch contains
cmp bx,bx (always equal); threshold comments conflict with decrement logic.
Do not copy these defects or represent unsupported operations as successful.
A restricted first driver needs a published supported subset and honest
negative errors for excluded features, or additional primary documentation
and tests before claiming complete legacy SOUND compatibility.

A USER SetTimer callback is appropriate for the standalone Play/Stop app,
but cannot guarantee driver playback while a caller waits without dispatching
messages. It is not a drop-in substitute for the verified SYSTEM callback.
