# Compact status-dialog verification

The changed source replaces only the busy and exit-veto result MessageBoxes
with TNOTICE. The playback/PSG window-procedure region remains byte-identical
to the preceding source. The existing option and sound behavior is retained.

Warning-free MSC6 /G0 /Gw /AS build and a separate byte-identical rebuild pass.
A scoped 8088 audit checked 2,338 reachable instructions without later-CPU
suspects; imported Windows code and unresolved indirect branches are excluded.

Exact-build Windows 3.0 real-mode integration checks covered real TEXIT /go
following one TSHELL confirmation, independent application veto and later
accepted retry, known busy helper and missing-helper fallback, and shutdown
with a compact notice open. Native pixel inspection confirmed the final
notices fit 160-pixel screens with readable text and visible OK.
Genuine DOS 6.22 native-emulator TSTART launch/exit recovered the selected shell
after real TEXIT and accepted Windows shutdown. No synthetic WIN was used.

Physical acoustic quality was not newly independently tested by this dialog
change. Unknown PSG writers, non-yielding tasks and forced termination retain
the existing limitations. Use owned Windows/DOS media for native verification.
