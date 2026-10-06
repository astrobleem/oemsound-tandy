# App-local instrument layer 1.2 (experimental test build)

This is an opt-in extension to the paired Windows 3.0 real-mode MIDIMAP.DRV,
not a registered MMSYSTEM device or a replacement for SOUND.DRV. It targets
Microsoft C6 /G0 8088-compatible code, small-model Win16, normal 640 KB
conventional memory, and the existing three PSG tones plus one noise voice.
All curves and the audition were newly authored. No game code, music, bank,
samples, PC-speaker bass, or Tandy DAC playback was copied or added.

## Calling contract

Existing exports and ordinals are retained. Capabilities report version
0x0102; MIDIAPI.H names two private messages. They are not MMSYSTEM IDs.

- MODM_SYNTHCONFIG (0x4000): p1=0 for legacy rectangular notes, p1=1 for
  envelopes, or p1=3 for envelopes plus preset-enabled vibrato; p2 must be 0.
  Other flags, including vibrato alone (2), return INVALPARAM. Reconfiguration
  immediately mutes all notes and resets programs. It does not release the
  exclusive SOUND lease.
- MODM_SYNTHUPDATE (0x4001): p1=p2=0. Call from the owning Windows task's
  message pump/timer, nominally every 55 ms. The driver reads GetTickCount;
  callers do not supply time. Calls before the next quantum do no PSG I/O.
- Both messages require the current owning task and MIDI_COOKIE. Rejected
  ownership and argument checks happen before a synthesis update.
- OPEN starts in legacy mode, including after CLOSE/reopen. Updated paired
  clients require version 1.2 and explicitly select mode 3 after each open.
  An unchanged older client with this mapper therefore retains rectangular
  sound. Do not pair a new instrument client with the old 1.1 mapper.
- MODM_DATA also advances overdue envelopes before processing its MIDI
  message. Program Change is accepted in both legacy and expressive modes;
  legacy mode remembers it without applying a timbre.
- MODM_RESET immediately mutes, resets every program to 0 and resets the
  synthesis clock. It preserves the current configuration flags. CLOSE
  mutes, clears flags, and releases the SOUND lease. Every normal client
  must RESET/CLOSE before FreeLibrary.

There is no callback timer inside the DLL, no queued work after CLOSE, no
IRQ/PIT/PIC hook, and no allocation in the synthesis service. WEP only mutes
through fixed local code; it cannot safely call Windows to release a lease.
Forced termination, abnormal DLL unload or a Windows crash remain unqualified;
restart the isolated session if a lease is left stuck. Direct-PSG programs
that ignore OpenSound are outside the ownership contract.

## Original presets and Program Change

MIDI programs are zero-based below. Each group of 16 selects one original
family, a deliberately coarse mapping rather than General MIDI timbres:

- 0..15: Keys, fast attack and attenuation decay to a quiet held level
- 16..31: Organ, flat held level
- 32..47: Bass, short attenuation decay to a held level
- 48..63: Pad, roughly 330 ms rise, held level, optional vibrato
- 64..79: Reed, quicker rise, held level, optional vibrato
- 80..95: Lead, short attack, held level, optional vibrato
- 96..111: Bell, self-expiring decay, nominal 715 ms
- 112..127: Hit, self-expiring decay, nominal 440 ms

Program Change updates only its MIDI channel. A sounding voice snapshots the
selected immutable preset, base divider, velocity attenuation and start time
at Note On, so later changes affect subsequent notes only. Repeated same
channel/note retriggers its voice and snapshots the current program. Three
occupied tone slots still steal the oldest-triggered voice. A Note Off for a
different stolen channel/note does not silence its replacement. Same-key
repeated Note On/Off retains the previous mapper's retrigger semantics; there
is no stack of overlapping identical channel/note instances.

Channel 10 (zero-based 9) stays monophonic percussion, ignoring Program Change
for sound selection. Notes 35/36 use kick shaping (nominal 330 ms); 42/44/46
use hat shaping (165 ms); other accepted drum notes use snare shaping
(275 ms). The existing fixed noise clocks are retained and never steal tone
channel 2 to clock noise. Noise-control registers are written at retrigger,
not rewritten for volume-only envelope updates.

