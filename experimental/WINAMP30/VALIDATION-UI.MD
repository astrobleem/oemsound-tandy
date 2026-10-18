# Playlist UI validation

The MSC6 `/G0` build passes. Its production MINIPLAY.EXE and LWFILE.DLL are byte-identical to the candidate used for the focused browser check and independent audio regression.

| Check | Result |
| --- | --- |
| Portable C89 playlist model | 149 checks, zero failures |
| Native browser, gestures, Cancel, Add, duplicate, missing file, caption and no-selection Remove | 130 checks, zero failures |
| Native eight-entry playlist, keyboard navigation, narrow scrolling and restored selection | 162 checks, zero failures |
| Native visible-modal playback, active/ordinary removal, source-file preservation, Stop and close | 104 checks, zero failures |
| Independent gzip, PSG events, refusal, EOF and ownership regression | 88 checks, zero failures |

Native runs use disposable Win3.0 real-mode emulator images configured for an 8088 at 4.772728 MHz, 640KB and stock CGA. Each run has its own bounded lease, clean DOS return, released sound/bridge ownership and unchanged seed image. These results do not constitute physical acceptance of the new UI.

During a 2197ms interval with the browser visibly open, foreground service calls advanced from 7 to 38, PSG byte count from 252 to 300, and the displayed clock advanced with error zero. Removing the active row then left playback running and its file intact.

The independent regression retains PSG hash 54622, 131 writes and three intentional noise writes. Its free-space readings were 247520 bytes before and after. The eight file/format/parser source units and pinned audio clock/MCI/bridge dependencies remain unchanged.

The app grows by 4032 bytes on disk and 4512 bytes in its NE minimum data allocation; its heap and stack reservations remain unchanged. The playlist model occupies 4102 bytes, including a 4096-byte path array. Browser-case `GetFreeSpace(0)` samples were 244224 cold, 232800 with the dialog open, 244096 after first Cancel, and 243520 after all browser cases. These are Win16 free-space observations, not calibrated peak resident memory or CPU measurements.

The QA driver uses a checked 200ms NULL-window timer and returns to GetMessage between finite phases. Native dialog actions require visibility, and completion waits for the API return marker after window destruction. The production browser implementation was unchanged throughout qualification.

## Physical acceptance, 2026-10-18

The user ran the hash-verified UX3 CF copy on the real Tandy 1000 EX after being asked to try Browse/Add, Remove and scrolling past four songs, then reported "that worked great publish it". This is a qualitative user acceptance and explicit publication approval. It is not a measured physical timing/audio/memory result, an exhaustive 32-slot or format sweep, or qualification of simultaneous playback with the pending combined display driver. Runtime bytes are pinned in RUNTIME.JSON; the scoped record is EVIDENCE/PHYSICAL.JSON.
