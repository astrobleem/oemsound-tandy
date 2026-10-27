# Canonical Windows 3.0 Tandy sound driver

Developer reference. For normal installation, use [the DOS setup guide](../../../docs/INSTALL.MD).

This source builds SOUND.DRV, whose internal Win16 module name is SOUND.
The same driver provides the legacy three PSG queues, Windows alerts, the
optional PIT2 voice and the prepared PIT2 lease. Consumers resolve private
extensions from the already installed SOUND module; they do not load a second
driver or swap it for an application. TSOUND.C is the retained source filename.

## Ownership and PSG arbitration

OpenSound returns three voices and grants one task exclusive ownership. A
second open, including a repeated open by that task, returns -1. CloseSound
from another task has no effect. StopSound flushes notes, cancels alerts,
revokes raw/PIT leases, mutes owned output and removes the exact SYSTEM timer.
CloseSound additionally relinquishes the task owner. WEP performs teardown.

OpenSound initially protects direct PSG clients as well as legacy callers:
a Windows alert cannot assume that empty legacy queues mean idle hardware.
Publishing a legacy queue establishes managed playback. Alerts may then borrow
an idle/resting voice or overlay voice 1 while all queues continue advancing
at the original elapsed time. Each alert phase occupies a full callback
interval. Cancelled, reentrant or busy alerts return 0 without stealing raw
playback. A completed alert returns 1. Calls without a current task or with interrupts masked return 0 before
creating a timer. A missing callback cancels only the alert after 20 independent
BIOS ticks (about 1.1 seconds), or a finite poll budget if that clock is frozen.
The borrowed managed note is restored; the task and PIT ownership remain held.
Only an unchanged idle alert timer is retired. No USER API is needed by this
alert deadline, including during early desktop initialization.

Direct writers explicitly acquire TANDYPSGLEASE(version=1, operation=1) after
OpenSound and their final StopSound. CLAIM is idempotent for that task and
performs no I/O when already held, so it also validates continued ownership.
QUERY=0 returns 0 only while unowned; RELEASE=2 mutes raw PSG and relinquishes
only the raw claim. Invalid version/operation returns -16, wrong task -1,
and busy queues/alerts -3. Legacy enqueue and StartSound return -3 during a
raw claim. StopSound revokes it. PSGLINK.H defines the callable contract.

## PIT2 extensions

TANDYPITVOICE uses the published version-1 (version,hz,level) signature in
PITAPI.H. It requires the OpenSound task, supports 37..7902 whole Hz at level 1,
and accepts hz=0 or level=0 to stop. It preserves EX routing bits and refuses
an already active speaker. Legacy PSG queues may run alongside this voice.

TANDYPITLEASE preserves the prepared API in PITLINK.H: version 0x0100,
QUERY/CLAIM/RELEASE and TPINFO's fixed resident fast entry point. CLAIM and
RELEASE require the actual OpenSound task. QUERY grants no token. The fast
capability checks its nonzero generation token before I/O; it never calls
Windows or DOS. SUSPEND stops owned PIT2 output; REENTER refuses another
speaker owner. Release, StopSound and CloseSound invalidate old tokens.
A raw PSG claim and PIT lease coexist. Neither PIT release nor suspension
releases PSG ownership. The simple PIT voice refuses a prepared PIT lease.

## Legacy sound contract

Ordinals 1..18 and the FAR PASCAL signatures are unchanged. Three fixed queues
hold 32 six-byte records by default, 64 maximum. SetVoiceNote supports rests,
notes 1..84, divisors 1..64 and 0..15 dots. SetVoiceSound accepts whole-Hz 16.16
frequencies and positive 2.5-ms duration units. Unrepresentable pitches return
-13. Accent retains tempo, articulation, pitch and attenuation. Count includes
the active note through note-off. Threshold bits latch crossings and clear
atomically when read. WaitSoundState yields and rejects impossible waits.
Envelope, legacy noise and SyncAllVoices retain their established negative
results (-11, -15 and -16); the integration does not replace those calls with
success stubs. ABI.MD records the underlying legacy calling conventions.

## Build and tests

Run BUILD.PY with explicit --toolchain, --dosbox, --lease and new --out paths.
It preflights the supplied MSC6/MASM/LINK4 files, acquires the shared emulator
lease, checks the process inventory and runs only its own hidden bounded
child. The build uses /G0 /Gw /Gs /AS /W3 /Od and .8086 assembly. Production
SOUND.DRV omits SOUND_TEST hooks; the separate TSOUND.DRV test build contains
PSG traces, deterministic callback stepping and failure/race injection.
BUILD.JSON pins source, external tools and both artifacts. DRVTEST/INTRUD
exercise alerts, callback timing, cross-task refusal and lifecycle behavior.

HOSTTEST.PY compiles the real driver state machine with mocked Win16 and port
calls and checks raw/queued/PIT arbitration, stale tokens, handoff and cleanup.
On Windows it uses the required preflighted gcc-ucrt64.py launcher; missing
toolchain dependencies refuse before compiler startup. Native acceptance is
separate from these mocked checks. TESTS.JSON and BEEP-TESTS.JSON retain their
historical binary scope; current native receipts identify the tested driver
hash separately. Emulator results do not claim physical hardware validation.