Supported melodic range remains MIDI 45..96, drums 35..81. Velocity 1..127
sets a 14..0 attenuation offset. Curves add attenuation and clamp at 15;
very quiet velocities can become inaudible before the held note is released.
Note Off, zero-velocity Note On and channel CC120/CC123 mute immediately;
there is no release tail. Ordinary CC7/11 expression, CC64 pedal, pitch bend,
pressure, SysEx/long messages and callbacks remain unsupported.

Vibrato is an eight-step triangle, nominal period 440 ms, after a nominal
440 ms onset delay. Depth is floor(base divider / 256); two depth units is
the largest excursion. It is intentionally zero for dividers below 256
(MIDI 69 and above in this table), avoiding coarse high-register warble.
Integer PSG dividers make the exact interval note-dependent. Presets share
oscillators normally; no extra voice is spent on modulation.

## Cooperative timing

This is not a precision 50/100 Hz synthesizer. A 55 ms nominal quantum matches
ordinary Windows timer service. Slow drawing, menus, logging or another
non-yielding task can delay the service. The last hardware values remain
until the owning task next runs. No software in this task context can undo
that audible gap.

On resumption the driver computes elapsed quanta once and looks up the
current shape directly. It does not loop over missed ticks or replay register
writes. Expired percussive voices are muted immediately. Held voices move to
the correct current envelope/vibrato phase. At most four voices are visited
and at most ten PSG bytes are written in one update (three latch/data pairs
plus four volumes). Unchanged values are cached. Note-on/retrigger writes
are separate from this update bound.

Captured unloaded audition service intervals had median 55 ms and 95th
percentile/maximum 109 ms after the first service. Intentional heavy redraw
plus an 880 ms non-yielding block produced gaps of 1.48 s at 25000 emulator
cycles and 5.71 s at 3000. Catch-up stayed bounded and cleanup passed, but
those gaps audibly interrupt shaping. Emulator cycle settings are not a
calibration of physical 4.77 MHz 8088 performance. Hardware CPU cost, physical
sound quality and hardware acceptance remain unmeasured.

## Paired clients

- MiniMIDI now forwards Program Change from SMF files and services synthesis
  on its existing 55 ms pulse. Its parser, Browse/Play/Stop controls, range
  limits, event budget, stale-event stop and cleanup are preserved.
- Mini Piano preserves its keyboard, mouse, octave and Panic controls. It
  uses the default Keys preset and a separate 55 ms synthesis timer; its
  original mouse safety watchdog remains independent.
- JOYMIDI preserves its controls, note mapping, velocity behavior, armed
  state and safety cleanup; its existing pulse services the Keys envelope.
- Beats preserves its controls and pattern behavior; its pulse services
  the default Keys voices and the new expiring noise drums.

The Piano, JOYMIDI and Beats UIs do not add preset selectors. To hear all
families, use Program Change in a MiniMIDI file or call the private API.
Vibrato is optional at API configuration level; paired clients enable it,
and it only applies to the eligible presets and note range described above.

## Diagnostics and test evidence

Existing DebugState queries 0..14 retain their meanings. Added queries:
15 flags; 16 serviced passes; 17 collapsed/missed quanta; 18 maximum service
gap in ms; 19 maximum bytes written by a service pass; 32..47 channel
programs; 48..51 voice preset snapshots. Counters 16..18 saturate at 65535;
existing writes/steals counters retain their 16-bit wrap behavior. A preset
query for an inactive slot is only the last snapshot, not an active note.

The final mapper is 5948 bytes, SHA-256:
a59ddd0c121d89a7d446eccc00e03f9646c3372e9505b9dcd48f6a44cbb5ac39

Build with the supplied C6/MASM/LINK4 instructions and lawful external
Windows/toolchain inputs. Fixed DLL state and near result pointers stay in
DGROUP: passing a caller-stack address to a near output parameter in this
small-model DLL is unsafe because caller SS need not equal DLL DS. The
final compiled handoff was checked independently, alongside a scoped 8088
instruction audit and native Windows tests. See the kit's validation report
for exact binaries, test configurations, results and limits.
