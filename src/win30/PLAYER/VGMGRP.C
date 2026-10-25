#include "VGMGRP.H"

static void vgm_group_clear(VGM_BURST_GROUP *g)
{
    VGM_U8 *p = (VGM_U8 *)g;
    unsigned int i;
    for (i = 0; i < (unsigned int)sizeof(*g); ++i)
        p[i] = 0;
    g->loop_index = VGM_BURST_NO_LOOP_INDEX;
}

static int vgm_group_fail(VGM_BURST_READER *r, VGM_U16 reason,
                          VGM_U32 source_offset)
{
    r->error = reason;
    r->error_offset = source_offset;
    return VGM_BURST_ERROR;
}

static int vgm_group_start(VGM_BURST_READER *r, VGM_U32 at,
                           VGM_U32 source_offset, int has_write)
{
    vgm_group_clear(&r->pending);
    r->pending.at = at;
    r->pending.source_offset = source_offset;
    r->has_pending = 1;
    if (has_write && r->have_last_write) {
        if (at < r->last_write_at || at - r->last_write_at < r->min_spacing)
            return vgm_group_fail(r, VGM_BURST_E_SPACING, source_offset);
    }
    return VGM_BURST_DONE;
}

static int vgm_group_finalize(VGM_BURST_READER *r)
{
    if (!r->has_pending)
        return VGM_BURST_DONE;
    if (r->pending.count != 0) {
        r->last_write_at = r->pending.at;
        r->have_last_write = 1;
    }
    r->group_ready = 1;
    return VGM_BURST_READY;
}

static void vgm_group_accept(VGM_BURST_READER *r)
{
    if (r->has_pending) {
        if (r->pending.count != 0) {
            r->write_count += r->pending.count;
            ++r->burst_count;
        }
    }
    r->has_pending = 0;
    r->group_ready = 0;
    vgm_group_clear(&r->pending);
}

int vgm_burst_open(VGM_BURST_READER *r, VGM_READ_AT read_at,
                   void *context, VGM_U32 file_size, VGM_U32 min_spacing)
{
    VGM_U8 *p;
    unsigned int i;
    if (r == 0)
        return VGM_BURST_ERROR;
    p = (VGM_U8 *)r;
    for (i = 0; i < (unsigned int)sizeof(*r); ++i)
        p[i] = 0;
    vgm_group_clear(&r->pending);
    r->min_spacing = min_spacing ? min_spacing : VGM_BURST_DEFAULT_MIN_SPACING;
    if (vgm_psg_open(&r->source, read_at, context, file_size) != VGM_STEP_DONE) {
        r->error = VGM_BURST_E_SOURCE;
        r->source_error = r->source.error;
        r->error_offset = r->source.error_offset;
        return VGM_BURST_ERROR;
    }
    return VGM_BURST_DONE;
}

int vgm_burst_step(VGM_BURST_READER *r)
{
    VGM_EVENT e;
    int result;
    if (r == 0)
        return VGM_BURST_ERROR;
    if (r->error != VGM_BURST_E_NONE)
        return VGM_BURST_ERROR;
    if (r->group_ready)
        return VGM_BURST_READY;
    if (r->source_done)
        return VGM_BURST_DONE;
    for (;;) {
        if (r->has_lookahead) {
            e = r->lookahead;
            r->has_lookahead = 0;
        } else {
            result = vgm_psg_next(&r->source, &e);
            if (result == VGM_STEP_ERROR) {
                r->source_error = r->source.error;
                return vgm_group_fail(r, VGM_BURST_E_SOURCE,
                                      r->source.error_offset);
            }
            if (result == VGM_STEP_DONE) {
                r->source_done = 1;
                if (r->has_pending)
                    return vgm_group_finalize(r);
                return VGM_BURST_DONE;
            }
        }
        if (e.kind == VGM_EVENT_WRITE) {
            if (!r->has_pending) {
                result = vgm_group_start(r, e.pit_cycles, e.source_offset, 1);
                if (result == VGM_BURST_ERROR)
                    return result;
            } else if (r->pending.at != e.pit_cycles) {
                r->lookahead = e;
                r->has_lookahead = 1;
                return vgm_group_finalize(r);
            }
            if (r->pending.count >= VGM_BURST_MAX_WRITES)
                return vgm_group_fail(r, VGM_BURST_E_TOO_MANY,
                                      e.source_offset);
            r->pending.bytes[r->pending.count] = e.value;
            r->pending.write_offsets[r->pending.count] = e.source_offset;
            ++r->pending.count;
            continue;
        }
        if (e.kind == VGM_EVENT_WAIT) {
            if (r->has_pending && e.duration_pit_cycles != 0UL)
                return vgm_group_finalize(r);
            continue;
        }
        if (e.kind == VGM_EVENT_LOOP) {
            if (r->has_pending && r->pending.at != e.pit_cycles) {
                r->lookahead = e;
                r->has_lookahead = 1;
                return vgm_group_finalize(r);
            }
            if (!r->has_pending) {
                result = vgm_group_start(r, e.pit_cycles, e.source_offset, 0);
                if (result == VGM_BURST_ERROR)
                    return result;
            }
            if (r->pending.loop_boundary)
                return vgm_group_fail(r, VGM_BURST_E_SOURCE, e.source_offset);
            r->pending.loop_boundary = 1;
            r->pending.loop_index = r->pending.count;
            r->pending.loop_source_offset = e.source_offset;
            if (r->pending.count == 0)
                return vgm_group_finalize(r);
            continue;
        }
        if (e.kind == VGM_EVENT_END) {
            r->source_done = 1;
            if (r->has_pending)
                return vgm_group_finalize(r);
            return VGM_BURST_DONE;
        }
        return vgm_group_fail(r, VGM_BURST_E_SOURCE, e.source_offset);
    }
}

