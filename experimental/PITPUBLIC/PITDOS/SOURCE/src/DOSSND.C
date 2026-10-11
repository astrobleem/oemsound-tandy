/* One cooperative DOS player owner, not a TSR or interprocess lock.
 * No direct I/O outside this module except the owner's legacy PWM engine.
 * DOS_SOUND_TEST supplies I/O/flags/session hooks for bounded verification.
 */
#include "DOSSND.H"
#ifndef DOS_SOUND_TEST
#include <dos.h>
#include <conio.h>
#include "DOSGUARD.H"
static int SessionPermit(void) { return DosSessionPermit(); }
static unsigned ReadPort(unsigned port) { return inp(port); }
static void WritePort(unsigned port,unsigned value) { outp(port,value); }
static unsigned SaveIRQ(void)
{
    unsigned flags;
    _asm pushf
    _asm pop flags
    _asm cli
    return flags;
}
static void RestoreIRQ(unsigned flags)
{
    _asm push flags
    _asm popf
}
#else
extern int SessionPermit(void);
extern unsigned ReadPort(unsigned);
extern void WritePort(unsigned,unsigned);
extern unsigned SaveIRQ(void);
extern void RestoreIRQ(unsigned);
#endif
unsigned _cdecl PitRead(void) { return ReadPort(0x61); }
void _cdecl PitGate(unsigned bits) { WritePort(0x61,bits); }
void _cdecl PitProgram(unsigned divisor)
{
    WritePort(0x43,0xb6);
    WritePort(0x42,divisor & 255);
    WritePort(0x42,divisor >> 8);
}
#include "win30/DRIVER/PITCORE.H"
static const unsigned pitchHz[52]={
    110,117,123,131,139,147,156,165,175,185,196,208,220,233,
    247,262,277,294,311,330,349,370,392,415,440,466,494,523,
    554,587,622,659,698,740,784,831,880,932,988,1047,1109,
    1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,2093
};
static unsigned soundOwner, currentNote;
static int pitAllowed, speech, psgTouched;
static int Owns(unsigned owner)
{ return owner && owner==soundOwner; }
static void MutePsg(void)
{
    unsigned i;
    for(i=0;i<4;++i)WritePort(0xc0,0x9f+(i<<5));
    psgTouched=0;
}
static int OwnerAcquire(unsigned version,unsigned owner,int pitRequired,
    int pitEnabled,int exclusiveSession)
{
    unsigned flags;
    if(version!=DOS_SOUND_VERSION || !owner ||
       (pitRequired!=0 && pitRequired!=1) ||
       (pitEnabled!=0 && pitEnabled!=1))return DOS_SOUND_CAPABILITY;
    if(pitRequired && !pitEnabled)return DOS_SOUND_CAPABILITY;
    if(exclusiveSession!=1 || !SessionPermit())return DOS_SOUND_CONTEXT;
    flags=SaveIRQ();
    if(soundOwner){RestoreIRQ(flags);return DOS_SOUND_BUSY;}
    /* Refuse an already active speaker before any output write. */
    if(ReadPort(0x61)&3){RestoreIRQ(flags);return DOS_SOUND_BUSY;}
    soundOwner=owner;pitAllowed=pitEnabled;currentNote=0;
    speech=0;psgTouched=0;
    RestoreIRQ(flags);return 0;
}
static int OwnerPsgByte(unsigned owner,unsigned value)
{
    unsigned flags;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(speech)return DOS_SOUND_SPEECH;
    if(value>255)return DOS_SOUND_CAPABILITY;
    flags=SaveIRQ();WritePort(0xc0,value);psgTouched=1;
    RestoreIRQ(flags);return 0;
}
static int OwnerPitOff(unsigned owner)
{
    unsigned flags;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(speech)return DOS_SOUND_SPEECH;
    flags=SaveIRQ();pitStop();currentNote=0;
    RestoreIRQ(flags);return 0;
}
static int OwnerPitNote(unsigned owner,unsigned note,int retrigger)
{
    unsigned flags;int result;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(speech)return DOS_SOUND_SPEECH;
    if(!pitAllowed)return DOS_SOUND_CAPABILITY;
    if(note<45 || note>96)return DOS_SOUND_PITCH;
    if(retrigger!=0 && retrigger!=1)return DOS_SOUND_CAPABILITY;
    if(currentNote==note && !retrigger)return 0;
    flags=SaveIRQ();
    /* Changed pitch and explicit repeated attacks both off/rearm. */
    pitStop();currentNote=0;
    result=pitSet(1,pitchHz[note-45],1);
    if(!result)currentNote=note;
    RestoreIRQ(flags);return result;
}
static int OwnerSpeechBegin(unsigned owner)
{
    unsigned flags;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(speech)return DOS_SOUND_SPEECH;
    flags=SaveIRQ();pitStop();currentNote=0;
    if(psgTouched)MutePsg();
    speech=1;
    RestoreIRQ(flags);return 0;
}
#ifdef DOS_SOUND_TEST
static int OwnerSpeechGate(unsigned owner,unsigned lowBits)
{
    unsigned flags;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(!speech)return DOS_SOUND_SPEECH;
    if(lowBits>3)return DOS_SOUND_CAPABILITY;
    flags=SaveIRQ();
    WritePort(0x61,(ReadPort(0x61)&0xfc)|lowBits);
    RestoreIRQ(flags);return 0;
}
#endif
static int OwnerSpeechEnd(unsigned owner)
{
    unsigned flags;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    if(!speech)return DOS_SOUND_SPEECH;
    flags=SaveIRQ();
    /* Also covers an owner's legacy PWM writes outside SpeechGate. */
    WritePort(0x61,ReadPort(0x61)&0xfc);
    speech=0;currentNote=0;
    RestoreIRQ(flags);return 0;
}
static int OwnerRelease(unsigned owner)
{
    unsigned flags;
    if(!soundOwner)return 0;
    if(!Owns(owner))return DOS_SOUND_OWNER;
    flags=SaveIRQ();pitStop();currentNote=0;
    if(speech)WritePort(0x61,ReadPort(0x61)&0xfc);
    if(psgTouched)MutePsg();
    soundOwner=0;pitAllowed=0;speech=0;
    RestoreIRQ(flags);return 0;
}

