#include "MODPLAY.H"

#define MOD_U32_MAX 0xFFFFFFFFUL

static int effect_supported(MOD_U8 fx, MOD_U8 param);

static void clear_error(MOD_Error *e)
{
    if (e != 0) {
        e->code = MOD_ERR_NONE;
        e->order = 0;
        e->row = 0;
        e->tick = 0;
        e->channel = 0;
        e->effect = 0;
        e->parameter = 0;
        e->reason = "";
    }
}

static int fail(MOD_Error *e, MOD_ErrorCode code, const char *why)
{
    if (e != 0) {
        e->code = code;
        e->reason = why;
    }
    return 0;
}

static MOD_U16 be16(const MOD_U8 *p)
{
    return (MOD_U16)(((MOD_U16)p[0] << 8) | (MOD_U16)p[1]);
}

static int has_signature(const MOD_U8 *h)
{
    if (h[1080] == 'M' && h[1081] == '.' &&
        h[1082] == 'K' && h[1083] == '.') return 1;
    if (h[1080] == '4' && h[1081] == 'C' &&
        h[1082] == 'H' && h[1083] == 'N') return 1;
    return 0;
}

const char *mod_error_name(MOD_ErrorCode code)
{
    switch (code) {
    case MOD_ERR_NONE: return "no error";
    case MOD_ERR_ARGUMENT: return "invalid argument";
    case MOD_ERR_READ: return "source read failed";
    case MOD_ERR_SIGNATURE: return "unsupported MOD signature (expected MK. or 4CHN)";
    case MOD_ERR_HEADER: return "invalid MOD header";
    case MOD_ERR_TRUNCATED: return "truncated MOD data";
    case MOD_ERR_ORDER: return "invalid order or pattern index";
    case MOD_ERR_INSTRUMENT: return "invalid instrument number";
    case MOD_ERR_UNSUPPORTED_EFFECT: return "unsupported ProTracker effect";
    case MOD_ERR_BAD_BREAK: return "invalid pattern break row";
    case MOD_ERR_INTERNAL: return "internal player state error";
    case MOD_ERR_FINETUNE: return "nonzero finetune is unsupported by the placeholder PSG patch";
    case MOD_ERR_TONE_RANGE: return "MOD period is outside the supported PSG tone range";
    case MOD_ERR_VOICE_LIMIT: return "more than three logical MOD channels are audible at once";
    case MOD_ERR_PREFLIGHT_LIMIT: return "song did not terminate within the bounded preflight tick limit";
    default: return "unknown MOD error";
    }
}

