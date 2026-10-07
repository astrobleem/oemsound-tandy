# Optional PIT2 voice: local stages 1 and 2

Source base: 6b8da52dda292c01abc061f9f190238047031cc1, sound polish
branch used by experimental PR 31. Local branch: ai/pit2-voice.
No installed driver, CF media, published package or remote branch changed.

## Architecture and limits

SOUND owns the private version-1 TANDYPITVOICE export under its existing
OpenSound task lease. The standard API still reports three PSG voices.
The extension is dynamically discovered; an older driver refuses the option.
The reusable PITCORE.H implements on/off whole-Hz tone generation, 37..7902
Hz, using the 1,193,182-Hz PIT clock. Level accepts only 0 or 1: there is no
independent hardware amplitude envelope. Disabled/default operation does not
touch PIT or speaker ports. Changing pitch uses mode 3, channel 2 only.

The assembly writes 43h=B6h and two divisor bytes to 42h. It reads/modifies
only bits 0..1 of 61h, retaining EX mute/routing/keyboard bits. No writes to
62h, PIT0/PIT1, interrupt vectors or IRQ tick rates are added. The current
PSG route is retained. First acquisition refuses nonzero gate/speaker bits.
It does not read/restore an unknown previous PIT mode or divisor; it requires
a quiet speaker and leaves the channel silenced after release.

Beats uses private MIDI-driver message 5441h, p1=0/1, p2=1 (version), with
the existing cookie/owner. Enabled channel 15 maps to PIT instead of the PSG;
this mapping is opt-in and resets sounding notes when changed. Velocity is
only on/off. Normal channels and PSG/noise behavior remain unchanged by
default. The fifth editable lane is visible, with a separate PIT on/off
button. Replay, stop, Escape, deactivation, close and reset clear the voice.
Failure while starting a note stops playback. Driver WEP silences the PIT;
MIDI WEP uses its own bounded assembly off operation, avoiding cross-DLL
calls during unload. Neither automatically resumes stale notes after speech.

Applications sharing OpenSound cooperate. Automatic Mouth currently uses
PSG vowel cues and takes this lease. Buddy PWM speech uses the same hardware
PIT2 resource as this tone and cannot run independently alongside it. Before
any future PWM operation, its owner must stop PIT music and end/hold the
musical note explicitly. Resume only if that note is still current. Existing
speech guards have not been changed.

Windows MessageBeep/direct-I/O programs can bypass the lease. This work does
not establish system-wide ownership, transparent beep interception, EX route
fidelity or physical hardware acceptance. A beep starting during a tone may
disturb it; application-only arbitration cannot prevent that. Stock Windows
DoBeep behavior and full system integration require separate runtime tests.

## Consumer checklist and coordinated phases

| Consumer | Local result | Next coordinated work |
| --- | --- | --- |
| SOUND driver | Versioned API implemented; standard ABI unchanged | Physical EX acceptance and beep interference tests |
| Beats | Fifth lane and explicit opt-in implemented, 8086 build verified | Native baseline/PIT/UI/close passed; physical UI acceptance |
| Mini Piano (MINIPIAN/PIANO) | F3 extra held pitch; native PIT/baseline passed | Native old-driver refusal passed; physical keyboard acceptance |
| JoyMIDI | F3 optional octave layer; native PIT cleanup passed | Native baseline passed; physical joystick acceptance |
| MiniMIDI | Explicit channel-16 PIT button; native five-generator test passed | Native baseline passed; authored channel-16 files |
| Automatic Mouth | Existing speech lease preserved | Keep PIT voice off while speech owns hardware |
| TCHIME / XPCHIME / TEXIT | Stage3 shared lease and explicit PIT option | 28 native chime/demo cases passed; physical/exit-veto gates remain |
| PSGPLAY | Stage3 per-play lease and F3 optional layer | Native default/PIT/refusal/close/focus/Escape passed |
| DOS PSGTEST / WAVHYB / legacy MIDIMAP | Remaining DOS integration / historical stub | See precise done/remaining/N/A scope in PITCONS.MD |
| Display driver/launcher/install utilities | No musical UI needed | Preserve sound ownership and route; no display driver edits |
| Buddy Maker converter/WPF preview | MML1/MML2 unchanged | Coordinate explicit versioned extra-voice interchange and opt-in UI |
| Buddy DOS runtime | Existing PIT PWM guard unchanged | Shared tone/PWM mutual exclusion, no stale-note restore |
| Buddy Windows16 player | API design available; no patch applied | Coordinate native owner/PWM path and lifecycle |
| Buddy x64 player | Concurrent pacing checkout untouched | Versioned parsing/rendering after owner task completes |