static unsigned resourcesHeld;
int _cdecl DosSoundAcquire(unsigned resources)
{
    int result;
    if(!resources || (resources & ~(DS_PSG|DS_PIT|DS_PWM)))
        return DOS_SOUND_CAPABILITY;
    result=OwnerAcquire(1,1,!!(resources & DS_PIT),
        !!(resources & DS_PIT),1);
    if(!result)resourcesHeld=resources;
    return result;
}
int _cdecl DosSoundPsgByte(unsigned value)
{
    if(!soundOwner)return DOS_SOUND_OWNER;
    if(!(resourcesHeld & DS_PSG))return DOS_SOUND_CAPABILITY;
    return OwnerPsgByte(1,value);
}
int _cdecl DosSoundPitNote(unsigned note,unsigned attack)
{
    if(!soundOwner)return DOS_SOUND_OWNER;
    if(!(resourcesHeld & DS_PIT))return DOS_SOUND_CAPABILITY;
    if(attack>1 || (!note && attack))return DOS_SOUND_CAPABILITY;
    if(!note)return OwnerPitOff(1);
    return OwnerPitNote(1,note,attack);
}
int _cdecl DosSoundPwmBegin(void)
{
    if(!soundOwner)return DOS_SOUND_OWNER;
    if(!(resourcesHeld & DS_PWM))return DOS_SOUND_CAPABILITY;
    return OwnerSpeechBegin(1);
}
void _cdecl DosSoundPwmEnd(void) { if(soundOwner)OwnerSpeechEnd(1); }
void _cdecl DosSoundRelease(void)
{
    if(soundOwner)OwnerRelease(1);
    resourcesHeld=0;
}
