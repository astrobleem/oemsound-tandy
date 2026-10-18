#ifndef VGM_PSG_H
#define VGM_PSG_H

/* Bounded VGM 1.00-1.71 SN76489 event reader. C89, no OS headers. */
typedef unsigned char VGM_U8;
typedef unsigned short VGM_U16;
typedef unsigned long VGM_U32;

#define VGM_OK 0
#define VGM_E_ARGUMENT 1
#define VGM_E_READ 2
#define VGM_E_MAGIC 3
#define VGM_E_VERSION 4
#define VGM_E_HEADER 5
#define VGM_E_CHIP 6
#define VGM_E_CLOCK 7
#define VGM_E_FLAGS 8
#define VGM_E_COMMAND 9
#define VGM_E_TRUNCATED 10
#define VGM_E_LOOP 11
#define VGM_E_OVERFLOW 12

#define VGM_EVENT_WRITE 1
#define VGM_EVENT_WAIT 2
#define VGM_EVENT_LOOP 3
#define VGM_EVENT_END 4
#define VGM_STEP_ERROR (-1)
#define VGM_STEP_DONE 0
#define VGM_STEP_EVENT 1

/* read_at must fill exactly count bytes and return nonzero on success. */
typedef int (*VGM_READ_AT)(void *context, VGM_U32 offset,
                           VGM_U8 *destination, VGM_U16 count);

typedef struct VGM_EVENT {
    VGM_U16 kind;
    VGM_U8 value;
    VGM_U8 sn_register;
    VGM_U8 noise_control;
    VGM_U8 sn_latch_byte;
    VGM_U32 samples;
    VGM_U32 pit_cycles;
    VGM_U32 duration_samples;
    VGM_U32 duration_pit_cycles;
    VGM_U32 source_offset;
} VGM_EVENT;

typedef struct VGM_PSG {
    VGM_READ_AT read_at;
    void *context;
    VGM_U32 file_size;
    VGM_U32 stream_end;
    VGM_U32 cursor;
    VGM_U32 total_samples_declared;
    VGM_U32 error_offset;
    VGM_U32 loop_offset;
    VGM_U32 loop_samples_declared;
    VGM_U32 loop_samples_actual;
    VGM_U32 loop_samples_at_start;
    VGM_U32 samples;
    VGM_U32 pit_cycles;
    VGM_U32 pit_remainder;
    VGM_U32 sn_clock_hz;
    VGM_U16 sn_feedback;
    VGM_U8 sn_shift_width;
    VGM_U8 sn_flags;
    VGM_U8 latched_register;
    VGM_U8 loop_seen;
    VGM_U8 loop_pending;
    VGM_U8 ended;
    VGM_U16 error;
} VGM_PSG;

/* Initialize using a seekable callback and the exact uncompressed file size. */
int vgm_psg_open(VGM_PSG *state, VGM_READ_AT read_at, void *context,
                 VGM_U32 file_size);

/* Produce one unchanged PSG write, accumulated wait, loop marker, or end. */
int vgm_psg_next(VGM_PSG *state, VGM_EVENT *event);

/* Map a reason code to a stable short diagnostic string. */
const char *vgm_psg_error_text(VGM_U16 reason);

#endif
