# Host checks

From the repository root:

```powershell
python src/win30/BEATLAB/HOSTTEST.PY --out C:/Users/chad/AppData/Local/Temp/beats-check-new
```

The output directory must be new. On Windows the runner only uses the
established `gcc-ucrt64.py` launcher with explicit `-x c`. On POSIX, `--cc`
can select a C compiler. No emulator is launched.

The runner uses current `BEATLAB/MIDIMAP.C` / `INSTR.C` and the sibling
`FIDDLE/MMLCORE.C`. It also requires the unchanged shared
`DRIVER/PITCORE.H` and archived `BEATMML` / `MMLFIDL` regression sources.
The archive is found in the repository or as packet source siblings.
Selected mapper/PIT filenames are passed to harnesses at compile time,
so the same source is tested from either layout.

- `core.c` tests the real pattern codec, demo metadata, bounded output,
  transactional rejection, DOS names and save/load. It passes exported
  MML through the current Fiddle parser and verifies each attack's pitch,
  preset family, part and timing. Low `[P]` notes are admitted only there;
  PSG parts retain their old range. Stored low speaker notes survive
  save/load even when PIT is disabled; disabled export omits `[P]`.
  Core/export/parser compilation is C89.
- `file_faults.c` injects short writes, replacement failure and failed
  rollback into the real save implementation. It checks the original
  survives, or that the backup and staged new pattern remain recoverable.
- `mapper.c` runs against the current app-local mapper and its unchanged
  instrument curves, then against the archived mapper. SOUND, task/time
  and port calls are mocked. The ROM-dependent
  open path is excluded; real MIDI data, configuration, update, reset and
  close logic run. Every intended note must occupy the correct route and
  pitch, and every gate/reset/close must mute. The speaker demo verifies
  all five voices together and 55/65/73 Hz speaker-only bass answers without
  octave folding. Boundary 25/26, disabled PIT, other channels, stale
  note-off, reset/close, missing service, wrong owner and busy-gate cases
  are explicit checks. Old-range PSG, percussion and PIT traces must be
  byte-identical between the current and archived mapper.
- `pit_service.c` includes the actual shared `PITCORE.H`, tests all
  7,866 accepted frequencies from 37 through 7902 Hz, verifies nearest
  divisors and checks invalid/busy requests preserve hardware state.
  The same actual core backs the mapper's mocked SOUND speaker callback.
- `legacy_parser.c` establishes that the archived parser rejects the new
  low-P demo while continuing to accept the old range and other demos.

`BUILD.JSON` records commands and results. `AUDIO.JSON` records every
intended attack's peak, RMS, AC RMS, peak-to-peak signal and audible sample
count, plus speaker divisors, actual/nominal pitch, lane totals, a fixed
mix gain and composite clipping counts. Speaker reference pitch comes from
the actual programmed divisor, including its documented quantization.
The five WAV files are deterministic software references reconstructed
from actual mapper register state. They use one gain for all demos,
without per-demo normalization. The noise model and mix are QA references,
not claims of exact analog sound or hardware/emulator audio capture.

These checks qualify host source behavior. Native ABI, 8088 code/data size,
layout, cooperative timing and physical audio require their separate tests.
