Current download: [Tandy Winamp XT1](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-xt1.20261024). [Install guide](INSTALL.TXT).

# Tandy Winamp UX3

[Download TWUX3.ZIP](https://github.com/astrobleem/oemsound-tandy/releases/download/tandy-winamp-ux3.20261018/TWUX3.ZIP) or open the [release page](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-ux3.20261018).
The package contains the accepted app, matching audio helpers, corresponding
first-party source and notices, and already-public original synthetic examples.
Read [INSTALL.TXT](INSTALL.TXT) before launching on an existing qualified profile.

- Use **Add...** to browse for a supported MID, VGM, VGZ, MOD, TND or SND file.
- **Remove** removes a playlist entry; its file stays intact. Removing the active
  entry lets that song continue until Stop, EOF or another Play.
- The session playlist holds **32 entries**. Scroll or use Home, End, Page Up and
  Page Down to reach songs beyond the visible rows.
- The window title stays **Tandy Winamp**; status stays inside the window.

UX3 received qualitative physical acceptance on the user's Tandy 1000 EX after
the requested Browse/Add, Remove and scroll-past-four steps on 2026-10-18.
The user reported "that worked great publish it". See the [scoped record](EVIDENCE/PHYSICAL.JSON).
This does not certify every format, all 32 slots, physical timing, or the pending
combined display-driver build. Windows XT release-candidate work remains separate.

Recorded validation passes 149 portable C89 checks, 396 native UI checks and
88 independent gzip/PSG/refusal/EOF/ownership checks. The native UI cases cover
the visible browser, scrolling/keyboard navigation, and continued playback while
browsing and removing the active entry. The production browser and eight parser/
format source units remain unchanged from their qualified inputs. See
[behavior](WINAMP-UI.MD), [building](BUILDING-UI.MD), [validation](VALIDATION-UI.MD)
and [runtime pins](RUNTIME.JSON). Public CI verifies existing package inputs;
it does not compile the period native toolchain or repeat physical testing.

The app derives from the published [libwintandy format preview](https://github.com/astrobleem/libwintandy/tree/24ff4f932c0d4d4e2a18a3c49b0ea0a8ff90e71f/examples/FORMATS30).
The application-local LWFILE browser is the first-party Win3.0 implementation;
no Win3.1 COMMDLG is needed. Existing MIT and component notices are retained.
The external TSOUND driver retains its own published license and is not bundled.
No commercial/private songs, sampled media, Windows, SDK/toolchain, font, ROM or
disk image is included. MOD support is a bounded PSG arrangement, without sample
timbres; supported-format limits remain in the package's FORMATS.MD.
