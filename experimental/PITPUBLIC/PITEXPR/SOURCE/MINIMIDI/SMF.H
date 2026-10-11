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

#define SMF_MAX_FILE    32768U
#define SMF_MAX_TRACKS  16U
#ifndef SMF_MAX_EVENTS
#define SMF_MAX_EVENTS  16384U
#endif
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

/* The buffer is BORROWED and must remain immutable and alive. On Win16 it
 * must fit in one far segment (GlobalLock of a <=32 KiB allocation is usual).
 * Fully validates the complete file and merged timeline before succeeding.
 * Returns 1 on success, 0 on failure; failure leaves playback disabled.
 */
int smf_open(const unsigned char SMF_FAR *file, unsigned size);

/* Return 1 for a channel message, 0 for clean EOF, -1 for error.
 * Meta events and bounded F0/F7 SysEx payloads are never emitted.
 * Equal-tick events use lowest track index first, preserving in-track order.
 */
int smf_next(SMF_EVENT *event);

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