int vgm_burst_dispatch(VGM_BURST_READER *r, VGM_BURST_SINK sink,
                       void *context)
{
    int status;
    if (r == 0 || sink == 0)
        return VGM_BURST_ERROR;
    if (r->error != VGM_BURST_E_NONE)
        return VGM_BURST_ERROR;
    if (!r->group_ready || !r->has_pending)
        return VGM_BURST_DONE;
    status = sink(context, &r->pending);
    if (status == VGM_SINK_FULL)
        return VGM_BURST_BLOCKED;
    if (status != VGM_SINK_ACCEPT)
        return vgm_group_fail(r, VGM_BURST_E_SINK,
                              r->pending.source_offset);
    vgm_group_accept(r);
    return VGM_BURST_ACCEPTED;
}

void vgm_burst_report(const VGM_BURST_READER *r,
                      VGM_PREFLIGHT_REPORT *report)
{
    if (r == 0 || report == 0)
        return;
    report->sn_clock_hz = r->source.sn_clock_hz;
    report->total_samples = r->source.samples;
    report->loop_offset = r->source.loop_offset;
    report->loop_samples = r->source.loop_samples_actual;
    report->write_count = r->write_count;
    report->burst_count = r->burst_count;
    report->error_offset = r->error_offset;
    report->sn_feedback = r->source.sn_feedback;
    report->error = r->error;
    report->source_error = r->source_error;
    report->sn_shift_width = r->source.sn_shift_width;
    report->sn_flags = r->source.sn_flags;
}

int vgm_burst_preflight(VGM_READ_AT read_at, void *context,
                        VGM_U32 file_size, VGM_U32 min_spacing,
                        VGM_PREFLIGHT_REPORT *report)
{
    VGM_BURST_READER reader;
    int result;
    if (report == 0)
        return VGM_BURST_ERROR;
    if (vgm_burst_open(&reader, read_at, context, file_size, min_spacing) ==
        VGM_BURST_ERROR) {
        vgm_burst_report(&reader, report);
        return VGM_BURST_ERROR;
    }
    for (;;) {
        result = vgm_burst_step(&reader);
        if (result == VGM_BURST_ERROR) {
            vgm_burst_report(&reader, report);
            return result;
        }
        if (result == VGM_BURST_DONE) {
            vgm_burst_report(&reader, report);
            return VGM_BURST_DONE;
        }
        vgm_group_accept(&reader);
    }
}

const char *vgm_burst_error_text(VGM_U16 reason)
{
    static const char *const messages[] = {
        "ok", "source VGM parser rejected input",
        "distinct write bursts violate minimum spacing",
        "more than 16 writes share one source timestamp",
        "burst sink rejected a ready group"
    };
    if ((unsigned int)reason >= (unsigned int)(sizeof(messages) / sizeof(messages[0])))
        return "unknown burst error";
    return messages[reason];
}
