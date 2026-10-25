# Fifth-lane bass: bounded correction proposal

The A2 floor is an inherited admission restriction, not the current shared
SOUND service's pitch floor. No mapper source has been changed in this
proposal; coordinate that change with the parent before implementation.

## Current code evidence

- `src/win30/DRIVER/PITCORE.H:pitSet` admits whole-Hz 37..7902 and computes
  the PIT divisor with `(1193182UL + hz/2) / hz`; no shared-driver change is
  needed. `PITAPI.H` documents the same range.
- The shipped authoring mapper's `noteOn` rejects outside45..96 before
  its `pitEnabled && channel==15` branch. Its 52-entry PIT frequency table
  covers only that interval. PSG and speaker are unnecessarily conflated.
- The new app's row5 admission and existing `BeatExportPit` repeat45..96.
- Fiddle `MMLCORE.C` also applies45..96 to the P part; its application
  requires exactly mapper version0x0102. Extending only the Beats UI/mapper
  would leave low-note MML exports incompatible with the current companion.

## Preferred small change, after coordination

Keep PSG melodic45..96 and noise35..81 unchanged. Permit PC-speaker notes
26..96 only when PIT is opted in on MIDI channel16. MIDI26/D1 rounds to
37Hz; MIDI25/C#1 rounds to35Hz and is below the published service contract.
Retain every existing45..96 table entry, prepending only19 low entries.
Use A1/C2/D2 (MIDI33/36/38, 55/65/73Hz) in the speaker-bass demo without
folding or a doubled PSG bass. Set new speaker cells to A1 by default.

Maintain this mapper as an isolated current Beats source copy under
BEATLAB; do not edit frozen shipped snapshots or the active Winamp synth.
A version/capability check must reject an old mapper before sending a
required low note; ordinary four-voice behavior remains compatible.

To retain useful MML export, accompany this with the equally lane-specific
P-part range correction in a current Fiddle parser/app copy and matching
native client rebuild. Preserve old MML1/2 outcomes, reject low PSG notes,
and leave P volume binary and envelopes/presets unchanged. This requires
coordinating the Fiddle/mapper update and revising the current candidate
packager's intentional Fiddle/MIDIMAP byte-preservation guard. It is not a
shared SOUND, bank, timer or transport change.

If that adjacent client change is deferred, keep low bass usable in Beats
and .BTS but explicitly reject unsupported low-note MML export; never
silently transpose it or emit a score the current Fiddle cannot play.
The preferred complete chain avoids that new interoperability limitation.

## Focused acceptance

Exercise note25 rejection, note26 minimum37Hz, 33/36/38 bass, old45..96
identity, PIT-off and non-speaker-channel low-note rejection, note-off,
retrigger/reset/close, busy/missing service and all five simultaneous
routes. Preserve .BTS round trips and PIT state, and test exported low P
notes through the actual revised Fiddle parser. Re-run per-note AC-signal,
correct-octave, identical-gain balance and clipping checks. Native UI,
playback and clean return remain separate shared-lease gates.
