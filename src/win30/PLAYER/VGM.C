#include "VGM.H"

#define VGM_PIT_NUMERATOR 3579545UL
#define VGM_PIT_DENOMINATOR 132300UL

static VGM_U16 vgm_u16(const VGM_U8 *p)
{
    return (VGM_U16)((VGM_U16)p[0] | ((VGM_U16)p[1] << 8));
}

static VGM_U32 vgm_u32(const VGM_U8 *p)
{
    return (VGM_U32)p[0] | ((VGM_U32)p[1] << 8) |
           ((VGM_U32)p[2] << 16) | ((VGM_U32)p[3] << 24);
}

static int vgm_read(VGM_PSG *s, VGM_U32 at, VGM_U8 *p, VGM_U16 n)
{
    if (at > s->file_size || (VGM_U32)n > s->file_size - at)
        return 0;
    return s->read_at(s->context, at, p, n) != 0;
}

static int vgm_bcd_version(VGM_U32 v)
{
    unsigned int a, b, c, d;
    if ((v & 0xffff0000UL) != 0)
        return 0;
    a = (unsigned int)((v >> 12) & 15UL);
    b = (unsigned int)((v >> 8) & 15UL);
    c = (unsigned int)((v >> 4) & 15UL);
    d = (unsigned int)(v & 15UL);
    return (a <= 9U && b <= 9U && c <= 9U && d <= 9U);
}

static int vgm_has_other_chip(const VGM_U8 *h, VGM_U32 header_end)
{
    static const VGM_U8 offsets[] = {
        0x10, 0x2c, 0x30, 0x38, 0x3c, 0x40, 0x44, 0x48, 0x4c, 0x50,
        0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70,
        0x74, 0x80, 0x84, 0x88, 0x8c, 0x90, 0x94, 0x98,
        0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8,
        0xbc, 0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd8, 0xdc,
        0xe0
    };
    unsigned int i;
    for (i = 0; i < (unsigned int)sizeof(offsets); ++i) {
        unsigned int at = offsets[i];
        if ((VGM_U32)at + 4UL <= header_end && (VGM_U32)at + 4UL <= 0x100UL) {
            if (vgm_u32(h + at) != 0UL)
                return 1;
        }
    }
    return 0;
}

static int vgm_fail(VGM_PSG *s, VGM_U16 reason)
{
    s->error = reason;
    return VGM_STEP_ERROR;
}

static int vgm_add_wait(VGM_PSG *s, VGM_EVENT *e, VGM_U32 count,
                        VGM_U32 source_offset)
{
    VGM_U32 whole, rem, extra;
    if (count > 0xffffffffUL - s->samples)
        return vgm_fail(s, VGM_E_OVERFLOW);
    /* 27 + 7445/132300 cycles per sample, exact R4 3579545/3 Hz ratio. */
    whole = count * (VGM_PIT_NUMERATOR / VGM_PIT_DENOMINATOR);
    rem = count * (VGM_PIT_NUMERATOR % VGM_PIT_DENOMINATOR) +
          s->pit_remainder;
    extra = rem / VGM_PIT_DENOMINATOR;
    if (whole > 0xffffffffUL - extra ||
        whole + extra > 0xffffffffUL - s->pit_cycles)
        return vgm_fail(s, VGM_E_OVERFLOW);
    e->kind = VGM_EVENT_WAIT;
    e->duration_samples = count;
    e->duration_pit_cycles = whole + extra;
    e->source_offset = source_offset;
    s->pit_remainder = rem % VGM_PIT_DENOMINATOR;
    s->pit_cycles += e->duration_pit_cycles;
    s->samples += count;
    e->samples = s->samples;
    e->pit_cycles = s->pit_cycles;
    return VGM_STEP_EVENT;
}

