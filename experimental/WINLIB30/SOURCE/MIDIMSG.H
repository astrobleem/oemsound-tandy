#ifndef MIDI_MESSAGE_POLICY_H
#define MIDI_MESSAGE_POLICY_H
#include "LTPBASE.H"
/* Foreground admission policy, shared by SMF preflight and timestamp producer.
   LTP_U16/LTP_U32 are supplied by the caller. No timer, ownership or OS dependency. */
static LTP_U16 MSValidMessage(LTP_U32 msg,LTP_U16 low)
{
 LTP_U16 s=(LTP_U16)msg&240,ch=(LTP_U16)msg&15;
 LTP_U16 a=((LTP_U16)msg>>8)&255,b=(LTP_U16)(msg>>16)&255;
 if((msg&0xff000000UL)||a>127||b>127)return 0;
 if(s==0x80||s==0xc0)return 1;
 if(s==0xe0)return 1;
 if(s==0x90)return !b||(ch==9?(a>=35&&a<=81):(a<=127&&(a>=45||low)));
 if(s!=0xb0)return 0;
 /* One local bank only. Never silently substitute a nonzero bank. */
 if(a==0||a==32)return b==0;
 return a==1||a==7||a==10||a==11||a==64||a==91||a==93||a==120||a==121||a==123;
}
#endif
