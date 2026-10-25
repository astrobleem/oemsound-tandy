# Tandy Beats Lab validation and release gate

Qualified Tandy Beats Lab update. Application work began on OEMSound main
18c5b271d9d24ed93d0929b23d06ea29a08e96bd; both current kits are composed
against the published bank baseline c8dce49a24e83db28affccbc5f4c3b5937cb8109.
Only the scoped Beats/Fiddle/app-local mapper bytes are replaced. Shared
installer/Winamp bank bytes, companion instruments and frozen snapshots stay
exact. The user explicitly approved the presentation and publication.

## Ownership and current source

BEATLAB owns the maintained Beats editor and app-local MIDIMAP 1.3;
FIDDLE owns the matching current parser/client. Both folders receive the
same rebuilt mapper. That support module uses existing OpenSound ownership,
PSG register output and SOUND's TANDYPITVOICE service. It is not another
system driver. Shared SOUND, MIDI transport, Winamp/banks and accepted songs
are unchanged. Frozen authoring/expressive source remains historical.

The previous authoring/expressive kits already contained five channel routes
0/1/2/9/15. Their MIDIMAP was 6584 bytes (SHA256
48db106b87919fe95d24b9435faea1e978a7534803d0e56d8ff8b2fec139e516).
The root README previously linked the older four-lane preview instead of
the current authoring app/setup. The new app makes the fifth lane explicit,
automatically loads its enabled setting, and adds bounded native .BTS
save/load, existing-feature MML export and original discoverable demos.
LOWPIT.MD describes the approved speaker-only lower-note extension.

## Host and independent review

`python src/win30/BEATLAB/HOSTTEST.PY --out <new-output>` passes:

- 9824 pattern/codec/files/current Fiddle MML checks; 11 injected save
  failure/rollback/recovery checks.
- 23608 actual unchanged PITCORE checks across all 7866 accepted Hz values.
- 12 archived parser, 16972 archived mapper and 44480 current mapper checks.
  Old-range PSG/percussion/speaker traces are byte-identical.
- Five fixed-gain 0.18 reference WAVs, 144 individually AC-audible intended
  attacks, zero unintended silent attacks/clip samples, peak 0.4088. Actual
  PIT divisors drive pitch. AUDIO.JSON includes per-note/lane measurements.
  These are software references, not analog or physical-hardware capture.

Repository and relocated source layouts pass. PACKTEST.PY passes 32 tests,
including stale/failed native receipt refusal, compiled-source closure,
paired-client identities, exact shared dependency preservation and demo
creation from captured validated source rather than mutable live files.
Synthetic NE fixtures are structural tests, not runnable native evidence.

Focused independent reviews covered runtime range/routing/metadata,
cleanup, Win16 pointer ABI, read-only MML, dirty shutdown, playhead erasure,
bounded paths, receipt closure and save failure paths. Findings corrected
include MASM preflight and immutable captured-source demo generation.

## Native build and execution

BUILD.PY requires an external MSC6/MASM/LINK4 toolchain, an existing shared
emulator lease and new private output. It records 21 actual staged/original
source pins and all three runtime identities. The final native compile/link
passed via the guarded DOSBox runner with normal return 0 and lease release.
No other owner's emulator/process/lock was altered.

Functional native build06 and its Beats run passed on a disposable 640 KB HX /
Windows 3.0 fixture using the published WINXT720 fixed 320x200 profile,
its memory reservation helper and the exact shared SOUND bytes.

- BEATS /test passed 42 assertions, including window/control bounds, five
  demos, .BTS save/load, restored PIT state, MML/clipboard, keyboard/mouse
  editing, five simultaneous voices, A1/55 Hz, Stop/replay/deactivation/
  close and injected attack/update failure cleanup. Saved .BTS and exported
  .MML exactly matched the supplied FIVE demo files.
- Fiddle /lowtest passed 17 assertions on the actual Beats-exported low-P
  MML. Fiddle and mapper hashes are identical between the full combined
  run and final build06; the final subsequent Beats change was About text.
- Actual Grid, Speaker bass/PIT-on and MML screens were inspected. A slow
  visual capture caught About wrapping past 200 pixels. It was reduced to
  nine short lines; the final native capture shows all branding, key hints
  and OK inside the screen. This was a real app correction, not a warning.
- Both functional native scenarios returned to DOS. Only owned children were stopped,
  and shared leases were released. No other owner's process/lock changed.

Local evidence: beats-build-06/BUILD.JSON and PROCESS.JSON;
beats-native-03/RESULT.JSON (42 Beats + 17 Fiddle);
beats-visual-01/{GRID,SPEAKER,MML}.PNG; and
beats-native-04/RESULT.JSON / ABOUT.PNG (final 42 Beats + compact About,
304.7 seconds). Failed legacy-display, timed-bound and busy-lease receipts
are retained separately and are not treated as passing qualification.

Final runtime SHA256:

- BEATS.EXE: b6d8278ecacfa4441f8a656205ead3a9ab30d74fe39ecc79b4ee0f6888fbad31
- MIDIMAP.DRV: 2a98f7e9f91514418c9ac4d00d85462a0d95b7cbbf861e597546c4891b87623c
- MMLFIDL.EXE: c51f42992261c49169548f735c732cae068b423e0f40b31b4cc82766463f7b93

