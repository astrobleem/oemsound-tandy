BUDDY HOLLY - DOS AND WINDOWS TINY PLAYERS

Two 8088 players share the three-tone/noise score and a sparse one-voice
speaker arrangement with written bass riffs, melody and musical rests.
Windows: 64x48 RGBI video at 4 fps. DOS: 128x96 at 8 fps, plus 64x48/4 fallback.
Full video: 241.4 seconds. Default song cue: 35.652 to 200.365 seconds.
Dialogue intervals deliberately have no synthesized notes.

WINDOWS 3.0 REAL MODE
Extract WINPLAY.ZIP to a DOS-accessible folder. From DOS, run
BUDSET C:\BUDDY (a NEW absolute 8.3 directory whose parent exists).
The installer verifies all 16 payload files before and after copying,
refuses an existing target and changes no system/INI/boot/PATH files.
In Windows File/Run, select C:\BUDDY\TINYVID.EXE. Keep its installed
files together; no global MIDI/SOUND driver registration is needed.
BUDSET /CHECK C:\BUDDY verifies an installed copy using this package.

Space starts the song, pauses or resumes; Enter starts the full video.
Left/Right seek five seconds; paused seeking stays silent. Escape stops.
Focus loss, minimizing or the system menu pauses and releases sound.
A selects Auto (PSG only for a recognized Tandy ROM, otherwise speaker).
B selects PC speaker; P explicitly selects a configured C0 PSG card.
Sound selection is independent of the display adapter. AdLib and General
MIDI playback are not implemented. On a generic XT keep the stock or a
compatible Windows SOUND manager; a Tandy-ROM-only provider can refuse
OpenSound there even with a separate C0 card. This package does not replace it.

The compact player fits a 160x100 CGA desktop with compact driver fonts.
It probes GDI bitmap format and uses lossless WZP planes when supported,
with portable WZV/DIB fallback. CGA16.BMP supplies optional 16-color wallpaper.
The speaker score is resident and prepared before playback. SPKCLK.DLL
uses SYSTEM service at about 55 ms, independent of USER video dispatch.
Optional faster timing: from DOS BEFORE Windows, load TBRIDGE.EXE, start
Windows /R, then the player. SPKFAST.DLL reuses current libwintandy broker
code at 8..55 ms deadlines on recognized real-mode Windows 3.0 SYSTEM.
Unsupported fast configurations fall back to the stock prepared service.
After completely leaving Windows, TBRIDGE /U removes your idle broker.
Never install/remove it in a Windows DOS box or remove another app's broker.
PSG uses the private app-local mapper and shared SOUND lease where present;
its ordered foreground dispatch remains cooperative USER timing.
See WIN30/README.TXT for complete installation, devices, logs and removal.

DOS
DOS requires DOS 3.0+ and an 8086/8088-compatible PC. Exit Windows before
running DOSPLAY. Automatic Tandy ROM detection chooses PSG + BIOS mode 9;
other PCs choose PC speaker + CGA, or mono text when already in mode 7.
/S speaker, /T PSG at 0C0h, /N silent; independently /C CGA, /X text,
/M mono text, /G Tandy graphics. /A audio, /V video, /F full, /H chorus.
Escape, Space or Ctrl+C stops, mutes, and restores the original display.
For original-speed CGA, CD LOWRES then DOSPLAY. Keep DOSPLAY.EXE, WEEZER.WZV,
WEEZER.WZM, SPEAKER.WZM and WEEZER.CUE together. See DOS/README.TXT for
exact capabilities, source build and qualification. AdLib and General MIDI
are future backend possibilities and are not implemented.

QUALIFICATION
Windows native emulator tests used a 4.77-MHz 8088, 640 KB, XTIDE and the
160x100 16-color CGA driver. Full fast-speaker playback: 940 frames/26 drops,
668 prepared states, none skipped; all 337 written pitches matched mixer PCM.
Stock prepared-speaker full playback: 965 frames/1 drop, 54-ms maximum
logical audio lateness. Full C0 PSG: 952 frames/14 drops, no skipped states.
The final startup fix reduced tested PSG restart lateness from 700..789 ms
to 148..217 ms; the final complete song still measured up to 273 ms late.
PSG has no prepared fast bridge. See WIN30/QA/RESULTS.TXT for exact versions,
cleanup, timing methods and retained observer/cancellation qualifications.
DOS passed its 24-case native matrix; low-resolution full CGA rendered
966 frames with zero drops. See DOS/RESULTS.TXT and DOS/QUALIFICATION.json.
These are emulated results, not physical hardware or slow-media certification.

Both stream bounded video chunks rather than preloading the movie or
decoding AVI. High video is 11,870,232 bytes / 49,152 bytes per second;
low video is 1,483,800 bytes / 6,144 bytes per second. File size is not RAM use.
Use dedicated new folders. Logs are written alongside the runtime.
BUILD.TXT describes reproduction. FORMAT.TXT documents the streams.
ATTRIB.TXT separates original code from third-party music/video.
SHA256.TXT covers this tree; runtime ZIPs have their own manifests.
See WIN30/QA/RESULTS.TXT and DOS/RESULTS.TXT for versioned evidence.
