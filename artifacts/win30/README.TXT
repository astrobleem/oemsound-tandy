TANDY WINDOWS 3.0 PSG MILESTONE
=============================

Three bounded components for known original Tandy 1000 EX/HX PSG hardware:

1. PSGPLAY.EXE: a native Windows 3.0 / 8088 Play/Stop demo.
   No driver installation. Run in real-mode Windows, choose Play or P.
   Stop or S/Escape mutes all four PSG channels. Close also mutes.
   Only one PSG-producing program should run at a time.

2. TSOUND.DRV: an EXPERIMENTAL restricted legacy Windows SOUND driver.
   It is not a full system-sound replacement. Use ONLY in a cloned test
   installation initially. Read SOURCE/DRIVER/README.MD before installing.
   Keep your working display driver and stock sound driver intact.

3. TCHIME.EXE: optional standalone startup chime, about 0.9 seconds of audio.
   It runs asynchronously, mutes all four channels and unloads itself.
   No experimental driver installation is required. Try it manually first;
   SOURCE/CHIME/README.MD explains optional WIN.INI enable and disable.
   It does not alter startup settings itself. STARTUP.WAV is its preview.

DEMO.WAV is a captured emulator rendition of PSGPLAY's C/E/G sequence.
The application does NOT play WAV files. It writes notes to the PSG.

IMPORTANT LIMITS
- Windows 3.0 real mode and 8086/8088 target; no Win3.1/MME/MIDI support.
- No physical EX/HX listening or hardware timing qualification has occurred.
- TSOUND does not implement stock beep, noise, envelopes or SyncAllVoices.
- Abrupt owner termination is not qualified. A killed owner may leave the
  driver unavailable until Windows is restarted. Normal Stop/Close/WEP and
  note-off paths are tested; do not treat that as crash recovery.
- PSGPLAY uses cooperative Windows timers. Slow redraws or blocked tasks
  extend a stage; the app never skips a voice to catch up. It is a tiny
  hardware demonstrator, not a precise performance benchmark.
- PSGPLAY overwrites C:\PSGTRACE.LOG with its bounded diagnostic trace.
- /test, /stoptest and /closetest are fixture automation switches which
  close Windows. Use only in disposable test sessions without unsaved work.

VERIFIED NATIVE DEMO
- 6,287-byte EXE, /G0 /Gw /AS C build; repeat build identical.
- All five existing display profiles: full three-voice trace and actual
  audio bands at approximately 262, 330 and 392 Hz.
- Manual Play/replay/Stop plus automated Stop/close-during-playback.
- All-channel mute on normal stop/close; stable IRQ8/IRQ1C snapshots and
  consistent BIOS/Windows clock deltas in seven emulator cases.
- Cookie coexistence: its saved state advanced from 100 to 114 at 1 CPS,
  with all three PSG voices and final mute independently traced.
- 157 isolated host logic assertions for failure and lifecycle branches.
- Scoped binary ISA audit: 1,738 reachable instructions, no post-8088
  suspects, sole hardware output at C0h. This is not an exhaustive proof
  of imported Windows code or physical behavior.

VERIFIED EXPERIMENTAL DRIVER
- 5,656-byte fixed-code/data driver; 105 native checks, zero failures.
- Three queues, ownership isolation, full-queue rejection, note/rest timing,
  thresholds, repeated open/close, active Stop and timer removal passed.
- Captured simultaneous C/E/G chord and final quiet tail; CHORD.WAV is a
  short excerpt, and the full capture is retained in EVIDENCE/DRIVER.
- No private IRQ/PIT/PIC hooks; stock SYSTEM timer scheduling.
- SYSTEM timer exhaustion and abrupt task death remain unqualified.

VERIFIED STARTUP CHIME
- 3,999-byte EXE; fresh rebuild identical.
- Startup in 160x200x16, 640x200x4 and baseline; all three notes captured.
- An observer starts concurrently while the chime is loaded but invisible.
  Afterwards its module and hidden window are gone; no resident remains.
- Disabled-startup test produces digital silence.
- 79 host failure/cleanup assertions, 1,057 scoped 8088 instructions audited.
- Slow initial painting is allowed to finish before the first tone starts.
  During playback, an overly late timer stops rather than piling up notes.
  Frozen/non-yielding Windows can still delay cleanup; no hard real-time claim.

BUILD / SOURCE / EVIDENCE
SOURCE/README.MD has native demo build/run details.
SOURCE/DRIVER/README.MD has restricted driver API, fixture install/rollback.
EVIDENCE contains raw traces, captured audio, test/build results and audit
scope. No OS installations, stock DDK sources, proprietary toolchain or
Windows binaries are included. Bring your existing licensed dependencies.
No working display driver has been replaced. See PUBLICATION.TXT when present
for the public draft PR receipt; this archive may predate that receipt.

LICENSING
The existing upstream README says MIT, while its LICENSE contains GPLv3.
LICENSE.TXT preserves that existing license text. This milestone does not
resolve or alter those conflicting project claims. Existing repository
notices remain unchanged. Newly added source accompanies the binaries.

BASELINE
https://github.com/astrobleem/oemsound-tandy/commit/e811c22a6bbeb7bdbe2a04ed5bc6e713ba90e5c6
