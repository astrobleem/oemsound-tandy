# Shared PIT consumer inventory and staged integration

Status refers to the isolated ai/pit2-voice source, not published defaults or
installed media. Frozen SOUNDP01/P02 and WININST12 ZIPs are never edited.

| Actual consumer | Status | Scope / next gate |
| --- | --- | --- |
| SOUND TSOUND.DRV | Done stage 1 | Private version-1 lease-checked tone API, channel2 only; default standard ABI unchanged |
| Beats baseline source | Done stage 1/2 | Fifth opt-in lane, paired MIDIMAP route, cleanup; native baseline/PIT/UI/close passed |
| Mini Piano baseline source | Done stage 2 | F3 distinct fourth held pitch, key/refcount cleanup; native default/PIT/old-driver passed |
| JoyMIDI baseline source | Done stage 2 | F3 octave layer, explicit rearm; synthetic input/native default/PIT passed, physical joystick pending |
| MiniMIDI baseline source | Done stage 2 | Explicit channel16 PIT mapping, no automatic file remapping; native baseline/PIT passed |
| TCHIME | Done stage 3 | Shared lease before raw PSG writes; default phrase intact; /pit explicit fixed C4 layer |
| XPCHIME | Done stage 3 | Shared lease, default score intact; /pit fixed-level F3/Ab3/Eb3 bass layer |
| TEXIT | Done stage 3 | Actual lease instead of debug-export heuristic; /pitpreview or /pitgo; no busy/error notice system beep; exit only after clean completion |
| Native PSGPLAY | Done stage 3 | Per-play lease, default phrase; F3 or /pit optional C3 layer; off on stop/focus/Escape/close |
| Automatic Mouth | Done safety scope / no musical UI needed | Existing SOUND lease retained; PIT music excluded while it owns sound; no PWM or voice option added |
| Experimental expressive WININST12 app copies | Remaining coordinated integration | Same named apps have separately frozen expressive sources/engine; stage2 is not an implicit patch to these. Carry shared option/cleanup through expressive owner review and tests |
| Native MML Fiddle / Beats MML export | Remaining contract integration | Parent/owner source and parser required; MML3/P proposal first; do not edit older canonical exports |
| DOS PSGTEST | Remaining | Standalone DOS direct-PSG diagnostic lacks Windows task lease. Need explicit DOS-session owner and Windows-active fail-closed gate before optional tone; no TSR/system-wide guarantee |
| DOS WAVHYB | Remaining safety prerequisite | Existing sigma-delta/PSG sampled path is not low-CPU PIT music. Existing PIT0 assumptions must be audited separately; do not add melodic PIT concurrently or change its default timing here |
| src/MIDIMAP.C legacy stub | N/A active audio / development scaffold | Reports success without generating sound. Use paired src/win30/BEATS driver for active apps; do not pretend stub supports PIT or register replacement globally |
| oemdisplay display modes, splash, launcher, install utilities | N/A musical voice UI | Preserve route/ports and shared SOUND integration; no installed display/driver changes |
| Tandinator sound preview | Remaining sibling coordination | Sibling owns active fixes; share API/cleanup patch after its review, never edit its checkout |
| Buddy Maker converter + WPF preview | Remaining format decision | PITMOVIE.MD proposes MML3/P + WZP1 and mandatory capability gate; no schema writes before coordination |
| Buddy DOS player | Remaining runtime decision | Tone/PWM mutual exclusion, speech overlap validation, no stale restore and mandatory-version refusal |
| Buddy Win16 player | Remaining owner/source integration | Actual shared sound/PWM owner path plus native lifecycle testing |
| Buddy x64 player | Remaining format integration | Concurrent pacing checkout untouched; proposed validated WZP renderer after owner review |

New portable source: SHARED/PITCLNT.H and SNDOWN.H. SNDOWN wraps the existing
OpenSound lease around legacy raw-PSG writes and refuses capability failures.
Acquisition refusal performs no application PSG writes and never falls back to
ports or MessageBeep. Direct PSG byte patterns and current cooperative timing
are preserved for default chimes/demo. All PIT levels remain binary on/off.
Windows beeps and unrelated direct-I/O tasks can bypass this cooperative lease.

Stage2 delivery: E:/budmaker/PITVoice-Stage2-20261006-01/PIT-VOICE-STAGE2.zip.
Source ZIP/patch contains only project source, repository license and contributor
instructions. Private outer package adds original compiled EXEs/DRVs, build/test
receipts, original mixed emulator WAVs, and Library upload receipts. Exclude
owned Windows guest/runtime/backup files, proprietary compiler/linker/libs,
credentials, installed drivers, CF payloads, workflow changes and third-party
song/vocal samples. Private WAVs/receipts are NOT committed/public source assets.
No remote publication authorized until integration review. Choir delivery is
complete and is not revised as part of further consumer rollout.

Stage3 verification: all four final /G0 native builds passed; 28 Windows 3
real-mode cases (default/PIT/busy/old-driver/close/focus/Escape for each) passed.
Shared owner mocked QA passes20 checks. Refusals log zero application PSG
writes; all cases end owned=0 and pit=0. PSGPLAY traces retain IRQ8/IRQ1C
vectors and advancing BIOS ticks. TEXIT exit-veto, physical joystick/keyboard,
EX audio summing and system-beep interference are not certified by these runs.