The current MSC6 build07 executable sizes and NE allocation requests are:

| Module | File bytes | CODE | DGROUP minimum | Heap | Stack |
| --- | ---: | ---: | ---: | ---: | ---: |
| BEATS.EXE | 30229 | 24068 | 9504 | 2048 | 4096 |
| MIDIMAP.DRV | 6622 | 5367 | 674 | 0 | 0 |
| MMLFIDL.EXE | 28643 | 22150 | 22464 | 4096 | 4096 |

Each near data/code segment remains below 64 KB. These module requests do
not measure total Windows memory use; native 640 KB execution is separate.
MSC6 conversion/signed-comparison warnings remain from current/retained
code; compile/link completed successfully. No raw host GCC/cc1 was launched.

## Candidate packet

```
python src/win30/BEATLAB/PACK.PY --base-kit artifacts/win30/MML3AUTH.ZIP --build <native-build> --out <new-output>
```

PACK.PY replaces only BEATS.EXE, paired MIDIMAP.DRV and MMLFIDL.EXE runtime
members. TWSETUP.EXE, TWXT2.ZIP/shared SOUND and every other base runtime
remain byte-identical. It includes five ready .BTS/.MML demos, current and
frozen regression source, manifests and separate client-update evidence.
No consumer Python/compiler/file gathering is required. Existing Fiddle
Open loads the exported MML; Beats Load opens editable .BTS patterns.

Publication was explicitly authorized after branding discussion; complete
the current-bank composition gate below before publishing.

### Common bank payload and reproducible baselines

Both kits are composed from the qualified bank publication at OEMSound
merge c8dce49a24e83db28affccbc5f4c3b5937cb8109. TWXT2.ZIP remains 2420622
bytes, SHA256 fed4fdec0bb5652a6da4c987b6b8e7a4f860f92f4a8e34aad30ac26b49fb3486;
its embedded TWSETUP.EXE remains 389040 bytes, SHA256
534790b2de96bee1d9b138d573acaad1898a231d8b7b96ef5c03a1cde48fa66f.
The common package, installer and bank DLLs are never replaced with the
older pre-bank candidate payload. VERIFY.PY checks their complete receipts
and the installer's embedded members; app/UI benchmarks are not repeated.

RELEASE.JSN records beatlab_baseline_commit and beatlab_baselines. Restore
the two original bank-baseline ZIPs from artifacts/win30 at that immutable
commit into a new private directory, preserving bytes. Current artifacts
contain finished clients and are deliberately refused as producer baselines.
For example, developer Python subprocess.check_output(['git','show',
'c8dce49a24e83db28affccbc5f4c3b5937cb8109:artifacts/win30/MML3AUTH.ZIP'])
returns the exact baseline bytes; Path.write_bytes saves them without shell
text-redirection encoding. Verify their declared pins before composing.

Use PACK.PY --kit MML3AUTH --base-kit BANK-BASE/MML3AUTH.ZIP --build BUILD
--out NEW-AUTH, and --kit PITEXPR --base-kit BANK-BASE/PITEXPR.ZIP --build
BUILD --out NEW-INSTRUMENTS. PITEXPR reads the sibling MML3AUTH baseline for
its frozen regression aliases, or accepts --regression-kit explicitly.
Both recipes preserve the current TWSETUP/TWXT2 and companion apps, and
reuse the qualified app/mapper/demo bytes. Published schema2 verification
also compares current normalized native sources, headers and actual PITCORE
with staged receipt bytes, so a later source edit cannot pass as a fresh
native build. Frozen regression aliases remain historical.

### One current Beats app in both kits

The user approved publication with Tandy Beats Lab as the primary product
name. DAWs With Attitude remains a small secondary label; LLMompton and
Dr. DRAM remain in About/README. Build07 passed native affected-UI checks:
10 name/control/close assertions and actual 320x200 Grid/About captures in
beats-native-05. Its primary title, small ready label and restored mouse
Pitch-/Pitch+ controls fit; MML hides/restores those controls correctly.

PITEXPR's shipped Beats used the same mapper 1.2, instrument curves, periods,
assembly and MIDI header as the authoring snapshot. The current app retains
mode 3, eligible vibrato, velocities 96/76, percussion, straight eighths,
75% gates and dropped-stall behavior. Restoring the mouse pitch controls
closed the real interaction gap. Companion Piano/JoyMIDI/MiniMIDI sources
accept mapper versions >=1.2, so 1.3 passes their existing checks. Host tests
already prove old-range output identity. The old 156-pixel window fitted a
160-pixel profile; the new 312-pixel editor intentionally targets 320x200.
No 160-mode compatibility is claimed. The old single demo's high C5/E5
speaker answers are superseded by the approved editable low-bass demos.

Both kits must receive identical app/mapper/demo bytes. PITEXPR retains flat
BEATS.EXE/MIDIMAP.DRV paths and its unchanged companion executables;
MML3AUTH retains BEATS/ and FIDDLE/. Kit guides and manifests reflect those
layouts. Frozen source and historical receipts retain their original scope.

Bank composition now uses the exact published payload above. Publication is
explicitly authorized; finish exact-head review/CI, publish only the scoped
app packets/catalog, and verify anonymous downloads before reporting success.
