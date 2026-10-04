BUDDY HOLLY - TANDY PSG + TINY VIDEO (2026-10-04)

Two 8088 players share a simplified 80-bar, three-tone + noise arrangement.
Windows: 64x48 RGBI video at 4 fps. DOS: 128x96 at 8 fps, plus 64x48/4 fallback.
Full video: 241.4 seconds. Default song cue: 35.652 to 200.365 seconds.
Dialogue-only intervals are intentionally silent; alignment is approximate.

RUN
Unzip WINPLAY.ZIP on a modern computer; copy all five runtime files together:
TINYVID.EXE, MIDIMAP.DRV, WEEZER.WZV, WEEZER.WZM, WEEZER.CUE.
Launch TINYVID.EXE under Windows 3.0 real mode with a suitable Tandy driver.
Space plays/stops the song; Enter plays the entire video; Escape stops.
Focus loss, menu activation, or closing stops and releases sound.
MIDIMAP.DRV stays beside the app; no SYSTEM.INI edit is required.

DOS requires DOS 3.0+ and Tandy 1000-compatible PSG and BIOS graphics mode 9.
Unzip DOSPLAY.ZIP; exit Windows completely, enter its folder, run DOSPLAY.
DOSPLAY /F plays the entire video. DOSPLAY /A is audio-only; /V video-only.
Escape, Space or Ctrl+C stops, mutes, and restores the original display mode.
For the smaller fallback: CD LOWRES, then DOSPLAY (or DOSPLAY /F).
Keep DOSPLAY.EXE, WEEZER.WZV, WEEZER.WZM and WEEZER.CUE together.

Both stream bounded chunks; neither preloads the movie or decodes AVI.
High-resolution video is 11,870,232 bytes and needs 49,152 video bytes/s;
low-resolution video is 1,483,800 bytes and needs 6,144 video bytes/s.
Large on-disk files do not imply equivalent RAM allocation. Logs are written
in the working folder. Use a new folder rather than overwriting other apps.

STATUS
The user reports a strong positive DOS result and Windows timing struggles.
DOS is the recommended playback version. These reports are qualitative.
The exact final binaries completed the full video in a 640KB DOSBox-X Tandy
profile at 3000 cycles with no video drops. Windows coalesced 10 late music
states; DOS coalesced none. Emulator cycles are not physical CPU MHz.
See QA for exact build/preset scope and limitations. No measured physical
frame rate, jitter, removable-media throughput, or hardware certification.

BUILD.TXT describes reproduction. FORMAT.TXT documents the stream format.
ATTRIB.TXT separates original program code from third-party music/video.
SHA256.TXT hashes this overlay; each runtime ZIP has its own SHA256.TXT.
