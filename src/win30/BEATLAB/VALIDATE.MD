# DWA Beats local validation and release gate

This is a source/host candidate based on OEMSound main
`18c5b271d9d24ed93d0929b23d06ea29a08e96bd`.
The currently published MML3AUTH download still contains the previous Beats
client. Do not publish the new capability descriptions before the updated
native client and complete kit pass their separate gates.

## Established behavior and preserved dependencies

The current published release asset digests matched this checkout's
MML3AUTH and PITEXPR packets. Both contain the same 6584-byte app-local
MIDIMAP.DRV, SHA256
`48db106b87919fe95d24b9435faea1e978a7534803d0e56d8ff8b2fec139e516`.
Its NE imports include SOUND. Actual source takes OpenSound ownership,
outputs PSG registers through its existing assembly helper, and resolves
SOUND's TANDYPITVOICE for PC speaker. It is not a second system driver.
The authoring BEATS.EXE is 21845 bytes; the expressive-kit BEATS.EXE is
11631 bytes. Both binaries contain the five-lane MIDI channel map
0/1/2/9/15. Shipped source agrees after line-ending normalization.

The frozen authoring/expressive source, old packets, shared SOUND driver,
MIDI mapper/transport, accepted song data and active Winamp work are unchanged.
New maintained application source is isolated under src/win30/BEATLAB.

## Completed host checks

- `python src/win30/BEATLAB/HOSTTEST.PY --out <new-output>`:
  9807 strict-C89 codec/file/demo/MML checks, 11 injected save/rollback
  checks, 27403 checks through the unchanged actual mapper, and 144
  individually audible attacks across five reference WAVs.
- Fixed gain 0.18 for every demo: zero silent attacks, zero clipped samples,
  composite peak 0.4088. Per-note AC signal, route, pitch, gate and per-lane
  measurements are in AUDIO.JSON. This is software QA audio, not hardware
  capture or an analog-fidelity claim.
- Speaker notes 45/48/50 are 110/131/147 Hz without folding or doubling.
  All five voices coexist at steps 1/5; odd-indexed step positions contain
  speaker-only attacks. The current mapper does not admit pitches below A2.
- Repository and relocated packet-like source layouts pass the same suite.
- `python src/win30/BEATLAB/PACKTEST.PY`: 13 packaging tests, including
  stale/absent native-build refusal and exact shared/Fiddle preservation.
  Synthetic native test fixtures are not a runnable product deliverable.
- Open Watcom 16-bit Windows syntax compilation of BEATS.C and BEATPAT.C
  passed without warnings. This is not an MSC6 link or native runtime pass.
- Focused independent review findings were corrected: MML refresh and
  read-only handling, dirty shutdown, playhead erasure, bounded app path,
  staged-source receipt hashes and Win16 string-pointer ABI.

## Native and publication gates still pending

Automatic approval review denied the DOSBox-based BUILD.PY invocation
before execution because the earlier explicit no-emulator instruction had
not been lifted. No emulator started. No native binary or updated candidate ZIP was
produced. An explicit permission question is pending.
Do not retry that action until new applicable approval is received.

After an explicit exception, use the existing shared emulator lease and
an isolated toolchain/output. BUILD.PY accepts --dosbox, --toolchain,
--lease and --out; it stages only this app's compilation and pins the
actual compiled bytes. Native execution must use a disposable 640 KB
Windows 3.0 fixture with the exact current shared SOUND and paired mapper.
Run BEATS /test for real window/control bounds, mouse/keyboard editing,
save/load of the fifth-lane setting, MML/clipboard, five-voice playback,
Stop/replay/deactivate/close and injected attack/update-failure cleanup.
Inspect actual 320x200 Grid, MML and About screens for readable controls.
Record executable hashes, CODE/DGROUP/heap/stack and acceptance results.

Only then prepare a new local candidate with:

```
python src/win30/BEATLAB/PACK.PY --base-kit artifacts/win30/MML3AUTH.ZIP --build <native-build> --out <new-output>
```

PACK.PY requires matching staged/current source pins, executable identity
and a successful shared-lease process receipt. It preserves Fiddle,
MIDIMAP, TWSETUP and TWXT2 exactly, supplies ready .BTS/.MML demos and
current source, and records the client change separately from historical
unchanged-client receipts. Parent coordination is required for any shared
package refresh. Push/PR/merge and release replacement still need explicit
publication approval.
