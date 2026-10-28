/* SPDX-License-Identifier: MIT */
#ifndef LT_TICK_DOS_H
#define LT_TICK_DOS_H

/* Fixed-rate update clock for one foreground DOS program (Open Watcom,
 * 16-bit): the only IRQ0 (PIT channel 0) handler a game installs.
 *
 * Rate: PIT channel 0 runs in mode 2 with the given divisor, so updates come
 * at 1,193,182 / divisor Hz exactly (LTTK_DIVISOR_60HZ: 60.0001 Hz;
 * LTTK_DIVISOR_NES: 60.0988 Hz, the NES NTSC frame rate).
 *
 * BIOS time: the handler adds the divisor to an accumulator and calls the
 * previous IRQ0 handler each time 65,536 PIT cycles have gone by, so the
 * BIOS day counter, INT 1Ch hooks and the floppy motor timeout keep their
 * normal average rate of 18.2065 Hz (each BIOS tick is late by less than one
 * update period). Between those calls the handler sends its own EOI.
 * lt_time_dos.h (ltd_*) keeps working while the clock is open.
 *
 * Work: the callback runs once per tick, inside the interrupt, on a private
 * 3 KiB stack in DGROUP with interrupts enabled. If a tick arrives while the
 * callback is still running it is counted and run right afterwards by the
 * outer interrupt, in order, so updates are never lost or reordered, only
 * delayed. Put game updates (input sampling, logic, lt_sfx_tick/lt_score
 * service) in the callback and draw in the foreground as fast as it goes:
 * logic speed is then independent of drawing speed.
 *
 * Audio: libtandy's audio modules install no timer; service them from the
 * callback. lt_audio/lt_sfx calls must not run concurrently, so either make
 * every audio call from the callback or bracket foreground audio calls
 * with lttk_defer(1)/lttk_defer(0).
 *
 * Close restores PIT mode 3 with divisor 65,536 (the BIOS setting) and the
 * previous vector if ours is still installed. */
#define LTTK_PIT_HZ 1193182UL
#define LTTK_DIVISOR_60HZ 19886U
#define LTTK_DIVISOR_NES 19854U
#define LTTK_STACK_BYTES 3072U

typedef void (*lttk_work)(void *context);

typedef enum {
    LTTK_OK = 0,
    LTTK_INVALID = 1,       /* no callback, or divisor below 1000 */
    LTTK_BUSY = 2,          /* already open */
    LTTK_NOT_OPEN = 3,
    LTTK_NOT_OURS = 4       /* vector replaced by another program; PIT
                               restored, vector left alone */
} lttk_status;

/* Install with work stopped (ticks are counted from open). */
lttk_status lttk_open(unsigned divisor, lttk_work work, void *context);
lttk_status lttk_close(void);
/* Start (1) or stop (0) calling work. Starting drops ticks counted while
 * stopped, so the first update comes at the next tick. */
void lttk_run(int on);
/* Hold (1) or release (0) work around foreground code that shares state
 * with the callback. Ticks keep counting; held ones run at the next tick
 * after release. Nests. */
void lttk_defer(int on);

unsigned lttk_divisor(void);
/* Ticks since open (wraps at 2^32). */
unsigned long lttk_ticks(void);
/* PIT input cycles (1/1,193,182 s) since open, monotonic: ticks * divisor
 * plus the elapsed part of the current period. 0 when closed. */
unsigned long lttk_now(void);
/* Most ticks that waited for a running callback, and how many ran late. */
unsigned lttk_backlog_max(void);
unsigned long lttk_late(void);

#endif
