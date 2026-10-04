# Optional Tandy pre-exit chime

TEXIT is a Windows 3.0 real-mode / 8086 one-shot companion. After confirmation
it plays its original descending phrase, mutes channels, then requests normal
Windows exit. Applications retain their save prompts and may veto. The chime
plays before those queries, so a veto may occur after the sound has played.
This returns to DOS; it does not power off the computer.

This update changes only the busy and cancelled-exit status notices to a
compact 128-pixel client dialog that fits 160-pixel displays. The original
playback, timer, PSG ownership, mute/cleanup and ExitWindows logic is retained.
The shared helper is TNOTICE.C/H, with NoticeProc exported from TEXIT.DEF.

Options retain their exact lowercase spelling: /preview plays without exit;
/go skips TEXIT's own confirmation for callers such as TSHELL that already
asked. Test-only /testgo and /testpreview append diagnostic logs in disposable
fixtures. Do not put TEXIT in WIN.INI startup or a DOS post-Windows batch.

Known PSG apps or the experimental TSOUND driver cause refusal. Unknown
hardware writers cannot be discovered reliably. Use silent/ordinary exit
when sound is busy. A delayed incomplete phrase aborts without forcing exit;
Windows scheduling can delay cleanup if another task never yields. There is
no IRQ/PIT/PIC modification, busy wait, global hardware lock or forced logout.

Build with your owned period tools:

    python3 src/win30/TEXIT/BUILD.PY --repo <toolchain-root> --dosbox <dosbox-x> --out <new-build-directory>

The compact build uses Microsoft C 6.0 /G0 /Gw /AS /W3 /Od and segmented LINK4.
It is 8,687 bytes, SHA-256
78d22200c4164732902330685e8ca03a7e0ff8f961d7086f86dd4b44ae3bfe99.
A separate rebuild was byte-identical. No binaries, licensed SDK/toolchain,
Windows/DOS files, ROMs or private user reports are included in this change.
See TESTS.MD for the bounded verification of this status-dialog update.
