# DWA Beats local validation and release gate

Unpublished client update based on OEMSound main
`18c5b271d9d24ed93d0929b23d06ea29a08e96bd`. Existing MML3AUTH/PITEXPR
release bytes are unchanged. Publish capability descriptions with the
coordinated native packet, after explicit publication approval.

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

Repository and relocated source layouts pass. PACKTEST.PY passes 20 tests,
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

Final build04 completed the full native application scenario on a disposable
640 KB HX / Windows 3.0 fixture with the published WINXT720 fixed 320x200
profile and exact shared SOUND bytes. BEATS /test passed 42 assertions:
window/control bounds, five demos, .BTS save/load, restored PIT state,
MML/clipboard, keyboard/mouse editing, all five voices, A1/55 Hz,
Stop/replay/deactivation/close and injected failure cleanup. MMLFIDL
/lowtest passed 17 assertions using the actual Beats-exported low-P MML.
The native .BTS and .MML outputs exactly matched the supplied FIVE files.
Both apps returned to DOS; the owned child stopped and lease released.
Local beats-native-03/RESULT.JSON records the full 377.9-second run.

The initial 300-second test bound expired only during its later visual
pass; it is not used as whole-run acceptance. The slower visual-only run confirmed Grid, the automatically enabled low
speaker demo and readable MML. It found the standard MessageBox wrapped
About beyond 200 pixels; the compact branding/key-hint correction now
awaits its final native rebuild and visual check. Build05 was refused by
the normal busy lease before launch; the legitimate other owner is being
waited out. No earlier binary is claimed as the final corrected app.

The final MSC6 build's executable sizes and NE allocation requests are:

| Module | File bytes | CODE | DGROUP minimum | Heap | Stack |
| --- | ---: | ---: | ---: | ---: | ---: |
| BEATS.EXE | 28997 | 23284 | 9296 | 2048 | 4096 |
| MIDIMAP.DRV | 6622 | 5367 | 674 | 0 | 0 |
| MMLFIDL.EXE | 28643 | 22150 | 22464 | 4096 | 4096 |

Each near data/code segment remains below 64 KB. These module requests do
not measure total Windows memory use; native 640 KB execution is separate.
MSC6 conversion/signed-comparison warnings remain from current/retained
code; compile/link completed successfully. No raw host GCC/cc1 was launched.

The original audit-only no-emulator rejection was unexecuted. Later app
scope explicitly authorized guarded native work; a busy legitimate owner
was waited out. An initial legacy display fixture failed before app startup;
it is not accepted. Only successful complete receipts qualify execution.

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

Publication, pushes, PRs, merges and release replacement remain unapproved.