int mod_open(MOD_Module *m, MOD_ReadAt rd, void *user,
             MOD_U32 file_bytes, MOD_Error *e)
{
    MOD_U8 h[1084];
    MOD_U8 i;
    MOD_U8 max_pattern;
    MOD_U32 samples;
    MOD_U32 need;
    MOD_U16 words;

    clear_error(e);
    if (m == 0 || rd == 0) return fail(e, MOD_ERR_ARGUMENT, "module and read callback are required");
    if (file_bytes < MOD_HEADER_BYTES) return fail(e, MOD_ERR_TRUNCATED, "file is shorter than the 1084-byte four-channel header");
    if (!rd(user, 0UL, h, (MOD_U16)MOD_HEADER_BYTES))
        return fail(e, MOD_ERR_READ, "could not read MOD header");
    if (!has_signature(h)) return fail(e, MOD_ERR_SIGNATURE, mod_error_name(MOD_ERR_SIGNATURE));
    if (h[950] == 0 || h[950] > MOD_ORDERS)
        return fail(e, MOD_ERR_HEADER, "song length must be 1..128 orders");

    m->read_at = rd;
    m->read_user = user;
    m->file_bytes = file_bytes;
    m->song_length = h[950];
    m->restart_order = h[951];
    max_pattern = 0;
    for (i = 0; i < MOD_ORDERS; ++i) {
        m->order[i] = h[952 + i];
        if (i < m->song_length && m->order[i] > max_pattern)
            max_pattern = m->order[i];
    }
    if (max_pattern > 127) return fail(e, MOD_ERR_ORDER, "pattern index exceeds ProTracker order range");
    m->pattern_count = (MOD_U8)(max_pattern + 1);
    m->pattern_offset = MOD_HEADER_BYTES;
    need = MOD_HEADER_BYTES + (MOD_U32)m->pattern_count * MOD_PATTERN_BYTES;

    samples = 0UL;
    for (i = 0; i < MOD_INSTRUMENTS; ++i) {
        const MOD_U8 *s;
        MOD_U8 j;
        s = h + 20 + (MOD_U16)i * 30;
        for (j = 0; j < 22; ++j) m->instrument[i].name[j] = (char)s[j];
        m->instrument[i].name[22] = '\0';
        words = be16(s + 22);
        m->instrument[i].sample_bytes = (MOD_U32)words * 2UL;
        m->instrument[i].finetune = (MOD_U8)(s[24] & 15U);
        if (s[25] > 64)
            return fail(e, MOD_ERR_HEADER, "instrument volume exceeds the ProTracker 0..64 range");
        m->instrument[i].volume = s[25];
        m->instrument[i].loop_start_bytes = (MOD_U32)be16(s + 26) * 2UL;
        m->instrument[i].loop_length_bytes = (MOD_U32)be16(s + 28) * 2UL;
        m->patch_id[i] = i;
        m->patch_kind[i] = MOD_PATCH_SAMPLE_PLACEHOLDER;
        if (samples > MOD_U32_MAX - (MOD_U32)m->instrument[i].sample_bytes)
            return fail(e, MOD_ERR_HEADER, "sample lengths overflow MOD address arithmetic");
        samples += (MOD_U32)m->instrument[i].sample_bytes;
    }
    if (need > MOD_U32_MAX - samples)
        return fail(e, MOD_ERR_HEADER, "pattern and sample lengths overflow MOD address arithmetic");
    need += samples;
    if (need > file_bytes)
        return fail(e, MOD_ERR_TRUNCATED, "pattern table or declared sample payload exceeds file length");
    return 1;
}

int mod_period_to_psg(MOD_U16 mod_period, MOD_U16 *psg_period)
{
    MOD_U32 extra;
    MOD_U32 divisor;
    if (psg_period == 0 || mod_period == 0 || mod_period > 4095U) return 0;
    /*
     * round(mod_period * 3579545 * 2 / 7093789), decomposed to keep the
     * intermediate within 32 bits on Win16: ratio is 1 + 65301/7093789.
     */
    extra = (MOD_U32)mod_period * 65301UL + 3546894UL;
    extra /= 7093789UL;
    divisor = (MOD_U32)mod_period + extra;
    if (divisor == 0 || divisor > MOD_PSG_TONE_MAX) return 0;
    *psg_period = (MOD_U16)divisor;
    return 1;
}

void mod_player_init(MOD_Player *p, const MOD_Module *m)
{
    MOD_U8 c;
    if (p == 0) return;
    p->module = m;
    p->order = 0;
    p->row = 0;
    p->tick = 0;
    p->speed = 6;
    p->bpm = 125;
    p->stopped = (MOD_U8)(m == 0 ? 1 : 0);
    p->stop_at_song_end = 0;
    p->jump_valid = 0;
    p->jump_order = 0;
    p->break_valid = 0;
    p->break_row = 0;
    p->pattern_loop_valid = 0;
    p->pattern_loop_row = 0;
    for (c = 0; c < MOD_CHANNELS; ++c) {
        p->channel[c].period = 0;
        p->channel[c].base_period = 0;
        p->channel[c].instrument = 0;
        p->channel[c].volume = 0;
        p->channel[c].active = 0;
        p->channel[c].effect = 0;
        p->channel[c].parameter = 0;
        p->channel[c].arp_x = 0;
        p->channel[c].arp_y = 0;
        p->channel[c].loop_row = 0;
        p->channel[c].loop_count = 0;
        p->channel[c].loop_done = 0;
        p->channel[c].loop_mark_valid = 0;
    }
}

static int set_row_error(MOD_Error *e, MOD_ErrorCode code, const char *why,
                         const MOD_Player *p, MOD_U8 ch, MOD_U8 fx, MOD_U8 param)
{
    fail(e, code, why);
    if (e != 0) {
        e->order = p->order;
        e->row = p->row;
        e->tick = p->tick;
        e->channel = ch;
        e->effect = fx;
        e->parameter = param;
    }
    return -1;
}

