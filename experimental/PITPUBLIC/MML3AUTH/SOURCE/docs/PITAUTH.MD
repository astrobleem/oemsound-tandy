# Private MML3 Fiddle and Beats authoring integration

Pinned inputs: MMLFIDL02.zip (213995 bytes, SHA256
c5039c54897ca204affbe102ab67d69f3fc31ef23c336846bd00d83f5e609241)
and BEATMML1.zip (153370 bytes, SHA256
66b0f0802be769398822cd58385d6cd5bb5419473867e7be338b0756bc827c4f).
Their 200/225 manifest entries verified before importing source only. Original
ZIPs and delivered kits remain unchanged. Work is in isolated authoring-voice,
branch ai/mml3-authoring; no installed/CF/workflow/remote/default promotion.

Authoritative interchange: frozen Maker MML3.md revision1, spec SHA256
C47A115B936F67C21665A5C542CF1F0B1684DEEF3809E21C21BBCFEAF2860768.
Use Maker's WZV3/WZM2/WZP1/PIT.REQ bundle contract, not the old proposal.
These native apps author/save MML source; Maker owns movie conversion, required
bundle pairing, binary allocation bounds and the 150ms speech exclusion guard.
The host merge harness is verification code, not a replacement movie compiler.

## Controls and default behavior

Beats retains its four existing PSG/noise lanes and adds a fifth editable PIT
lane. The PIT button defaults off; *PIT marks explicit opt-in. Off exports the
exact old MML2 text, even if hidden PIT data is present. On exports MML3 plus
[P] with binary V1 and the same dotted-sixteenth/rest-thirty-second 75% gate.
Every attack is a separate note/rest pair. An entirely empty fifth lane still
exports explicit [P] with rests; required PIT is never silently omitted. Grid,
MML preview, Copy and safe non-overwriting NAME.MML Save remain available.
Authoring/export works without PIT hardware; playback with *PIT refuses missing
capability. Switch PIT off explicitly to author the legacy four-lane score.

Fiddle's existing parser is extended rather than replaced. Explicit MML3 gains
[P], MIDI45..96, T120 O4 L4 V1 defaults, V0/1 only, no presets/envelopes/bend/ties.
The fifth section retains independent clocks and repeat state, with the shared
32768-token budget, 4096 notes/rests per part, 8192 bytes and 600-second bounds.
MML3 must advance the score. Existing MML1/2 validation/event behavior stays.
Edit the MML3 header and [P] in the text editor; P selects that section and
disables the preset selector. File save preserves the exact authored text.
PIT is off by default. Any explicit P, even empty/all-rest, requires *PIT and
the paired PIT-capable runtime before playback can acquire sound.

Controls fit the original 156x200 window: compact Grid/MML/PIT buttons in Beats;
Open/Save/preset share a row in Fiddle, followed by five section buttons. The
private layouts account for the original fixed-width font rather than extending
the window beyond the small display. Native tests use owned stock-CGA fixtures;
custom display modes still require their own visual/hardware acceptance.

## Ownership and playback

Both apps load their co-located expressive MIDIMAP, retaining INSTR.C/H and
PERIODS.H bytes from the pinned kits. The paired mapper uses the existing SOUND
OpenSound task lease and stage4 TANDYPITVOICE discovery; no port fallback.
Standard capabilities still report three PSG voices. Missing SOUND export is
checked before MODM_OPEN, so unsupported P playback makes no application PSG
writes. Failed/busy acquisitions stop and release; default playback remains
available with the original driver. Co-located mapper is required for review.

P is fixed hardware level, without PSG envelope/preset/vibrato processing.
Fiddle evaluates all due P events before applying one current state. Off/rearm
handles equal repeated notes, and bounded catch-up suppresses incomplete or
expired P attacks rather than replaying a burst. Replay starts fresh. Stop,
Escape, deactivation, close, end-session, capability/busy refusal and mapper
reset/close mute owned output and release the lease. Speech retains its own
exclusive SOUND lease; neither app is authorized to share PIT music with PWM.
Uncooperative system beeps/direct writers can bypass this cooperative API.

## Evidence

Original core suites: 5319 MML1 + 11557 MML2 checks, no failures. Original
43 source fixtures retain byte-identical accepted event dumps and rejection
decisions. Original Beats suite: 1645 valid exports plus overflow/invalid checks;
all 21 frozen tempo exports remain byte-identical. New exporter: 1113 cases
through the existing extended parser, including 52 pitches across 21 tempos,
all-rest explicit P, disabled identity and bounded buffers.

Ten frozen MML3 authoring cases match exact WZM/WZI/WZP bytes through the
independent test merger; all 17 invalid source/movie-bound cases reject. The
ENDATT fixture depends on movie duration, so that rejection belongs to the
bounded merge/converter check, not Fiddle's unbounded standalone source editor.
The full frozen kit's reference check separately verifies 362 files, 112
movie/audio decisions and 52 canonical divisors; no binary movie loader is
claimed for these source-authoring apps.

Native /G0 /AS builds and lifecycle acceptance cover default/export/UI,
three PSG tones + noise + PIT, replay, required-P-disabled/all-rest, old driver,
busy refusal, focus, Escape, close and expired-state stall cleanup. Independent
SPKR/TANDY captures must confirm overlap. Final package pins exact run receipts
and executable/source hashes. Physical EX fidelity, joystick/device input,
human listening and final Maker whole-movie/speech integration remain separate.
