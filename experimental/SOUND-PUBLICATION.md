# SOUNDP01 and SOUNDP02 source publication

This is an opt-in experimental source/evidence publication, not a stable
runtime release or an installation update. Baseline sources, runtime ZIPs,
WININST12, BUDENV02, startup settings and the root README are unchanged.

The two directories preserve distinct tested checkpoints, not sequential
overlays. SOUNDP01 covers sound ownership in PSGPLAY, the startup chimes and
legacy/expressive Beats. SOUNDP02 covers the instrument UI, Mouth, TEXIT and
Windows Buddy. Their production app source sets do not overlap. Build each
selected component with its own instructions and matching app-local mapper;
never install a mapper in WINDOWS/SYSTEM.

All included checkpoint files are byte-identical to their qualified kits.
The original README.TXT and MANIFEST.JSON files describe those complete kits,
including runtime and media files deliberately omitted here. The original
manifests are historical evidence, not an inventory of this source-only
publication. SOURCE-PUBLICATION.json lists exactly the included files.

Included: component source/build scripts, original license notices, QA source
and runners, and retained build/ISA/test receipts. Omitted: runtime binaries,
MIDI/binary media fixtures, screenshots, and QA executables. Native QA requires
the matching original fixtures plus lawful external Windows, period compiler
and emulator inputs; this source-only extraction is not a turnkey QA kit.
No compiler, operating-system image, third-party video, song audio, user files
or secrets are published.

Qualification is historical, not a fresh build or emulator rerun:
- SOUNDP01: six native Beats cases plus the accepted owner test; five scoped
  ISA audits. Busy refusal emitted zero PSG writes in the three direct apps.
- SOUNDP02: 41 accepted native cases; nine scoped ISA audits; host evidence
  reports 677 SMF, 100,036 Piano and 194,837 joystick checks.
- Native configuration: Windows 3.0 real mode, 640 KB, 8086_prefetch, no
  XMS/EMS/UMB/FPU and fixed 3000 emulator cycles. These are not calibrated MHz.
- Physical Tandy sound, latency and joystick acceptance remain open.
  Full original Buddy media playback was not rerun for the label changes.

Publication validation checked every file against its original manifest,
confirmed both kits use qualification baseline
b25d5bce188cf63a3eb7a5ae9bab68147a862cd2,
and preserves exact source bytes without new implementation changes.