All musical consumers remain in scope. Stages 1 and 2 provide shared plumbing,
Beats, Mini Piano, JoyMIDI and MiniMIDI. Further consumers must follow
the bounded Windows/PSG+PIT test gate, then share the paired MIDI driver rather
than diverging copies. SOUNDP01/P02 frozen source/payloads remain unchanged.

MML1/MML2 are canonical interchange, not implicitly five-voice formats. A
proposed future MML3 (or explicitly typed companion with mandatory version)
must encode PIT pitch/on-off and reject unsupported or unknown extra data
clearly. No parser, writer or movie contract is modified in this patch. Parent
coordination is required before selecting or changing that format.

## Evidence

Microsoft C /G0 + MASM .8086 builds of TSOUND.DRV and the paired Beats/MIDIMAP
passed using existing locally licensed tools through DOSBox-X machine=tandy.
Host-only tests execute the production core and drivers with mocked I/O and
Windows calls: PITTEST 267 checks, LEASEQA 16 checks, HOSTTEST 200035 checks.
They cover bit preservation, pitch bounds, already-gated refusal, disabled
baseline, old/unknown-version refusal, task ownership, all five voices,
stale note-off, reset, close and rollover. Shared PITCLNT.H also passes 71
native DOS mocked discovery/cache/lease/bounds checks.

Private Windows 3.0 real-mode guests copied from the user's complete owned
backup now run successfully through DOSBox-X machine=tandy with stock CGA.
Eleven selected native runs report zero failures: Beats baseline, /pittest,
/uitest and /closetest; Mini Piano baseline, /pittest and /pitold; JoyMIDI
baseline and /pittest; MiniMIDI baseline and /pittest. These exercise actual
Windows callbacks and SOUND/MIDIMAP entry points, plus lifecycle cleanup.
The former incomplete-runtime blocker is resolved. These are emulator runs,
not physical EX routing or human listening acceptance. Source backup hashes
are checked before delivery; no installed driver is replaced.

Mini Piano selects one held pitch absent from the three active PSG slots.
Its F3 option starts off; release/refcounts, octave changes, Escape, focus,
option toggles and close clear the PIT client. JoyMIDI F3 adds a fixed-level
octave below its PSG note and disarms on input/focus/panic failure. MiniMIDI
opts MIDI channel 16 into the private MIDIMAP route. Option changes stop
playback; disabled focus behavior remains the original baseline. All require
the shared SOUND extension; unavailable/busy operation fails without direct
port fallback. PITCLNT.H intentionally supports MIDI pitches 45..96 within
the wider driver's Hz range. No standard MIDI-file or movie schema changes.

Separate approved choir experiments compare original synthetic vowels with
held PSG chords. They do not relax normal movie speech guards, and will be
reported with actual sample rate, routing and cleanup evidence separately.

Captured independent SPKR and TANDY emulator audio streams overlap in Beats,
Mini Piano, JoyMIDI and MiniMIDI PIT tests. The delivery evidence includes
stream activity statistics and mixed WAVs; they do not prove physical EX
summing, measured hardware fidelity or human listening acceptance.

Canonical complete consumer inventory and exclusions: PITCONS.MD.
Movie extension proposal (no format implementation): PITMOVIE.MD.
