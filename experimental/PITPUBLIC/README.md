# Public experimental sound packages

These freshly prepared original sound kits are published separately from the
baseline starter preview. [Download the experimental prerelease](https://github.com/astrobleem/oemsound-tandy/releases/tag/sound-exp-v0.1.0-pit.20261007).

| ZIP | Qualified content | Exact original source |
| --- | --- | --- |
| PITWIN.ZIP | Seven Windows clients, baseline mapper and shared PIT SOUND capability, stages 2/3 | sound-voice `02eaa8b11b14f1a8e216b7d3a6e7201aa0f5548a` |
| PITEXPR.ZIP | Expressive Beats/Piano/JoyMIDI/MiniMIDI opt-in extra voice, stage4 | expressive-voice `f580eb22f0902e1c018d0335184273d7a4f5d833` |
| MML3AUTH.ZIP | Fiddle/Beats MML3 fifth-part authoring/playback, stage5 | authoring-voice `d73aca98821d4653f2baaaa9799ba9c147f8a499` |
| PITDOS.ZIP | PSGTEST `/pit` through DOSSND v1, stage6 | dos-psg-voice `91426b8ff3f5cdc91ebb01f6f7f6a31226c87512` |

Every ZIP contains runtime, corresponding selected complete source, original
license notices, historical text QA and a current scope/manifest. PITEXPR and
MML3AUTH include the same qualified TSOUND runtime and matching stage3 driver
source. Windows PIT playback requires installing TSOUND **only in a disposable
copy of an existing lawful Windows 3.0 real-mode guest**. App-local MIDIMAP stays
beside matching clients. [INSTALL.TXT](INSTALL.TXT) describes prerequisites,
controls, clean setup and rollback; nothing installs automatically or changes CF.

PIT options remain off by default. Sound is fixed on/off; no independent
amplitude envelope. Speech/PWM shares PIT2 and is mutually exclusive with PIT
music. System beeps/direct writers can bypass leases. Emulator qualification
is bounded; physical EX summing/fidelity, real joystick/key input, custom-display
UI, abrupt termination and complete system integration remain open. TSOUND
has restricted SOUND semantics, including unsupported system beep functions.

`SOURCE-PINS.json` identifies every curated source file; `INPUTS.json` records
historical archive/receipt identities. Git bytes are preserved in this tree;
source text was compared to the qualified working-tree source archives with
line endings normalized for comparison only. Runtime bytes are unchanged.
CI checks public source identities and local include closure. Fresh package
checks verify all receipt/runtime pins and ZIP inventories; there is no fresh
native compilation, emulator run or hardware acceptance.

Historical notes that describe private deliveries or review-only publication
are preserved as evidence, superseded by the user's explicit 7 October public
publication request. Historical attribution may name pre-PIT mapper hashes;
current package manifests control runtime identity. Audio/media, private upload
receipts, proprietary tools/OS/guest files and ongoing Buddy sampled/slideshow
experiments are excluded. Native builds require lawful external period tools.

Original upstream project license is GPLv3; preserve included component MIT
notices and attribution. No underlying media rights are granted by code notices.

XPCHIME is omitted: its Windows XP score reduction has separate rights.
Original stage3 receipts describe eight clients including that omitted app.
