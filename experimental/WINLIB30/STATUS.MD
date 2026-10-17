# Source adoption evidence

Source pin: libwintandy `80b1431b9407e7fb14392804d4fdc9b10d9d1bab`.
Status on 17 October 2026: fresh MSC6 compile/link and the native Windows 3.0
GUI case passed. WINLIB30.ZIP is a bounded native preview with corresponding
source, not a hardware-qualified replacement or new stable release.

The R4 RAWAPP header-name collision is corrected in the source candidate:
RAWAPP.H carries player declarations and guarded RAWSCORE.H carries the
original raw-utility helpers. See FACADE/ERRATUM.JSON. Historical R4 evidence
applies to the original recorded binaries, not automatically to these builds.

Two fresh `/G0 /Gw /Gs /AS /W3 /Od` MSC6 builds of corrected direct-header
and facade inputs passed. All eleven products matched byte-for-byte across
the pair: MINIPLAY, RAWQA, five raw apps, three provider DLLs and TBRIDGE.
Only the permitted player/provider source subset is included here; utility
apps and their data are outside this directory. The three providers also
match the original R4 binaries after the explicit NE Windows 3.0 version
mark; TBRIDGE matches its original 2922-byte/CRC79c22c55 admission pin.

Fresh MINIPLAY is 22,109 bytes, SHA-256
`00dc26526fd78be1c2f18f713e51390e3f07229697ee4f03d8e7e971dd740654`.
This identifies a local test build, not a downloadable or qualified release.
Byte identity across the include change adds no code, static data, or NE
heap/stack allocation request. The native case passed 58 checks with zero failures. Free memory was exactly
292,992 bytes before and after; original BIOS/INT2F vectors were restored and
DOS return was clean. New calibrated CPU measurements remain pending.

Recorded host tests pass: raw format 126, MIDI transport 1493, synthesis
10029, live events 18, beat assertions, and packed-layout/typed-include
fixtures. These are unsanitized host checks. They do not establish segmented
ABI execution, Windows pixels, IRQ scheduling, sound fidelity or hardware
support. The included HOST/PLAIN.JSON and FACADE notes retain their exact
scope; the initial missing-UBSan link failures did not run tests.

The recorded native GUI case covers MIDI/TND/AGI switching, redraw/restart,
malformed/fast input refusal, Stop/EOF/close sound release and clean DOS return.
It does not add a new listening, physical-hardware, PCjr, Windows 3.1 or
calibrated CPU result. Five program files in the archive are fresh builds;
TPLAY is the original pinned launcher exercised in the case, with source
included. The external paired TSOUND is the exact public PITWIN identity.
Existing public packages/defaults remain unchanged.

WINLIB30.ZIP: 183,144 bytes, 111 entries, SHA-256
`d4d73d2d66439d8f0fdb3463027dbec1acbe37d3725aa65fb08894b9e69b4eeb`.

Evidence reconciliation: the top-level `PROVENANCE.JSON` source pins and its
`native_build` field describe the earlier verified R4/source-only archive.
They are historical inputs, not a statement that no later build exists. The
later preview and paired build are separate evidence below and in
`BUILD/PAIR-EVIDENCE.JSON`. `FACADE/EQUIV.JSON` reports only preprocessing
equivalence; its `native` value is scoped to that check.

`BUILD/PLAYER30.BAT` and `PLAYER30.PY` build six player/provider products.
The separately saved `AUDNEW`/`AUDOLD` paired recipe used the fuller
`audio-adopt/BUILD/BUILD30.BAT` and produced eleven identical products in each
compile/link run: MINIPLAY, RAWQA, QCHIME, QXP, QEXIT, QPSG, QMOUTH, RAWCLK,
MIDICLK, MCITNDY, and TBRIDGE. The paired input sets differed only in
MINIPLAY.C's forwarding include (256 files compared). This count describes
the full paired recipe, not the six-product recipe shipped here. The pair
does not imply all eleven are included in the preview archive; its limited
source slice excludes utility-app source/data.

The preview's CHIME.SND, PLUCK.TND, and STREAM.MID are original synthetic QA
fixtures. They are not commercial or third-party songs and are not a music
catalog. The preview ZIP is now rebuilt with evidence corrections and the GUI fixture
generator. All nine existing runtime files remain byte-identical; the saved
58-check native result is retained without rerunning unchanged runtime.

The 58-check native harness also addressed CHIME.SND, STRESS.TND, TYPE0.MID,
FX.TND, BAD.TND, and FAST.TND. `TESTS/NATIVE/FIXTURE-INPUTS.JSON` records the
first-party generator/source pins, per-file sizes and hashes, and the prepared
guest-image hash. The native result's source-image hash matches the saved
prepared-image record. The five additional test-only files are not bundled in
the ordinary player `RUNTIME`; no private image or extra audio payload is
added by this metadata update.