int vgm_psg_open(VGM_PSG *s, VGM_READ_AT read_at, void *context,
                 VGM_U32 file_size)
{
    VGM_U8 h[0x100];
    VGM_U32 version, eof, data_rel, data_start, snclock, loop_rel;
    VGM_U16 read_size;
    if (s == 0)
        return VGM_STEP_ERROR;
    {
        VGM_U8 *p = (VGM_U8 *)s;
        unsigned int i;
        for (i = 0; i < (unsigned int)sizeof(*s); ++i)
            p[i] = 0;
    }
    s->read_at = read_at;
    s->context = context;
    s->file_size = file_size;
    if (read_at == 0)
        return vgm_fail(s, VGM_E_ARGUMENT);
    if (file_size < 0x40UL)
        return vgm_fail(s, VGM_E_HEADER);
    {
        unsigned int i;
        for (i = 0; i < (unsigned int)sizeof(h); ++i)
            h[i] = 0;
    }
    read_size = (VGM_U16)(file_size < 0x100UL ? file_size : 0x100UL);
    if (!vgm_read(s, 0UL, h, read_size))
        return vgm_fail(s, VGM_E_READ);
    if (h[0] != 'V' || h[1] != 'g' || h[2] != 'm' || h[3] != ' ')
        return vgm_fail(s, VGM_E_MAGIC);
    eof = vgm_u32(h + 4);
    if (eof > 0xffffffffUL - 4UL || eof + 4UL != file_size)
        return vgm_fail(s, VGM_E_HEADER);
    version = vgm_u32(h + 8);
    if (!vgm_bcd_version(version) || version < 0x100UL ||
        version > 0x171UL)
        return vgm_fail(s, VGM_E_VERSION);
    if (version >= 0x150UL && file_size < 0x40UL)
        return vgm_fail(s, VGM_E_HEADER);
    data_rel = version >= 0x150UL ? vgm_u32(h + 0x34) : 0UL;
    if (data_rel == 0UL)
        data_start = 0x40UL;
    else {
        if (data_rel > 0xffffffffUL - 0x34UL)
            return vgm_fail(s, VGM_E_HEADER);
        data_start = 0x34UL + data_rel;
    }
    if (data_start < 0x40UL || data_start > file_size)
        return vgm_fail(s, VGM_E_HEADER);
    snclock = vgm_u32(h + 0x0c);
    if (snclock == 0UL || (snclock & 0xc0000000UL) != 0UL)
        return vgm_fail(s, VGM_E_CLOCK);
    if (vgm_has_other_chip(h, data_start))
        return vgm_fail(s, VGM_E_CHIP);
    s->sn_clock_hz = snclock;
    s->total_samples_declared = vgm_u32(h + 0x18);
    if (version >= 0x110UL) {
        s->sn_feedback = vgm_u16(h + 0x28);
        s->sn_shift_width = h[0x2a];
    } else {
        s->sn_feedback = 9;
        s->sn_shift_width = 16;
    }
    if (version >= 0x151UL) {
        s->sn_flags = h[0x2b];
        if (s->sn_flags != 0)
            return vgm_fail(s, VGM_E_FLAGS);
    }
    s->cursor = data_start;
    s->stream_end = file_size;
    loop_rel = vgm_u32(h + 0x1c);
    if (loop_rel != 0UL) {
        if (loop_rel > 0xffffffffUL - 0x1cUL)
            return vgm_fail(s, VGM_E_LOOP);
        s->loop_offset = 0x1cUL + loop_rel;
        if (s->loop_offset < data_start || s->loop_offset >= file_size)
            return vgm_fail(s, VGM_E_LOOP);
        s->loop_samples_declared = vgm_u32(h + 0x20);
        s->loop_pending = 1;
    }
    return VGM_STEP_DONE;
}

