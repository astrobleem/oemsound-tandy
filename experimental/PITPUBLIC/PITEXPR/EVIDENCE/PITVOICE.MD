# Isolated expressive PIT voice integration

Frozen source input: WININST12.zip SHA256
`dc88a11e51a4395417861217d53df4015f502635b7e6fdef7e12f96444ea8aee`.
The frozen ZIP is untouched. This separate checkout carries the optional
shared PIT API and UI into its four expressive clients. No movie schema,
MML parser/writer/export or installed media is changed.

Beats has a fifth editable lane and explicit PIT button; default off.
Mini Piano F3 plays one distinct held pitch absent from its three PSG slots.
JoyMIDI F3 provides an octave below its existing note and explicitly rearms.
MiniMIDI's PIT button routes MIDI channel16 only when enabled. No automatic
extra polyphony/remapping. These are musical parts; speech/display utilities
get no invented musical toggle. All original expressive engine service paths,
configuration3 defaults, Program Change state and preset/vibrato semantics
remain in use. INSTR.C/H and PERIODS.H are byte-identical to frozen source.

PIT uses SOUND TANDYPITVOICE version1 under OpenSound. The app-local mapper
implements private message5441h p1=0/1,p2=1 and routes channel15 (human16)
when enabled. Standard reported voice count stays3. PIT has fixed level,
binary on/off, no instrument preset, amplitude envelope or vibrato. Changing
PIT routing resets sounding notes; synth configuration remains selected.
Off/reset/close/WEP, focus, Escape, replay, and busy/capability refusal follow
shared cleanup. MIDIMAP WEP uses its own bounded assembly off, no other DLL
call. Windows beeps/direct-port tasks can bypass the cooperative lease.

Nine native Windows3 real-mode app runs passed baseline/PIT/old-driver tests.
Host driver logic passes200029 checks including original age rollover and
new capability/version/owner/five-generator/stale-off/reset/close cases.
MiniMIDI preserves the original short hat envelope: its held snapshot checks
PSG tones+PIT after noise naturally expires, then replay checks all five at
fresh attack. It does not alter INSTR to keep noise artificially sounding.
Original instrument harness acceptance is recorded separately in evidence.
Physical EX routing/human listening/system-wide ownership remain unverified.

Build through each BUILD.PY using existing licensed C6/MASM/LINK4 tools.
Keep the expressive paired MIDIMAP.DRV next to all four EXEs, distinct from
the PSG-only mapper; use the shared TSOUND only in a disposable owned guest.
Piano/JOY PITCLNT.H is the exact same shared header. Source/patch delivery
contains project source and notices only. Private outer kit adds original
compiled files, native/mocked QA and own mixed captures. No Windows/toolchain,
credentials, third-party samples, installed/CF or CI/remote changes included.
