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

WINLIB30.ZIP: 138,101 bytes, 96 entries, SHA-256
`12d2e62616975d6bc7d4c41e48492100c14947752fc8ad95faa770ea32350870`.