static int effect_supported(MOD_U8 fx, MOD_U8 param)
{
    if (fx == 0 || fx == 1 || fx == 2 || fx == 10 ||
        fx == 11 || fx == 12 || fx == 13 || fx == 14 || fx == 15) {
        if (fx != 14) return 1;
        if ((param >> 4) == 6 || (param >> 4) == 12) return 1;
    }
    return 0;
}

static int read_row(MOD_Player *p, MOD_Error *e,
                    MOD_U8 *jump_valid, MOD_U8 *jump_order,
                    MOD_U8 *break_valid, MOD_U8 *break_row,
                    MOD_U8 *loop_valid, MOD_U8 *loop_row,
                    MOD_U8 *stop)
{
    MOD_U8 data[16];
    MOD_U8 ch;
    MOD_U32 off;
    const MOD_Module *m;
    m = p->module;
    if (p->order >= m->song_length || p->row >= 64)
        return set_row_error(e, MOD_ERR_INTERNAL, "player position outside MOD order/row bounds", p, 0, 0, 0);
    if (m->order[p->order] >= m->pattern_count)
        return set_row_error(e, MOD_ERR_ORDER, "order points beyond declared pattern table", p, 0, 0, 0);
    off = m->pattern_offset + (MOD_U32)m->order[p->order] * MOD_PATTERN_BYTES + (MOD_U32)p->row * 16UL;
    if (off > m->file_bytes || m->file_bytes - off < 16UL ||
        !m->read_at(m->read_user, off, data, 16))
        return set_row_error(e, MOD_ERR_READ, "could not read current pattern row", p, 0, 0, 0);
    *jump_valid = 0;
    *break_valid = 0;
    *loop_valid = 0;
    *stop = 0;
    for (ch = 0; ch < MOD_CHANNELS; ++ch) {
        MOD_U8 *b;
        MOD_U8 inst;
        MOD_U16 period;
        MOD_U8 fx;
        MOD_U8 param;
        MOD_ChannelState *s;
        b = data + (MOD_U16)ch * 4;
        s = &p->channel[ch];
        period = (MOD_U16)(((MOD_U16)(b[0] & 15U) << 8) | b[1]);
        inst = (MOD_U8)((b[0] & 0xF0U) | (b[2] >> 4));
        fx = (MOD_U8)(b[2] & 15U);
        param = b[3];
        if (!effect_supported(fx, param))
            return set_row_error(e, MOD_ERR_UNSUPPORTED_EFFECT,
                "effect is not implemented by the PSG event producer", p, ch, fx, param);
        if (inst > MOD_INSTRUMENTS)
            return set_row_error(e, MOD_ERR_INSTRUMENT,
                "row instrument number exceeds 31", p, ch, fx, param);
        if (inst != 0) {
            s->instrument = inst;
            s->volume = m->instrument[inst - 1].volume;
        }
        s->effect = fx;
        s->parameter = param;
        s->arp_x = (MOD_U8)(param >> 4);
        s->arp_y = (MOD_U8)(param & 15U);
        if (period != 0) {
            s->period = period;
            s->base_period = period;
            s->active = 1;
        }
        if (fx == 0 && param == 0) {
            s->arp_x = 0;
            s->arp_y = 0;
        } else if (fx == 11) {
            *jump_valid = 1;
            *jump_order = param;
        } else if (fx == 12) {
            s->volume = (MOD_U8)(param > 64 ? 64 : param);
        } else if (fx == 13) {
            MOD_U8 hi;
            MOD_U8 lo;
            hi = (MOD_U8)(param >> 4);
            lo = (MOD_U8)(param & 15U);
            if (hi > 9 || lo > 9 || hi * 10U + lo > 63)
                return set_row_error(e, MOD_ERR_BAD_BREAK,
                    "Dxx row must be packed BCD 00..63", p, ch, fx, param);
            *break_valid = 1;
            *break_row = (MOD_U8)(hi * 10U + lo);
        } else if (fx == 14 && (param >> 4) == 6) {
            MOD_U8 count;
            count = (MOD_U8)(param & 15U);
            if (count == 0) {
                if (!s->loop_mark_valid || s->loop_row != p->row) {
                    s->loop_row = p->row;
                    s->loop_count = 0;
                    s->loop_done = 0;
                    s->loop_mark_valid = 1;
                }
            }
            else {
                if (!s->loop_done) {
                    if (s->loop_count == 0) s->loop_count = count;
                    *loop_valid = 1;
                    *loop_row = s->loop_row;
                    --s->loop_count;
                    if (s->loop_count == 0) s->loop_done = 1;
                }
            }
        } else if (fx == 14 && (param >> 4) == 12) {
            if ((param & 15U) == 0) {
                s->active = 0;
                s->period = 0;
            }
        } else if (fx == 15) {
            if (param == 0) *stop = 1;
            else if (param < 32) p->speed = param;
            else p->bpm = param;
        }
    }
    return 1;
}

