/* SPDX-License-Identifier: MIT */
/* Fixed-rate IRQ0 update clock; see lt_tick_dos.h. Small or compact model:
 * the private stack is in DGROUP so near pointers to the callback's locals
 * stay valid whatever stack the interrupt arrived on. */
#if !defined(__WATCOMC__) || !defined(__I86__)
#error lt_tick_dos.c requires Open Watcom 16-bit x86
#endif
#include <conio.h>
#include <dos.h>
#include "lt_tick_dos.h"

typedef void (__interrupt __far *lttk_vector)(void);

static lttk_vector previous;
static lttk_work work_fn;
static void *work_ctx;
static unsigned divisor_now;
static volatile unsigned long tick_count;
static unsigned long bios_acc;
static volatile unsigned waiting, in_work, defer_depth;
static volatile unsigned char running;
static unsigned char is_open;
static unsigned backlog_peak;
static unsigned long late_count;
static unsigned saved_ss, saved_sp, private_sp;
static unsigned char private_stack[LTTK_STACK_BYTES];
static void (*drain_ptr)(void);

static unsigned lttk_cli(void)
{
    unsigned f = 0;
    _asm {
        pushf
        pop f
        cli
    }
    return f;
}

static void lttk_sti(unsigned f)
{
    _asm {
        push f
        popf
    }
}

/* On the private stack with IF=1; returns with IF=0 when nothing waits, so
 * a tick arriving after the last check is never left behind. */
static void lttk_drain(void)
{
    for (;;) {
        _disable();
        if (!waiting || defer_depth) return;
        --waiting;
        _enable();
        if (running) work_fn(work_ctx);
    }
}

static void __interrupt __far lttk_isr(void)
{
    ++tick_count;
    bios_acc += divisor_now;
    if (bios_acc >= 65536UL) {
        bios_acc -= 65536UL;
        (*previous)();              /* BIOS tick, INT 1Ch, EOI */
    } else {
        outp(0x20, 0x20);
    }
    ++waiting;
    if (in_work) {                  /* the outer interrupt will run it */
        ++late_count;
        if (waiting > backlog_peak) backlog_peak = waiting;
        return;
    }
    if (defer_depth) return;
    in_work = 1;
    _asm {
        mov saved_ss, ss
        mov saved_sp, sp
        mov ax, ds
        mov ss, ax
        mov sp, private_sp
        cld
        sti
        call word ptr drain_ptr
        cli
        mov ss, saved_ss
        mov sp, saved_sp
    }
    in_work = 0;
}

lttk_status lttk_open(unsigned divisor, lttk_work work, void *context)
{
    unsigned f;
    if (is_open) return LTTK_BUSY;
    if (work == 0 || divisor < 1000U) return LTTK_INVALID;
    work_fn = work;
    work_ctx = context;
    divisor_now = divisor;
    tick_count = 0;
    bios_acc = 0;
    waiting = 0;
    in_work = 0;
    defer_depth = 0;
    running = 0;
    backlog_peak = 0;
    late_count = 0;
    private_sp = (unsigned)(private_stack + LTTK_STACK_BYTES - 2U);
    drain_ptr = lttk_drain;
    f = lttk_cli();
    previous = _dos_getvect(8);
    _dos_setvect(8, lttk_isr);
    outp(0x43, 0x34);               /* channel 0, lo/hi byte, mode 2 */
    outp(0x40, divisor & 0xFFU);
    outp(0x40, divisor >> 8);
    is_open = 1;
    lttk_sti(f);
    return LTTK_OK;
}

lttk_status lttk_close(void)
{
    unsigned f;
    lttk_status st = LTTK_OK;
    if (!is_open) return LTTK_NOT_OPEN;
    f = lttk_cli();
    running = 0;
    outp(0x43, 0x36);               /* channel 0, lo/hi byte, mode 3 */
    outp(0x40, 0);
    outp(0x40, 0);                  /* divisor 65,536 */
    if (_dos_getvect(8) == lttk_isr) _dos_setvect(8, previous);
    else st = LTTK_NOT_OURS;
    is_open = 0;
    lttk_sti(f);
    return st;
}

void lttk_run(int on)
{
    unsigned f = lttk_cli();
    if (on && !running) waiting = 0;
    running = (unsigned char)(on != 0);
    lttk_sti(f);
}

void lttk_defer(int on)
{
    unsigned f = lttk_cli();
    if (on) ++defer_depth;
    else if (defer_depth) --defer_depth;
    lttk_sti(f);
}

unsigned lttk_divisor(void)
{
    return is_open ? divisor_now : 0U;
}

unsigned long lttk_ticks(void)
{
    unsigned f = lttk_cli();
    unsigned long t = tick_count;
    lttk_sti(f);
    return t;
}

unsigned long lttk_now(void)
{
    unsigned f, lo, hi, pending, elapsed;
    unsigned long t;
    if (!is_open) return 0;
    f = lttk_cli();
    outp(0x43, 0x00);               /* latch channel 0 */
    lo = inp(0x40);
    hi = inp(0x40);
    t = tick_count;
    outp(0x20, 0x0A);               /* read the PIC request register */
    pending = inp(0x20) & 1U;
    lttk_sti(f);
    elapsed = divisor_now - (lo | (hi << 8));
    /* A reload not yet serviced starts the next period (small elapsed). */
    if (pending && elapsed < divisor_now / 2U) ++t;
    return t * divisor_now + elapsed;
}

unsigned lttk_backlog_max(void)
{
    return backlog_peak;
}

unsigned long lttk_late(void)
{
    return late_count;
}
