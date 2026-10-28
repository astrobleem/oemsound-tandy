# Provenance: DOS platform services

Branch `claude/dos-platform-services`. Owner approval for MIT relicensing:
Chad Doebelin, 2026-10-24, for the original page and clock code he owns.
That approval does not cover third-party code; none is included (below).

## Sources

All JAWS paths are in Chad's private native JAWS working copy
(`/home/claude/work/jaws/sol`, git). Those files were GPL-3.0-only there.

| libtandy file | Source | Source commit | Authorship | What happened |
| --- | --- | --- | --- | --- |
| `adapters/lt_page_dos.c`, `include/lt_page_dos.h` | `src/jaws_page.c`, `src/jaws_page.h` | `6bb5f04c3692526cf397e346d167c27912dcd3db` (2026-10-18), later edits to `88fddc6` | Written in that project for Chad by Claude (Anthropic) in a Claude Code session; no third-party code | Rewritten from the JAWS module: same method (signature probe for page 4, arena walk, last-fit reservation, predicted vsync), new API (`ltpg_pages`, swap on present, single-page fallback, polling without a clock), new names and comments |
| `adapters/lt_tick_dos.c`, `include/lt_tick_dos.h` | `src/jaws_tick.c`, `src/jaws_tick.h` | `db54ec071c18eb4d393a0c2bbb22ea77bdacd98d` (2026-10-18) | as above | Rewritten from the JAWS module: same method (PIT mode 2, BIOS chaining by a 65,536-cycle accumulator, private stack, in-order catch-up), new API (divisor argument, context pointer, run/defer, statistics), new names and comments |
| `adapters/lt_keys_dos.c`, `include/lt_keys_dos.h`, `src/lt_keys.c`, `include/lt_keys.h` | none | (new) | Written new for libtandy by Claude for Chad | Not derived from JAWS `src/jaws_dos.c` (see below) |
| `adapters/lt_mem_dos.c`, `include/lt_mem_dos.h` | idea from `src/jaws_game_dos.c` (launch figure: own MCB size + largest free block) | `1b191f8` | as above | Written new; only the two-term launch formula is the same |
| `src/lt_gfx_rows.c` | `final_row` in `src/jaws_game_dos.c` (wrapped row copy) | `1b191f8` | as above | Written new as general routines |
| `tools/eatlow.asm` | `tools/eatlow.asm` | `1b191f8` | as above | Copied; header changed from GPL-3.0-only to MIT under the approval |

## The GPL RAVE-derived code, and why it is not here

The JAWS working copy also has `src/jaws_dos.c`, imported in commit
`cf12527a66d9b5b2d74b87b783f9329102154996` from an earlier review package.
Its own header says its fine clock (`jaws_clock`) derives from RAVE's
`CLOCK.H` (SHA-256 `46F8D450...3FE3F6`) and its IRQ1 keyboard handler
follows RAVE's `BEAT.C` (SHA-256 `F657FE78...7E21`), both under GPL-3.0.
That file is not in libtandy and nothing was copied from it:

- The MIT update clock comes from `jaws_tick.c`, a separate module written
  later in the JAWS project (commit `db54ec0`), not from `jaws_clock`.
  `jaws_clock` reads the BIOS day counter, tracks PIT mode-3 half periods
  and midnight; `lt_tick_dos` owns IRQ0 and counts its own mode-2 periods.
- Shared idea, flagged: `lttk_now` (like `jaws_tick_now`) latches PIT
  channel 0 and reads the PIC request register to see whether a reload is
  pending, which is also the core of RAVE's fine-clock technique. That is a
  standard hardware method, not shared code.
- The keyboard handler was written new. It follows the standard PC/XT
  sequence (read port 60h, pulse port 61h bit 7, EOI to port 20h) that
  RAVE's handler also uses; the decoder (separate pressed/released
  bitmaps, E0 fake-shift filtering, BIOS state cleanup on close) differs.
- Line check: against `jaws_dos.c` the only identical non-trivial lines in
  the new modules are `lo = inp(0x40);` and `hi = inp(0x40);` (PIT latch
  reads in `lt_tick_dos.c`) and `#include` lines.

If the RAVE copyright holders consider the pending-IRQ technique or the
keyboard sequence covered, the affected functions are `lttk_now` and
`ltkb_isr`; they would need review rather than a header change.
