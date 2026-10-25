/* SPDX-License-Identifier: MIT
   Foreground-only exact integer helpers; no I/O or shared interrupt state. */
#ifndef LTP_MATH_H
#define LTP_MATH_H
#include "LTPBASE.H"
/* denominator must be nonzero. Each DIV quotient fits in one word because
   its high dividend word is strictly below the denominator. */
static LTP_U32 lt_div16(LTP_U32 numerator,LTP_U16 denominator)
{
#if defined(_MSC_VER) && (defined(_M_I86) || defined(M_I86) || _MSC_VER < 1000)
 LTP_U16 partLo,partHi;
 _asm mov ax,word ptr numerator+2
 _asm xor dx,dx
 _asm div word ptr denominator
 _asm mov partHi,ax
 _asm mov ax,word ptr numerator
 _asm div word ptr denominator
 _asm mov partLo,ax
 return ((LTP_U32)partHi<<16)|partLo;
#else
 return numerator/denominator;
#endif
}
/* floor(n/3579545), for every u32 n. 1199=floor(2^32/3579545).
   The reciprocal estimate is at most one low, then corrected exactly.
   The split word products are bounded below 2^27, with no 64-bit type. */
static LTP_U32 lt_clock_div(LTP_U32 n)
{
 LTP_U32 low=(LTP_U32)(LTP_U16)n*1199UL;
 LTP_U32 high=(n>>16)*1199UL+(low>>16);
 LTP_U32 q=high>>16;
 return q+(n-q*3579545UL>=3579545UL);
}
#endif
