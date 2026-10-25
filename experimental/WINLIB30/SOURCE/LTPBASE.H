/* SPDX-License-Identifier: MIT */
#ifndef LTP_BASE_H
#define LTP_BASE_H
#include <limits.h>
#if UCHAR_MAX != 255 || USHRT_MAX != 65535U
#error libtandy playback requires 8-bit char and 16-bit short
#endif
typedef unsigned char LTP_U8;
typedef unsigned short LTP_U16;
#if ULONG_MAX == 0xffffffffUL
typedef unsigned long LTP_U32;
typedef signed long LTP_I32;
#elif UINT_MAX == 0xffffffffU
typedef unsigned int LTP_U32;
typedef signed int LTP_I32;
#else
#error libtandy playback requires a 32-bit integer type
#endif
#if defined(_MSC_VER) && (defined(_M_I86) || defined(M_I86) || _MSC_VER < 1000)
#define LTP_FAR _far
#elif defined(__WATCOMC__) && defined(__I86__)
#define LTP_FAR __far
#else
#define LTP_FAR
#endif
#define LTP_ENVELOPES 1U
#define LTP_VIBRATO 2U
#endif
