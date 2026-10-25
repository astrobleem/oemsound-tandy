/* Actual foreground live adapter + actual MIDICLK; fake raw hardware only. */
#define main inherited_md_host_main
#include "mdhost/driver.c"
#undef main
#define HINSTANCE HMODULE
#define lstrcmp strcmp
#include "../SOURCE/MIDIAPI.H"
#include "../SOURCE/LIVEMIDI.H"
#ifndef LIVE_MAIN
#define LIVE_MAIN main
#endif
int LIVE_MAIN(void){WORD oldvol;unsigned i;acBound=1;acOpenFn=MDOpen;acPutFn=MDPush;acCommitFn=MDCommit;acStartFn=MDStart;acPumpFn=MDPump;acCloseFn=MDClose;acResetFn=MDReset;acStatusFn=MDStatus;acTimingFn=MDTiming;acClockFn=MDClock;acVoiceFn=MDVoiceMode;acDebugFn=MDDebug;acBankFn=MDInstrumentBank;liBankSeedFn=MDSeedBank;liSeedFn=MDSeed;liBeginFn=MDBeginLive;liFlags=3;
 liFlags=4;ck(LIOpen()!=0&&!lease&&!acHandle,"unsupported flags release lease");liFlags=3;ck(!LIOpen(),"live open");ck(!LIMessage(MODM_DATA,0x7f3c90),"first key");ck(nframes>10&&liveFrame.volume%16==0,"prepared expressive frames");
 advance(330);ck(!LIPump(),"foreground refill");oldvol=liSynth.volume[0];ck(oldvol>0,"held key has aged");
 i=flushes;ck(!LIApply(0x7f4090),"second key state");ck(!LIRebuild(),"second key generation");ck(flushes==i&&lease&&running,"input does not mute/flush/detach existing owner");ck((liveFrame.volume&15)==oldvol,"held note keeps aged envelope");ck(((liveFrame.volume>>4)&15)==0,"new key attacks independently");
 advance(900000);ck(!LIPump(),"long held age");ck(!LIApply(0x7f4390)&&!LIRebuild(),"long age rebuild");ck((liveFrame.volume&15)==8,"long held key does not restart attack");
 task=2;ck(MDBeginLive(acHandle,0)==MD_BUSY,"foreign live staging refused");ck(LIApply(0x3c80)!=0,"foreign update refused");task=1;
 ck(!LIReset()&&LIDebug(0)==0,"panic clear");ck(!LIClose()&&!lease,"close releases hardware");printf("PASS actual live adapter/source checks=%u\n",tests);return 0;}
