# Beats and the app-local MIDI instrument test

This experimental extension retains Beats' original controls and three-tone-plus-
noise budget. Read INSTR.MD for the version 1.2 mapper API, original presets,
Program Change behavior, cooperative timing and test limits. BUILD.BAT and
BUILD.PY build the mapper, Beats, the legacy API check, ownership intruder,
and the instrument/latency harness with Microsoft C6 /G0.

Keep the paired MIDIMAP.DRV beside the application. Do not install it into
WINDOWS/SYSTEM, change SYSTEM.INI, or replace SOUND.DRV. The system sound
lease remains the existing cooperative ownership mechanism.

This source snapshot is published as an experimental test kit. The stable
artifacts remain unchanged. Use the paired 1.2 runtime only in isolation.