static MOD_U16 arp_period(MOD_U16 base, MOD_U8 semitones)
{
    static const MOD_U16 ratio[12] = {
        1000, 944, 891, 841, 794, 749, 707, 667, 630, 595, 561, 530
    };
    MOD_U32 v;
    v = (MOD_U32)base * (MOD_U32)ratio[semitones % 12];
    v /= 1000UL;
    v >>= (semitones / 12);
    if (v == 0) v = 1;
    if (v > 65535UL) v = 65535UL;
    return (MOD_U16)v;
}

static void tick_effects(MOD_Player *p)
{
    MOD_U8 c;
    for (c = 0; c < MOD_CHANNELS; ++c) {
        MOD_ChannelState *s;
        MOD_U8 up;
        MOD_U8 down;
        s = &p->channel[c];
        if (s->effect == 0 && s->parameter != 0) {
            if (p->tick % 3 == 1) s->period = arp_period(s->base_period, s->arp_x);
            else if (p->tick % 3 == 2) s->period = arp_period(s->base_period, s->arp_y);
            else s->period = s->base_period;
        } else if (s->effect == 1 && p->tick != 0) {
            if ((MOD_U32)s->period > (MOD_U32)s->parameter)
                s->period = (MOD_U16)(s->period - s->parameter);
            else s->period = 1;
        } else if (s->effect == 2 && p->tick != 0) {
            if ((MOD_U32)s->period + s->parameter < 4096UL)
                s->period = (MOD_U16)(s->period + s->parameter);
            else s->period = 4095;
        } else if (s->effect == 10 && p->tick != 0) {
            up = (MOD_U8)(s->parameter >> 4);
            down = (MOD_U8)(s->parameter & 15U);
            if (up != 0) s->volume = (MOD_U8)(s->volume + up > 64 ? 64 : s->volume + up);
            else s->volume = (MOD_U8)(s->volume < down ? 0 : s->volume - down);
        } else if (s->effect == 14 && (s->parameter >> 4) == 12 &&
                   p->tick == (s->parameter & 15U)) {
            s->active = 0;
            s->period = 0;
        }
    }
}

static void advance_row(MOD_Player *p)
{
    MOD_U8 i;
    if (p->jump_valid) {
        p->order = p->jump_order;
        p->row = p->break_valid ? p->break_row : 0;
    } else if (p->break_valid) {
        ++p->order;
        p->row = p->break_row;
    } else if (p->pattern_loop_valid) {
        p->row = p->pattern_loop_row;
    } else {
        ++p->row;
        if (p->row >= 64) {
            p->row = 0;
            ++p->order;
        }
    }
    if (p->order >= p->module->song_length) {
        if (!p->stop_at_song_end && p->module->restart_order < p->module->song_length)
            p->order = p->module->restart_order;
        else p->stopped = 1;
    }
    if (p->row >= 64) p->row = 0;
    /* An invalid order entry is reported on the following tick, with context. */
    for (i = 0; i < MOD_CHANNELS; ++i) {
        if (p->channel[i].effect == 0 && p->channel[i].parameter != 0)
            p->channel[i].period = p->channel[i].base_period;
        p->channel[i].effect = 0;
        p->channel[i].parameter = 0;
    }
    p->jump_valid = 0;
    p->break_valid = 0;
    p->pattern_loop_valid = 0;
}

