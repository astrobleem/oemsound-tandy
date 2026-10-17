# Source adoption evidence

Source pin: libwintandy `80b1431b9407e7fb14392804d4fdc9b10d9d1bab`.
Status on 17 October 2026: fresh MSC6 compile/link passed; native Windows 3.0
execution and new screenshots are queued behind CF/launcher qualification.
This is source only, with no new binary release or hardware acceptance claim.

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
heap/stack allocation request. New runtime CPU measurements are pending.

Recorded host tests pass: raw format 126, MIDI transport 1493, synthesis
10029, live events 18, beat assertions, and packed-layout/typed-include
fixtures. These are unsanitized host checks. They do not establish segmented
ABI execution, Windows pixels, IRQ scheduling, sound fidelity or hardware
support. The included HOST/PLAIN.JSON and FACADE notes retain their exact
scope; the initial missing-UBSan link failures did not run tests.

Native acceptance must cover playback, contention, pause/reset/restart,
Stop/EOF/close mute and release, bounded queues, UI updates, clean DOS return
and CPU/RAM cost. Only successfully tested redistributable binaries can be
added after that gate. Existing public packages/defaults remain unchanged.
