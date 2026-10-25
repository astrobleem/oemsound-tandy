#ifndef MIDISKIP_H
#define MIDISKIP_H
#include "MIDIMSG.H"
/* Ordinary SMF traffic the PSG renderer does not model is dropped by the
   producer so the song still opens: aftertouch, every controller outside
   the MIDIMSG.H list (RPN/NRPN, data entry, filters, ...), bank select
   (the single supported bank stays selected; program changes still apply)
   and drum notes outside 35..81 (no PSG voice). Melodic notes 0..127 are
   admitted: those below 45 keep their key and sound octave-folded
   (MIDICLK.C LOW_NOTE_FOLD). Anything else MSValidMessage refuses still
   refuses the file. */
static LTP_U16 MSSkipMessage(LTP_U32 msg)
{
 LTP_U16 s=(LTP_U16)msg&240;
 if(MSValidMessage(msg,1))return 0;
 if(s==0xa0||s==0xd0||s==0xb0)return 1;
 return s==0x90&&((LTP_U16)msg&15)==9;
}
#endif
