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

Both stream bounded chunks; neither preloads the movie or decodes AVI.
High-resolution video is 11,870,232 bytes and needs 49,152 video bytes/s;
low-resolution video is 1,483,800 bytes and needs 6,144 video bytes/s.
Large on-disk files do not imply equivalent RAM allocation. Logs are written
in the working folder. Use a new folder rather than overwriting other apps.

STATUS
The Windows 3.0 player is the legacy app-local mapper path with cooperative
roughly 55-ms USER service; it does not use R4's newer 8-ms prepared-state
backend. The user reports a strong positive DOS result and Windows timing
struggles. These describe the original delivery; the portable DOS update has separate
qualification in DOS/RESULTS.TXT. These reports are
qualitative. See the [versioned sound timing notes](../../docs/SNDTIME.MD).
The original 2026-10-04 binaries completed the full video in a 640KB DOSBox-X Tandy
profile at 3000 cycles with no video drops. Windows coalesced 10 late music
states; DOS coalesced none. Emulator cycles are not physical CPU MHz.
See QA for exact build/preset scope and limitations. No measured physical
frame rate, jitter, removable-media throughput, or hardware certification.

BUILD.TXT describes reproduction. FORMAT.TXT documents the stream format.
ATTRIB.TXT separates original program code from third-party music/video.
SHA256.TXT hashes this overlay; each runtime ZIP has its own SHA256.TXT.
