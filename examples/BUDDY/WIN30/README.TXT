BUDDY HOLLY FOR WINDOWS 3.0 REAL MODE

Requires an 8088-compatible PC, 640 KB, Windows 3.0 real mode, and a
16-color display driver. The 64x48 video targets 4 frames/second.
The compact window fits the 160x100 CGA desktop with its compact fonts.
Windows and display drivers are not included.

INSTALL
Extract WINPLAY.ZIP to a DOS-accessible directory. In DOS,
change to that directory and run BUDSET C:\BUDDY (or another NEW absolute
8.3 directory whose parent exists). BUDSET verifies the complete payload,
refuses existing targets, copies and verifies the result. A failed copy
removes only the files it created in that new directory. Source corruption
is rejected before creating the destination. It changes no system files,
INI files, PATH, boot files, or registered MIDI/SOUND drivers.
BUDSET /CHECK C:\BUDDY verifies an installed copy against this package.
Keep the original extracted package if you want to run /CHECK later.

RUN
In Windows, File/Run: C:\BUDDY\TINYVID.EXE
Space starts the song, pauses, or resumes. Enter starts the full video.
Left/Right seek five seconds; a paused seek stays paused. Escape stops.
Minimizing, losing focus, or opening the system menu pauses and releases
sound. Resume reconstructs current state without replaying old attacks.
The song occupies 35.652..200.365 seconds of the 241.4-second movie.
Intro/outro dialogue intervals deliberately have no synthesized notes.

SOUND (independent of the display adapter)
A = Auto. A recognized Tandy ROM selects Tandy PSG; otherwise the one-voice
PC speaker is selected. A lease API reports availability, not hardware.
B = PC speaker. Uses the arranged low melody and written bass riffs with
rests. P = a configured Tandy-compatible PSG at port C0. Explicit P is for
a Tandy or a card you know is installed; it does not probe or assume a card.
Command letters B/P select the same devices at startup. Example:
TINYVID.EXE PF plays the full video using the configured PSG card.
On a plain XT, retain the stock Windows SOUND manager for speaker or
explicit-card playback. A Tandy-ROM-only SOUND provider can refuse
OpenSound on an XT; this installer does not replace that system driver.
AdLib and General MIDI playback are not implemented in this player.

SPKCLK.DLL prepares the bounded speaker score before playback and services
it in a fixed resident SYSTEM callback, independent of USER video work.
On stock Windows 3.0 SYSTEM, service is about 55 ms. No PIT0 reprogramming.
MIDIMAP.DRV is a private app-local 3-tone/noise mapper. It participates in
the shared SOUND PSG lease when that API exists. PSG snapshots are applied
in order with bounded foreground catch-up; this path remains USER-timed.
The optional queue/client contract in source is not a working PSG bridge.

OPTIONAL FASTER SPEAKER CLOCK
From a genuine DOS prompt BEFORE starting Windows, run this package's
TBRIDGE.EXE once. Then start Windows /R and run TINYVID. SPKFAST.DLL uses
the existing libwintandy broker/hook implementation, with audio PSG writes
disabled, and schedules prepared speaker deadlines at 8..55 ms intervals.
Only a recognized, real-mode Windows 3.0 SYSTEM image is supported; the
resident 55 ms service remains the fallback if this opt-in is unavailable.
It opens the same single SOUND owner, not a second instrument transport.
After leaving Windows completely, TBRIDGE.EXE /U removes an idle broker.
Only remove a broker that YOU installed for this session. Do not remove
one belonging to another program. An existing resident broker is reused.
Never load/unload the DOS broker inside a Windows DOS box.

VIDEO AND WALLPAPER
WEEZER.WZV is the original packed RGBI video. WEEZER.WZP contains the same
966 frames in lossless native planes. The player probes its actual GDI
bitmap format before selecting SetBitmapBits; it falls back to WZV/DIB
conversion when needed. WZP is optional if keeping only the portable path.
CGA16.BMP is an original 16-color harbor wallpaper. Select it manually in
Control Panel/Desktop if desired. The installer does not change wallpaper.
The player uses an instance-bound Windows callback and the SYSTEM font;
the old stray question marks came from an incorrect callback DS binding.

LOGS / REMOVAL / LIMITATIONS
TINYVID.LOG records device, timing, frame drops, and teardown results in
the app directory. TVREADY.LOG, TVSTART.LOG, TVMARK.LOG, BDINFO.LOG and
BDPROBE.BIN are small diagnostics and may be overwritten on the next run.
To remove: exit the player and Windows, remove only this package's files
from its dedicated installation directory, then remove the empty directory.
Preserve your own files, modified media, logs, and any preexisting broker.

See QA qualification receipts for exact measured builds/profiles and gaps.
Emulation is not physical-hardware qualification. Build/package this source
with source/BUILD.PY and PACK.PY. WINSRC.ZIP supplies corresponding source.
See ATTRIB.TXT for video/music attribution. No sampled soundtrack is used.
