#ifndef MODPLAY_H
#define MODPLAY_H

/*
 * Small, foreground-only ProTracker four-channel reader.
 * C89, no allocation, no DOS/Windows dependencies, no retained pattern data.
 */

typedef unsigned char MOD_U8;
typedef unsigned short MOD_U16;
typedef unsigned long MOD_U32;

typedef int (*MOD_ReadAt)(void *user, MOD_U32 offset, MOD_U8 *dst,
                          MOD_U16 count);

#define MOD_CHANNELS 4
#define MOD_INSTRUMENTS 31
#define MOD_ORDERS 128
#define MOD_PATTERN_BYTES 1024UL
#define MOD_HEADER_BYTES 1084UL
#define MOD_PIT_CYCLE_NUMERATOR 17897725UL
#define MOD_PREFLIGHT_DEFAULT_TICKS 65535UL
#define MOD_PSG_TONE_MAX 1023U

typedef enum MOD_ErrorCode {
    MOD_ERR_NONE = 0,
    MOD_ERR_ARGUMENT,
    MOD_ERR_READ,
    MOD_ERR_SIGNATURE,
    MOD_ERR_HEADER,
    MOD_ERR_TRUNCATED,
    MOD_ERR_ORDER,
    MOD_ERR_INSTRUMENT,
    MOD_ERR_UNSUPPORTED_EFFECT,
    MOD_ERR_BAD_BREAK,
    MOD_ERR_INTERNAL,
    MOD_ERR_FINETUNE,
    MOD_ERR_TONE_RANGE,
    MOD_ERR_VOICE_LIMIT,
    MOD_ERR_PREFLIGHT_LIMIT
} MOD_ErrorCode;

typedef struct MOD_Error {
    MOD_ErrorCode code;
    MOD_U8 order;
    MOD_U8 row;
    MOD_U8 tick;
    MOD_U8 channel;
    MOD_U8 effect;
    MOD_U8 parameter;
    const char *reason;
} MOD_Error;

typedef struct MOD_Instrument {
    char name[23];
    MOD_U32 sample_bytes;
    MOD_U8 finetune;
    MOD_U8 volume;
    MOD_U32 loop_start_bytes;
    MOD_U32 loop_length_bytes;
} MOD_Instrument;

#define MOD_PATCH_SAMPLE_PLACEHOLDER 1

typedef struct MOD_Module {
    MOD_ReadAt read_at;
    void *read_user;
    MOD_U32 file_bytes;
    MOD_U32 pattern_offset;
    MOD_U8 song_length;
    MOD_U8 restart_order;
    MOD_U8 pattern_count;
    MOD_U8 order[MOD_ORDERS];
    MOD_Instrument instrument[MOD_INSTRUMENTS];
    /* Explicit mapping: instrument n (1..31) -> patch ID n-1, placeholder. */
    MOD_U8 patch_id[MOD_INSTRUMENTS];
    MOD_U8 patch_kind[MOD_INSTRUMENTS];
} MOD_Module;

typedef struct MOD_ChannelState {
    MOD_U16 period;
    MOD_U16 base_period;
    MOD_U8 instrument;
    MOD_U8 volume;
    MOD_U8 active;
    MOD_U8 effect;
    MOD_U8 parameter;
    MOD_U8 arp_x;
    MOD_U8 arp_y;
    MOD_U8 loop_row;
    MOD_U8 loop_count;
    MOD_U8 loop_done;
    MOD_U8 loop_mark_valid;
} MOD_ChannelState;

typedef struct MOD_Player {
    const MOD_Module *module;
    MOD_ChannelState channel[MOD_CHANNELS];
    MOD_U8 order;
    MOD_U8 row;
    MOD_U8 tick;
    MOD_U8 speed;
    MOD_U8 bpm;
    MOD_U8 stopped;
    MOD_U8 stop_at_song_end;
    MOD_U8 jump_valid;
    MOD_U8 jump_order;
    MOD_U8 break_valid;
    MOD_U8 break_row;
    MOD_U8 pattern_loop_valid;
    MOD_U8 pattern_loop_row;
} MOD_Player;

/*
 * One tick snapshot preserves all four logical MOD channels. Instrument n
 * maps explicitly to patch ID n-1, kind MOD_PATCH_SAMPLE_PLACEHOLDER. The
 * consumer owns tone/noise selection and 4-to-3 tone allocation; no source
 * channel is rewritten or dropped here. Sample waveforms are not reproduced.
 * period[] contains original MOD periods. attenuation[] uses PSG convention,
 * 0 loud and 15 silent. An adapter that sends noise must separately account
 * for the current R4 limitation: a noise frame retriggers noise and cannot
 * represent attenuation-only updates independently.
 *
 * PIT tick duration is exact as numerator / denominator cycles; denominator
 * is 6 * BPM. Tempo changes can change it; the caller owns remainder policy.
 */
typedef struct MOD_TickFrame {
    MOD_U8 order;
    MOD_U8 row;
    MOD_U8 tick;
    MOD_U8 row_start;
    MOD_U8 speed;
    MOD_U8 bpm;
    MOD_U32 pit_cycles_numerator;
    MOD_U16 pit_cycles_denominator;
    MOD_U16 period[4];
    MOD_U8 attenuation[4];
    MOD_U8 instrument[4];
    MOD_U8 patch_id[4];
    MOD_U8 effect[4];
    MOD_U8 parameter[4];
    MOD_U8 enabled_mask;
} MOD_TickFrame;

/* Return 1 on complete read, 0 on failure. read_at may not retain dst. */
int mod_open(MOD_Module *module, MOD_ReadAt read_at, void *user,
             MOD_U32 file_bytes, MOD_Error *error);
void mod_player_init(MOD_Player *player, const MOD_Module *module);

/* Return 1 for a tick snapshot, 0 when stopped, -1 with error otherwise. */
int mod_player_tick(MOD_Player *player, MOD_TickFrame *frame,
                    MOD_Error *error);

/*
 * Validate all order-referenced rows and simulate up to max_ticks before
 * playback ownership. Returns 1 only if F00 or the song's order end is
 * reached; returns 0 with a detailed error on unsupported data, >3 active
 * lanes, invalid PSG divisors, read failures, or the bounded tick limit.
 */
int mod_preflight(const MOD_Module *module, MOD_U32 max_ticks, MOD_Error *error);

/* MOD period -> 10-bit SN/Tandy tone divisor for the selected PAL tuning. */
int mod_period_to_psg(MOD_U16 mod_period, MOD_U16 *psg_period);

/* Human-readable stable error reason, including unsupported command details. */
const char *mod_error_name(MOD_ErrorCode code);

#endif
