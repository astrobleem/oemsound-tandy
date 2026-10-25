# Tandy Winamp

**[Download TWXT1.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-xt1.20261024/TWXT1.ZIP)**. The player, TSOUND driver and matching
RESERVE helper are included. HPXY/XRSYNC video reservation is automatic;
stock CGA uses the same launch command without reservation.

## Install and start

1. Extract `TWXT1` to `C:\TWXT1` and exit Windows completely.
2. Copy the included TSOUND driver to Windows SYSTEM and select it once in
   SYSTEM.INI, following the [short install guide](INSTALL.TXT).
3. From plain DOS:

   ```dos
   C:\TWXT1\TPLAY C:\WINDOWS /CHECK
   ```

   After **preflight PASS**:

   ```dos
   C:\TWXT1\GO
   ```

Use your actual Windows directory; `GO C:\W30SND`, for example, selects it.
No extra WINXT launcher or separate sound-driver download is needed.

## Using the player

Add... opens the file browser. Remove changes the playlist, not the file;
an active removed song continues until Stop or its end. Scroll or use
Home/End/Page Up/Page Down to reach all 32 playlist slots. Stop and close
the player, then exit Windows normally to return to DOS.

The packet includes the player, audio helpers, launcher and drivers. Current native
[test results](LAUNCH/TESTS.JSON) cover HPXY/XRSYNC playback during redraws,
reservation/reuse and cleanup; the earlier [physical EX report](EVIDENCE/PHYSICAL.JSON)
retains its original scope.

[Supported formats](../../docs/FORMATS.MD) · [Build and policy](LAUNCH/POLICY.MD) ·
[Release](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-xt1.20261024).
Source and component notices are included.
