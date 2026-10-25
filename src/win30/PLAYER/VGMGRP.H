#ifndef VGM_GROUP_H
#define VGM_GROUP_H

#include "VGM.H"

#define VGM_BURST_MAX_WRITES 16
#define VGM_BURST_DEFAULT_MIN_SPACING 9545UL
#define VGM_BURST_NO_LOOP_INDEX 0xffffU

#define VGM_BURST_ERROR (-1)
#define VGM_BURST_DONE 0
#define VGM_BURST_READY 1
#define VGM_BURST_ACCEPTED 2
#define VGM_BURST_BLOCKED 3

#define VGM_SINK_ERROR (-1)
#define VGM_SINK_FULL 0
#define VGM_SINK_ACCEPT 1

#define VGM_BURST_E_NONE 0
#define VGM_BURST_E_SOURCE 1
#define VGM_BURST_E_SPACING 2
#define VGM_BURST_E_TOO_MANY 3
#define VGM_BURST_E_SINK 4

typedef struct VGM_BURST_GROUP {
    VGM_U32 at;
    VGM_U32 source_offset;
    VGM_U32 loop_source_offset;
    VGM_U16 count;
    VGM_U16 loop_index;
    VGM_U8 loop_boundary;
    VGM_U8 bytes[VGM_BURST_MAX_WRITES];
    VGM_U32 write_offsets[VGM_BURST_MAX_WRITES];
} VGM_BURST_GROUP;

typedef struct VGM_BURST_READER {
    VGM_PSG source;
    VGM_BURST_GROUP pending;
    VGM_EVENT lookahead;
    VGM_U32 min_spacing;
    VGM_U32 last_write_at;
    VGM_U32 error_offset;
    VGM_U32 write_count;
    VGM_U32 burst_count;
    VGM_U16 error;
    VGM_U16 source_error;
    VGM_U8 have_last_write;
    VGM_U8 has_pending;
    VGM_U8 group_ready;
    VGM_U8 has_lookahead;
    VGM_U8 source_done;
} VGM_BURST_READER;

typedef int (*VGM_BURST_SINK)(void *context,
                              const VGM_BURST_GROUP *group);

typedef struct VGM_PREFLIGHT_REPORT {
    VGM_U32 sn_clock_hz;
    VGM_U32 total_samples;
    VGM_U32 loop_offset;
    VGM_U32 loop_samples;
    VGM_U32 write_count;
    VGM_U32 burst_count;
    VGM_U32 error_offset;
    VGM_U16 sn_feedback;
    VGM_U16 error;
    VGM_U16 source_error;
    VGM_U8 sn_shift_width;
    VGM_U8 sn_flags;
} VGM_PREFLIGHT_REPORT;

/* A zero min_spacing selects VGM_BURST_DEFAULT_MIN_SPACING (9545 PIT cycles). */
int vgm_burst_open(VGM_BURST_READER *reader, VGM_READ_AT read_at,
                   void *context, VGM_U32 file_size, VGM_U32 min_spacing);

/* Incrementally assemble the next burst or loop-only marker group. */
int vgm_burst_step(VGM_BURST_READER *reader);

/* The pending group remains byte-for-byte unchanged when the sink returns FULL. */
int vgm_burst_dispatch(VGM_BURST_READER *reader, VGM_BURST_SINK sink,
                       void *context);

/* Read-only metadata for source clock, loop, and a preflight error. */
void vgm_burst_report(const VGM_BURST_READER *reader,
                      VGM_PREFLIGHT_REPORT *report);

/* Scan the whole stream without an external sink. Reopen the same source for
 * later playback; run this before acquiring output ownership. */
int vgm_burst_preflight(VGM_READ_AT read_at, void *context,
                        VGM_U32 file_size, VGM_U32 min_spacing,
                        VGM_PREFLIGHT_REPORT *report);

const char *vgm_burst_error_text(VGM_U16 reason);

#endif
