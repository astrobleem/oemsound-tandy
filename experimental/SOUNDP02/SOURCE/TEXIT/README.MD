# Optional Tandy pre-exit chime

TEXIT is a Windows 3.0 real-mode / 8086 one-shot companion. After confirmation
it plays its original descending phrase, mutes channels, then requests normal
Windows exit. Applications retain their save prompts and may veto. The chime
plays before those queries, so a veto may occur after the sound has played.
This returns to DOS; it does not power off the computer.

When sound is busy, normal exit offers Exit silently or Cancel in a compact
150-pixel-wide window. Cancel has initial focus; Escape cancels. Silent exit
is an explicit choice and still uses normal Windows save prompts and vetoes.
A busy /preview only reports that nothing played and never requests exit.
Sound playback now holds an OpenSound lease. A refused lease performs no
PSG writes, and cleanup releases only a successfully acquired lease.
The shared compact notice helper remains TNOTICE.C/H.

Options retain their exact lowercase spelling: /preview plays without exit;
/go skips TEXIT's own confirmation for callers such as TSHELL that already
asked. Test-only /testgo and /testpreview append diagnostic logs in disposable
fixtures. Do not put TEXIT in WIN.INI startup or a DOS post-Windows batch.

Known PSG apps or the experimental TSOUND driver cause refusal. Unknown
hardware writers cannot be discovered reliably. Choose Exit silently when sound is busy. A delayed incomplete phrase aborts without forcing exit;
Windows scheduling can delay cleanup if another task never yields. There is
no IRQ/PIT/PIC modification, busy wait, global hardware lock or forced logout.

Build with your owned period tools:

    python3 src/win30/TEXIT/BUILD.PY --repo <toolchain-root> --dosbox <dosbox-x> --out <new-build-directory>

The compact build uses Microsoft C 6.0 /G0 /Gw /AS /W3 /Od and segmented LINK4.
The prior compact-notice release was 8,687 bytes with SHA-256
78d22200c4164732902330685e8ca03a7e0ff8f961d7086f86dd4b44ae3bfe99.
The revised ownership/busy-choice candidate is a separate build; see its
trial manifest and new validation report for the current runtime identity. No binaries, licensed SDK/toolchain,
Windows/DOS files, ROMs or private user reports are included in this change.
See TESTS.MD for the bounded verification of this status-dialog update.