int vgm_psg_next(VGM_PSG *s, VGM_EVENT *e)
{
    VGM_U8 op, b[2];
    VGM_U32 wait, command_offset;
    unsigned int clear_i;
    if (s == 0 || e == 0 || s->read_at == 0)
        return VGM_STEP_ERROR;
    {
        VGM_U8 *p = (VGM_U8 *)e;
        for (clear_i = 0; clear_i < (unsigned int)sizeof(*e); ++clear_i)
            p[clear_i] = 0;
    }
    if (s->error != VGM_OK)
        return VGM_STEP_ERROR;
    if (s->ended)
        return VGM_STEP_DONE;
    s->error_offset = s->cursor;
    if (s->loop_pending && s->cursor == s->loop_offset) {
        s->loop_pending = 0;
        s->loop_seen = 1;
        s->loop_samples_at_start = s->samples;
        e->kind = VGM_EVENT_LOOP;
        e->source_offset = s->cursor;
        e->samples = s->samples;
        e->pit_cycles = s->pit_cycles;
        return VGM_STEP_EVENT;
    }
    if (s->loop_pending && s->cursor > s->loop_offset)
        return vgm_fail(s, VGM_E_LOOP);
    if (s->cursor >= s->stream_end)
        return vgm_fail(s, VGM_E_TRUNCATED);
    command_offset = s->cursor;
    s->error_offset = command_offset;
    if (!vgm_read(s, s->cursor, &op, 1))
        return vgm_fail(s, VGM_E_READ);
    ++s->cursor;
    if (op == 0x50) {
        if (s->cursor >= s->stream_end || !vgm_read(s, s->cursor, &b[0], 1))
            return vgm_fail(s, VGM_E_TRUNCATED);
        ++s->cursor;
        if ((b[0] & 0x80U) != 0)
            s->latched_register = (VGM_U8)((b[0] >> 4) & 7U);
        e->kind = VGM_EVENT_WRITE;
        e->value = b[0];
        e->sn_register = s->latched_register;
        e->noise_control = (VGM_U8)(s->latched_register == 6U);
        e->sn_latch_byte = (VGM_U8)((b[0] & 0x80U) != 0);
        e->source_offset = command_offset;
        e->samples = s->samples;
        e->pit_cycles = s->pit_cycles;
        return VGM_STEP_EVENT;
    }
    if (op == 0x61) {
        if (s->stream_end - s->cursor < 2UL ||
            !vgm_read(s, s->cursor, b, 2))
            return vgm_fail(s, VGM_E_TRUNCATED);
        s->cursor += 2UL;
        wait = vgm_u16(b);
        return vgm_add_wait(s, e, wait, command_offset);
    }
    if (op == 0x62)
        return vgm_add_wait(s, e, 735UL, command_offset);
    if (op == 0x63)
        return vgm_add_wait(s, e, 882UL, command_offset);
    if ((op & 0xf0U) == 0x70U)
        return vgm_add_wait(s, e, (VGM_U32)(op & 15U) + 1UL,
                            command_offset);
    if (op == 0x66) {
        if (s->loop_pending)
            return vgm_fail(s, VGM_E_LOOP);
        if (s->total_samples_declared != 0UL &&
            s->total_samples_declared != s->samples)
            return vgm_fail(s, VGM_E_HEADER);
        if (s->loop_seen) {
            s->loop_samples_actual = s->samples - s->loop_samples_at_start;
            if (s->loop_samples_declared != 0UL &&
                s->loop_samples_declared != s->loop_samples_actual)
                return vgm_fail(s, VGM_E_LOOP);
        }
        s->ended = 1;
        e->kind = VGM_EVENT_END;
        e->source_offset = command_offset;
        e->samples = s->samples;
        e->pit_cycles = s->pit_cycles;
        return VGM_STEP_EVENT;
    }
    return vgm_fail(s, VGM_E_COMMAND);
}

const char *vgm_psg_error_text(VGM_U16 reason)
{
    static const char *const messages[] = {
        "ok", "invalid argument", "input read failed", "bad VGM magic",
        "unsupported VGM version", "malformed VGM header",
        "unsupported chip present", "unsupported SN clock or variant",
        "unsupported SN flags", "unsupported command", "truncated command",
        "invalid loop boundary or sample count", "sample or PIT timeline overflow"
    };
    if ((unsigned int)reason >= (unsigned int)(sizeof(messages) / sizeof(messages[0])))
        return "unknown VGM error";
    return messages[reason];
}
