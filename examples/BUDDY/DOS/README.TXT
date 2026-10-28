BUDDY HOLLY FOR DOS - PORTABLE DOS UPDATE, 2026-10-28

STATUS: See RESULTS.TXT and QUALIFICATION.json for the exact qualified build.

Run DOSPLAY from its runtime directory after exiting Windows completely.
DOS 3.0 or later is required. Keep DOSPLAY.EXE, WEEZER.WZV, WEEZER.WZM,
SPEAKER.WZM and WEEZER.CUE together. Media files have not changed format.

DOSPLAY              Automatic sound/display; original song cue.
DOSPLAY /F           Entire 241.4-second video, including dialogue rests.
DOSPLAY /A           Audio only.
DOSPLAY /V           Video only.
DOSPLAY /S           PC speaker and the sparse monophonic bass arrangement.
DOSPLAY /T           Tandy-compatible PSG and the original four-voice score.
DOSPLAY /N           Silent playback.
DOSPLAY /C           BIOS mode 4: CGA 320x200, four colors.
DOSPLAY /X           BIOS mode 3: color text character-image fallback.
DOSPLAY /M           BIOS mode 7: monochrome text fallback.
DOSPLAY /G           BIOS mode 9: Tandy 320x200, 16 colors.
DOSPLAY /H           Short chorus check, 63.395 to 87.174 seconds.

Combine switches, e.g. DOSPLAY /S /C /F. Escape, Space or Ctrl+C stops.
Relaunch to restart. Sound and display overrides are independent: /T /C
allows a compatible sound expansion with ordinary CGA graphics. Do not
force a hardware backend or mode your machine does not provide.
The PSG sink writes I/O port 0C0h. Adapters using a different address or
an LPT connection need a separate backend; /T does not configure those.

Automatic selection retains the original Tandy ROM signature check.
Other machines select PC speaker, and either CGA or text if already in
monochrome mode 7. A failed mode set exits with an error; /X or /M offers
an explicit text fallback. EGA/VGA can use their CGA-compatible mode;
native EGA/VGA graphics, Hercules graphics and PCjr detection are absent.

The main WEEZER.WZV is 128x96 at 8 fps. LOWRES uses the existing 64x48,
4 fps video. Each is centered on the selected display. CGA reduces RGBI
to black/cyan/magenta/white. Text samples each 2x4 source block into a
character and color. Source frame deadlines determine video selection;
slow machines skip obsolete frames rather than slow the music clock.
For CGA on an original-speed 8088, CD LOWRES then run DOSPLAY. The higher
resolution asks for more disk bandwidth and drawing time; music deadlines
remain independent. Measured frame rates and complete-run scope are in RESULTS.TXT.

Sound uses a fixed 128-slot ring and libtandy's 120 Hz DOS update clock.
Foreground code reads and validates prepared WZM full-state events.
Each app callback consumes at most eight states per tick; the library may
drain multiple pending callbacks after a delay. This is not an arbitrary
overload guarantee. The app callback
performs no file/DOS/BIOS calls. The library chains the prior BIOS IRQ0
handler at its normal average rate. A drained queue silences until refilled.
Timer close restores the BIOS PIT divisor and the preceding IRQ0 vector.
PC-speaker stop clears port 61 bits 0 and 1 while preserving upper bits.

The PSG backend uses the current libtandy chip writer. The unchanged
clock adds its documented 3,072-byte private stack; the 128-slot queue
is 1,792 bytes. This evidence build also retains a 12,288-byte bounded
event trace. No static-score conversion or general MIDI synthesizer is
linked: retaining WZM timestamps preserves dialogue mutes and tempo maps.
The selected library objects contribute 1,716 code bytes and 3,112 static
clock bytes (including that private stack), before linker alignment.
The small chip API preserves the old byte sequence without pulling in
the larger channel/score/effect facade. Existing library presenters do
not supply this CGA mode-4 conversion, so that path remains in the app.

Its complete 1,342-state output matches the previous DOS algorithm byte for byte in
the host compatibility check. The speaker backend shares the current
Win16 sparse arrangement: lowered melody and written bass riffs, with
articulation gaps, no chord-root filling of melody rests, and no drum
imitation. The speaker has fixed output amplitude; velocity is not a
volume control. Recognizability needs a human audition.

AdLib and General MIDI are not implemented. The AUDIO interface permits
a future device backend; new device code and suitable arrangement work
would still be required. Only PSG, speaker and silent are selectable.

BUILD
The repository DOS/HOST/BUILD.PY uses an external Open Watcom 1.9 install:
python DOS/HOST/BUILD.PY --watcom C:\WATCOM --out C:\new-private-dos-build
It targets 8086 small model, near cdecl library callbacks, with C16 -0 and
WASM -0. The Windows DLL toolchain and projects are unchanged by this DOS
port. No emulator is started to build. Keep generated objects/maps outside
the source tree. SOURCE/BUILD.BAT is the equivalent Windows cmd.exe compiler recipe
for a private source copy, with WATCOM environment and PATH set.

HOST TESTS
python DOS/HOST/HOSTTEST.PY --launcher C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py --out C:\new-private-host-check
On other hosts use --cc gcc instead of --launcher. The test checks all PSG
bytes against the legacy mapper, speaker ports/gates, silent output and a
clock model. VALIDATE.PY <runtime-folder> validates media. AUDIT.PY <EXE>
requires GNU objdump on PATH and the corresponding Open Watcom MAP file;
its scoped reachable-code audit is not a physical-hardware certificate.
Python is a developer tool, never a playback requirement on the DOS PC.

SOURCE AND EVIDENCE
DOS/SOURCE contains the player and sinks. DOS/libtandy holds the unchanged
selected chip/clock services, exact revision/hashes, MIT license and source
provenance. Runtime LIBTANDY.TXT retains the complete MIT notice.
Fixed storage includes a 3072-byte private clock stack and
4096-byte application stack. The ring holds 127 usable prepared states.
Source integration and resource checks accompany the publication packet.
The native evidence archive includes lossless PCM, raw VRAM, logs, vector
snapshots and OS-free stopped FAT copies. Its REPRODUCE.TXT describes
regenerating waveform/content/timing analysis without an OS image or ROM.

This is a local publication candidate. No one-floppy work was performed.
Third-party music/video attribution in ATTRIB.TXT continues to apply.