int mod_player_tick(MOD_Player *p, MOD_TickFrame *f, MOD_Error *e)
{
    MOD_U8 jump_valid;
    MOD_U8 jump_order;
    MOD_U8 break_valid;
    MOD_U8 break_row;
    MOD_U8 loop_valid;
    MOD_U8 loop_row;
    MOD_U8 stop;
    MOD_U8 c;
    int row_start;

    clear_error(e);
    if (p == 0 || f == 0 || p->module == 0)
        return fail(e, MOD_ERR_ARGUMENT, "player, module and output frame are required"), -1;
    if (p->stopped) return 0;
    row_start = p->tick == 0;
    jump_valid = 0;
    jump_order = 0;
    break_valid = 0;
    break_row = 0;
    loop_valid = 0;
    loop_row = 0;
    stop = 0;
    if (row_start) {
        if (read_row(p, e, &jump_valid, &jump_order, &break_valid, &break_row,
                     &loop_valid, &loop_row, &stop) < 0) {
            p->stopped = 1;
            return -1;
        }
        p->jump_valid = jump_valid;
        p->jump_order = jump_order;
        p->break_valid = break_valid;
        p->break_row = break_row;
        p->pattern_loop_valid = loop_valid;
        p->pattern_loop_row = loop_row;
        if (stop) {
            p->stopped = 1;
            return 0;
        }
    } else tick_effects(p);

    f->order = p->order;
    f->row = p->row;
    f->tick = p->tick;
    f->row_start = (MOD_U8)(row_start ? 1 : 0);
    f->speed = p->speed;
    f->bpm = p->bpm;
    f->pit_cycles_numerator = MOD_PIT_CYCLE_NUMERATOR;
    f->pit_cycles_denominator = (MOD_U16)(6U * p->bpm);
    f->enabled_mask = 0;
    for (c = 0; c < MOD_CHANNELS; ++c) {
        MOD_ChannelState *s;
        s = &p->channel[c];
        f->period[c] = s->active ? s->period : 0;
        f->instrument[c] = s->instrument;
        f->patch_id[c] = s->instrument == 0 ? 0 : p->module->patch_id[s->instrument - 1];
        f->attenuation[c] = (MOD_U8)(15U - ((MOD_U16)s->volume * 15U / 64U));
        f->effect[c] = s->effect;
        f->parameter[c] = s->parameter;
        /* Volume 1..4 also quantizes to PSG attenuation 15 (silent). */
        if (s->active && f->attenuation[c] < 15U)
            f->enabled_mask |= (MOD_U8)(1U << c);
    }
    ++p->tick;
    if (p->tick >= p->speed) {
        p->tick = 0;
        advance_row(p);
    }
    return 1;
}

static int preflight_error(MOD_Error *e, MOD_ErrorCode code, const char *why,
                           MOD_U8 order, MOD_U8 row, MOD_U8 tick,
                           MOD_U8 channel, MOD_U8 fx, MOD_U8 param)
{
    fail(e, code, why);
    if (e != 0) {
        e->order = order;
        e->row = row;
        e->tick = tick;
        e->channel = channel;
        e->effect = fx;
        e->parameter = param;
    }
    return 0;
}

