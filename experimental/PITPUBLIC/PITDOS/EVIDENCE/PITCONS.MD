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
| Experimental expressive WININST12 app copies | Done isolated stage4; source review complete | Separate expressive-voice checkout carries the four controls and paired mapper; 10 native runs and200029 logic checks pass; frozen ZIP unchanged |
| Native MML Fiddle / Beats MML export | Done isolated stage5 | Pinned Fiddle02/BEATMML1 source; existing parser extended; opt-in fifth lane, exact old exports; 25 native runs pass; Maker frozen MML3 contract used |
| DOS PSGTEST | Done isolated stage6 | Shared foreground DOS owner, /pit C5 layer, Escape/busy/conflict cleanup; 31 host checks and41 native checks,7 runs; no TSR/system-wide guarantee |
| DOS WAVHYB | Remaining safety prerequisite | Shared early session guard prevents tested Windows-child use. No new musical toggle; sampled IRQ0/PIT0 path remains unqualified and unchanged; no simultaneous PIT music |
| src/MIDIMAP.C legacy stub | N/A active audio / development scaffold | Reports success without generating sound. Use paired src/win30/BEATS driver for active apps; do not pretend stub supports PIT or register replacement globally |
| oemdisplay display modes, splash, launcher, install utilities | N/A musical voice UI | Preserve route/ports and shared SOUND integration; no installed display/driver changes |
| Tandinator sound preview | API ready / N/A extra musical UI | No existing extra musical part. Shared capability is available when content requires it; sibling checkout untouched |
| Buddy Maker converter + WPF preview | Owner integration in progress | Frozen MML3/WZV3/WZM2/WZP1 contract accepted; native authoring packet and DOS ownership API handed off; converter/preview qualification belongs to Maker owner |
| Buddy DOS player | Adapter done; owner integration remains | DOS-Sound-Owner-v1 provides exact cooperative PSG/PIT/PWM resource API; native DOS/Windows-child tests pass; whole-clip sampled runtime and physical/session acceptance remain |
| Buddy Win16 player | Blocked source transfer / owner integration | No WINPLAY in current source. BUDWIN01 Library item resolves, but current Windows materializer lacks os.setxattr; local kit needed. Preserve fullscreen/seek and existing formats |
| Buddy x64 player | Done locally by x64 owner | Parent reports PIT qualified; concurrent pacing checkout untouched. No remote write or hardware claim |

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
No remote publication authorized until integration review. Choir experiment and diagnostic deliveries are
complete; raw audio and PWM remain unchanged during this consumer rollout.

Stage3 verification: all four final /G0 native builds passed; 28 Windows 3
real-mode cases (default/PIT/busy/old-driver/close/focus/Escape for each) passed.
Shared owner mocked QA passes20 checks. Refusals log zero application PSG
writes; all cases end owned=0 and pit=0. PSGPLAY traces retain IRQ8/IRQ1C
vectors and advancing BIOS ticks. TEXIT exit-veto, physical joystick/keyboard,
EX audio summing and system-beep interference are not certified by these runs.

DOS guard follow-up: identical read-only DOS binary reports WINDOWS=0 in plain
DOS and WINDOWS=1 when launched by Windows3 real mode (4680h returns0000).
Actual PSGTEST and WAVHYB Windows children now refuse before hardware paths.
The former native-Windows-app probe was the wrong context, not evidence for
DOS-child refusal. Shared DOSGUARD.H is accepted for the tested context;
original unaccepted diagnostic is retained as historical evidence.
This is a foreground DOS-session guard, not a TSR detector/cross-process lease.
Other Windows modes remain runtime-unverified. DOS PSGTEST optional PIT music
is now integrated through the accepted foreground adapter in isolated stage6. WAVHYB has no extra musical part/toggle; its
existing IRQ0/PIT0 sample implementation remains unqualified and unchanged.
Only conio declarations/MK_FP compatibility and the early guard are added.

Latest independent handoffs:
- DOS ownership: E:/budmaker/DOS-Sound-Owner-v1-20261006. Branch
  ai/dos-sound-owner, commit ad8c62a539d33805ec27226c941e5bb5d312a8e3.
  20036 host checks +52 frozen divisors; 18 native foreground and2 actual
  Windows-child checks. Cooperative one-player scope, not TSR/BIOS exclusion.
- Native authoring: E:/budmaker/PITVoice-Stage5-Authoring-20261006-01.
  authoring-voice branch ai/mml3-authoring, commit
  d73aca98821d4653f2baaaa9799ba9c147f8a499. 25 native runs, original16876
  parser checks,1645 old exporter cases and1113 new exporter cases;10 frozen
  authoring byte comparisons and17 source/movie-bound rejections. Source kits,
  instruments, old43 event fixtures and21 tempo exports preserved.
Remaining consumers stay explicit above; these deliveries do not claim the
entire cross-repository all-app rollout or physical hardware acceptance done.

Stage6 closes DOS PSGTEST and expressive source review. DOS PSGTEST branch
ai/dos-psg-pit builds through unchanged DOSSND v1 and has default/PIT byte
identity, Escape, conflict and actual Windows-child refusal evidence. The
expressive review confirms129 frozen-source/delivery checks and10 accepted
native runs; runtime source remains f580eb2. Tandinator has API availability
and no invented musical control. Maker production integration remains with
its owner; Win16 is source-transfer blocked. x64 PIT is locally qualified
according to the parent owner. WAVHYB sample qualification and physical
EX/custom display/joystick acceptance remain outside completed evidence.
