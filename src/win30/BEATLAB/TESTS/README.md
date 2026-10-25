# Host checks

From the repository root:

```powershell
python src/win30/BEATLAB/HOSTTEST.PY --out C:/Users/chad/AppData/Local/Temp/beats-check-new
```

The output directory must be new. On Windows the runner only uses the
established `gcc-ucrt64.py` launcher with explicit `-x c`. On POSIX, `--cc`
can select a C compiler. No emulator is launched.

The runner locates the shipped regression source in the repository archive
or in the packet's sibling `SOURCE/src/win30/BEATMML` and `MMLFIDL` folders.
It passes the selected mapper filename to the harness at compile time;
the same unchanged mapper is tested from either source layout.

- `core.c` tests the real pattern codec, demo metadata, bounded output,
  transactional rejection, DOS names and save/load. It passes exported
  MML through the shipped Fiddle parser and verifies each attack's pitch,
  preset family, part and timing. Core/export/parser compilation is C89.
- `file_faults.c` injects short writes, replacement failure and failed
  rollback into the real save implementation. It checks the original
  survives, or that the backup and staged new pattern remain recoverable.
- `mapper.c` includes the unchanged shipped `MIDIMAP.C` and links its
  `INSTR.C`. SOUND, task/time and port calls are mocked. The ROM-dependent
  open path is excluded; real MIDI data, configuration, update, reset and
  close logic run. Every intended note must occupy the correct route and
  pitch, and every gate/reset/close must mute. The speaker demo verifies
  all five voices together and speaker-only answers without octave folding.

`BUILD.JSON` records commands and results. `AUDIO.JSON` records every
intended attack's peak, RMS, AC RMS, peak-to-peak signal and audible sample
count, plus lane totals, a fixed mix gain and composite clipping counts.
The five WAV files are deterministic software references reconstructed
from actual mapper register state. They use one gain for all demos,
without per-demo normalization. The noise model and mix are QA references,
not claims of exact analog sound or hardware/emulator audio capture.

These checks qualify host source behavior. Native ABI, 8088 code/data size,
layout, cooperative timing and physical audio require their separate tests.