int mod_preflight(const MOD_Module *m, MOD_U32 max_ticks, MOD_Error *e)
{
    MOD_U8 pattern[1024];
    MOD_U8 used[MOD_INSTRUMENTS];
    MOD_U8 order;
    MOD_U8 row;
    MOD_U8 channel;
    MOD_U32 off;
    MOD_U32 t;
    MOD_Player player;
    MOD_TickFrame frame;
    MOD_U16 psg;
    MOD_U8 voices;
    int result;

    clear_error(e);
    if (m == 0 || m->read_at == 0)
        return fail(e, MOD_ERR_ARGUMENT, "opened module is required for preflight");
    if (max_ticks == 0) max_ticks = MOD_PREFLIGHT_DEFAULT_TICKS;
    if (max_ticks > MOD_PREFLIGHT_DEFAULT_TICKS)
        return fail(e, MOD_ERR_ARGUMENT, "preflight tick limit must be 1..65535");
    for (channel = 0; channel < MOD_INSTRUMENTS; ++channel) used[channel] = 0;

    /* Scan every pattern referenced by the song order before any playback. */
    for (order = 0; order < m->song_length; ++order) {
        MOD_U8 pat;
        pat = m->order[order];
        if (pat >= m->pattern_count)
            return preflight_error(e, MOD_ERR_ORDER,
                "order points beyond declared pattern table", order, 0, 0, 0xFF, 0, 0);
        off = m->pattern_offset + (MOD_U32)pat * MOD_PATTERN_BYTES;
        if (off > m->file_bytes || m->file_bytes - off < MOD_PATTERN_BYTES ||
            !m->read_at(m->read_user, off, pattern, (MOD_U16)MOD_PATTERN_BYTES))
            return preflight_error(e, MOD_ERR_READ,
                "could not read referenced pattern during preflight", order, 0, 0, 0xFF, 0, 0);
        for (row = 0; row < 64; ++row) {
            for (channel = 0; channel < MOD_CHANNELS; ++channel) {
                MOD_U16 pos;
                MOD_U8 inst;
                MOD_U8 fx;
                MOD_U8 param;
                pos = (MOD_U16)((MOD_U16)row * 16U + (MOD_U16)channel * 4U);
                inst = (MOD_U8)((pattern[pos] & 0xF0U) | (pattern[pos + 2] >> 4));
                fx = (MOD_U8)(pattern[pos + 2] & 15U);
                param = pattern[pos + 3];
                if (inst > MOD_INSTRUMENTS)
                    return preflight_error(e, MOD_ERR_INSTRUMENT,
                        "row instrument number exceeds 31", order, row, 0, channel, fx, param);
                if (inst != 0) used[inst - 1] = 1;
                if (!effect_supported(fx, param))
                    return preflight_error(e, MOD_ERR_UNSUPPORTED_EFFECT,
                        "effect is not implemented by the PSG event producer", order, row, 0,
                        channel, fx, param);
                if (fx == 13) {
                    MOD_U8 hi;
                    MOD_U8 lo;
                    hi = (MOD_U8)(param >> 4);
                    lo = (MOD_U8)(param & 15U);
                    if (hi > 9 || lo > 9 || hi * 10U + lo > 63)
                        return preflight_error(e, MOD_ERR_BAD_BREAK,
                            "Dxx row must be packed BCD 00..63", order, row, 0,
                            channel, fx, param);
                }
            }
        }
    }
    for (channel = 0; channel < MOD_INSTRUMENTS; ++channel) {
        if (used[channel] && m->instrument[channel].finetune != 0)
            return preflight_error(e, MOD_ERR_FINETUNE,
                "used instrument finetune is not supported by the selected PSG mapping",
                0, 0, 0, 0xFF, 0, 0);
    }

    /* Stop at song-order end for qualification; normal playback can restart. */
    mod_player_init(&player, m);
    player.stop_at_song_end = 1;
    for (t = 0; t < max_ticks; ++t) {
        result = mod_player_tick(&player, &frame, e);
        if (result == 0) return 1;
        if (result < 0) return 0;
        voices = 0;
        for (channel = 0; channel < MOD_CHANNELS; ++channel) {
            if (frame.enabled_mask & (MOD_U8)(1U << channel)) {
                ++voices;
                if (!mod_period_to_psg(frame.period[channel], &psg))
                    return preflight_error(e, MOD_ERR_TONE_RANGE,
                        "active MOD period maps outside PSG divisor 1..1023",
                        frame.order, frame.row, frame.tick, channel,
                        frame.effect[channel], frame.parameter[channel]);
            }
        }
        if (voices > 3)
            return preflight_error(e, MOD_ERR_VOICE_LIMIT,
                "more than three logical MOD channels are audible at once",
                frame.order, frame.row, frame.tick, 0xFF, 0, 0);
    }
    return preflight_error(e, MOD_ERR_PREFLIGHT_LIMIT,
        "song did not reach F00 or order end within 65535 ticks",
        frame.order, frame.row, frame.tick, 0xFF, 0, 0);
}
