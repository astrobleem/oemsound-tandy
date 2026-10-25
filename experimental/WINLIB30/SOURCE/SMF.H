/* Bounded Standard MIDI File reader, ANSI C / Microsoft C 6.0.
 * Copyright (c) 2026. Released under the MIT license; see CORE-MIT.TXT.
 */
#ifndef MINIMIDI_SMF_H
#define MINIMIDI_SMF_H

#ifdef SMF_HOST
#define SMF_FAR
typedef unsigned int SMF_ULONG;
#else
#ifndef SMF_FAR
#define SMF_FAR far
#endif
typedef unsigned long SMF_ULONG;
#endif

/* Sanity bound only: the file is streamed, never held in memory. The event
 * budget below is the real work bound. */
#define SMF_MAX_FILE    ((SMF_ULONG)1048576UL)
#define SMF_MAX_TRACKS  16U
#ifndef SMF_MAX_EVENTS
#define SMF_MAX_EVENTS  65000U
#endif
#define SMF_WINDOW      64U
#define SMF_MAX_US ((SMF_ULONG)3600000000UL)

typedef struct SMF_EVENT {
    SMF_ULONG ms;              /* Floor of exact elapsed time, milliseconds. */
    SMF_ULONG tick;            /* Absolute tick; checked for 32-bit overflow. */
    unsigned char status;     /* Full channel status, including channel. */
    unsigned char data1;
    unsigned char data2;      /* Zero for program change / channel pressure. */
    unsigned char length;     /* 2 or 3 bytes, including status. */
    unsigned char track;      /* Zero-based track index. */
} SMF_EVENT;

/* Streaming source: copy n bytes at absolute file offset into dst (a near
 * buffer owned by the parser). Return the byte count read; anything short
 * of n is a read error. Each track keeps one SMF_WINDOW-byte window.
 */
typedef unsigned (*SMF_READ)(SMF_ULONG offset, unsigned char *dst,
                             unsigned n);
int smf_open_stream(SMF_READ read, SMF_ULONG size);

/* The buffer is BORROWED and must remain immutable and alive. On Win16 it
 * must fit in one far segment (<= 65535 bytes). MCITNDY streams instead.
 * Fully validates the complete file and merged timeline before succeeding.
 * Returns 1 on success, 0 on failure; failure leaves playback disabled.
 */
int smf_open(const unsigned char SMF_FAR *file, unsigned size);
/* Memory-backed convenience over smf_open_stream (<= 65535 bytes). */

/* Return 1 for a channel message, 0 for clean EOF, -1 for error.
 * Meta events and bounded F0/F7 SysEx payloads are never emitted.
 * Equal-tick events use lowest track index first, preserving in-track order.
 */
int smf_next(SMF_EVENT *event);
/* At most one pending merged event; 2 means metadata/EOT consumed. */
int smf_step(SMF_EVENT *event);

/* Replay from the beginning without repeating the full validation pass. */
int smf_rewind(void);
void smf_close(void);

/* Error text belongs to the core. "No error" after a successful open. */
const char *smf_error(void);
SMF_ULONG smf_duration_ms(void); /* Includes the last EOT, floor milliseconds. */
unsigned smf_event_count(void); /* Number of channel messages in the file. */
unsigned smf_track_count(void);
unsigned smf_division(void);

#endif
