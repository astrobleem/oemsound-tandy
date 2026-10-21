#define WINVER 0x0300
#include <windows.h>
#include <string.h>
typedef int (FAR PASCAL *DBEEP)(unsigned);
typedef unsigned (FAR PASCAL *DBG0)(void);
typedef unsigned (FAR PASCAL *DBG1)(int);
typedef unsigned long (FAR PASCAL *DBGEL)(int);
typedef void (FAR PASCAL *DBGV)(void);
typedef int (FAR PASCAL *WEPFN)(int);
static HFILE log;
static int failures;
static unsigned (FAR PASCAL *dt)(void);
static int (FAR PASCAL *dh)(void);
static DBEEP dbeep;
static DBG0 dpc, dactive, drejects;
static DBG1 dpr;
static DBGV dpclear, dfail;
static DBGEL delapsed;
static DBG0 dwaiting;
static DBGV dhold, dholdactive, drelease;
static WEPFN dwep;
static unsigned char beepPattern[7] = {
 0x8d,0x07,0x94,0x85,0x09,0x94,0x9f
};
static unsigned char musicBeepPattern[10] = {
 0x8d,0x07,0x94,0x85,0x09,0x94,0x9f,0x8e,0x0f,0x94
};
static void result(char *name,int value,int want) {
 char s[96]; wsprintf(s,"%s got=%d want=%d\r\n",(LPSTR)name,value,want);
 _lwrite(log,s,lstrlen(s)); if(value!=want) ++failures;
}
static int traceis(unsigned char *want,unsigned count) {
 unsigned i;
 if(dpc()!=count) return 0;
 for(i=0;i<count;++i) if(dpr((int)i)!=want[i]) return 0;
 return 1;
}
static int tailmutes(void) {
 unsigned n;
 n=dpc();
 if(n<4) return 0;
 return dpr((int)n-4)==0x9f && dpr((int)n-3)==0xbf &&
        dpr((int)n-2)==0xdf && dpr((int)n-1)==0xff;
}
static int totalnotes(void) {
 return CountVoiceNotes(1)+CountVoiceNotes(2)+CountVoiceNotes(3);
}
static int notequeuesaligned(void) {
 return CountVoiceNotes(1)==CountVoiceNotes(2) &&
        CountVoiceNotes(1)==CountVoiceNotes(3);
}
static int elapsedaligned(void) {
 return delapsed(1)==delapsed(2) && delapsed(1)==delapsed(3);
}
static int waittrace(unsigned count,unsigned ms) {
 DWORD start;
 start=GetTickCount();
 while(dpc()<count && GetTickCount()-start<ms) Yield();
 return dpc()>=count;
}
static int loghas(char *path,char *needle) {
 HFILE f; int n; char b[160];
 f=_lopen(path,OF_READ); if(f==HFILE_ERROR) return 0;
 n=_lread(f,b,159); if(n<0) n=0; b[n]=0; _lclose(f);
 return strstr(b,needle)!=0;
}
static int waitlog(char *path,char *needle) {
 DWORD start;
 start=GetTickCount();
 while(GetTickCount()-start<3000UL) {
  if(loghas(path,needle)) return 1;
  Yield();
 }
 return loghas(path,needle);
}
static void waitms(unsigned ms) {
 DWORD t=GetTickCount(); while(GetTickCount()-t < ms) Yield();
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show) {
 HMODULE m; HFILE f; int i; unsigned t,rejects;
 DWORD start,elapsed;
 log=_lcreat("C:\\DRVTEST.LOG",0);
 m=GetModuleHandle("SOUND");
 dt=(unsigned (FAR PASCAL *)(void))GetProcAddress(m,"DEBUGTICKS");
 dh=(int (FAR PASCAL *)(void))GetProcAddress(m,"DEBUGTIMER");
 dbeep=(DBEEP)GetProcAddress(m,"DOBEEP");
 dpc=(DBG0)GetProcAddress(m,"DEBUGPSGCOUNT");
 dpr=(DBG1)GetProcAddress(m,"DEBUGPSGREAD");
 dpclear=(DBGV)GetProcAddress(m,"DEBUGPSGCLEAR");
 dactive=(DBG0)GetProcAddress(m,"DEBUGBEEPACTIVE");
 drejects=(DBG0)GetProcAddress(m,"DEBUGBEEPREJECTS");
 dfail=(DBGV)GetProcAddress(m,"DEBUGFAILNEXTTIMER");
 delapsed=(DBGEL)GetProcAddress(m,"DEBUGVOICEELAPSED");
 dwaiting=(DBG0)GetProcAddress(m,"DEBUGBEEPRETURNWAITING");
 dhold=(DBGV)GetProcAddress(m,"DEBUGHOLDBEEPRETURN");
 dholdactive=(DBGV)GetProcAddress(m,"DEBUGHOLDBEEPACTIVE");
 drelease=(DBGV)GetProcAddress(m,"DEBUGRELEASEBEEPRETURN");
 dwep=(WEPFN)GetProcAddress(m,"WEP");
 if(!dt || !dh || !dbeep || !dpc || !dpr || !dpclear || !dactive ||
    !drejects || !dfail || !delapsed || !dwaiting || !dhold ||
    !dholdactive ||
    !drelease || !dwep) {
  result("candidate exports",0,1); goto end;
 }
 start=GetTickCount(); dpclear(); MessageBeep(MB_OK);
 elapsed=GetTickCount()-start;
 result("NoOpenBeepDurationMs",elapsed>=50UL && elapsed<=250UL,1);
 result("NoOpenBeepWrites",dpc(),7);
 result("NoOpenBeepPattern",traceis(beepPattern,7),1);
 result("NoOpenBeepEnded",dactive(),0);
 result("NoOpenTimerRemoved",dh(),0);
 dpclear(); result("DirectDoBeepSuccess",dbeep(0),1);
 result("DirectDoBeepPattern",traceis(beepPattern,7),1);
 result("DirectDoBeepTimerRemoved",dh(),0);
 dpclear(); MessageBeep(MB_ICONEXCLAMATION);
 result("TypedBeepSamePattern",traceis(beepPattern,7),1);
 result("RepeatedBeepTimerRemoved",dh(),0);
 rejects=drejects();
 result("ReentryLaunch",WinExec("C:\\INTRUD.EXE /BEEPRACE",
        SW_SHOWNORMAL)>31,1);
 dpclear(); MessageBeep(MB_OK);
 result("ReentryRejected",drejects()-rejects,1);
 result("ReentryPattern",traceis(beepPattern,7),1);
 result("ReentryLog",waitlog("C:\\BEEPRACE.LOG",
        "BeepCall got=0 want=0"),1);
 result("ReentryTimerRemoved",dh(),0);
 rejects=drejects(); dhold();
 result("CompletionRaceLaunch",WinExec(
        "C:\\INTRUD.EXE /BEEPCOMP",SW_SHOWNORMAL)>31,1);
 dpclear(); MessageBeep(MB_OK);
 result("CompletionRacePattern",traceis(beepPattern,7),1);
 result("CompletionRaceRejected",waitlog("C:\\BEEPCOMP.LOG",
        "CompletionSecondBeepRejected got=0 want=0"),1);
 result("CompletionRaceRejectionCount",drejects()-rejects,1);
 result("CompletionWaiterReleased",dwaiting(),0);
 result("CompletionRaceTimerRemoved",dh(),0);
 dholdactive(); dpclear();
 result("OpenBeepLaunch",WinExec("C:\\INTRUD.EXE /OPENBEEP",
        SW_SHOWNORMAL)>31,1);
 MessageBeep(MB_OK);
 result("OpenBeepCanceled",dactive(),0);
 result("OpenBeepReentryLog",waitlog("C:\\OPENBEEP.LOG",
        "OpenReentryRejected got=0 want=0"),1);
 result("OpenBeepTimerRemoved",dh(),0);
 dpclear(); dfail();
 result("TimerFailureReturn",dbeep(0),0);
 result("TimerFailureWrites",dpc(),0);
 result("TimerFailureInactive",dactive(),0);
 result("TimerFailureNoHandle",dh(),0);
 result("Open",OpenSound(),3); result("Exclusive",OpenSound(),-1);
 result("LowHz",SetVoiceSound(1,65UL<<16,40),-13);
 result("Fraction",SetVoiceSound(1,(440UL<<16)|1,40),-13);
 result("BadNote",SetVoiceNote(1,85,4,0),-5);
 result("BadLength",SetVoiceNote(1,37,0,0),-6);
 result("BadDots",SetVoiceNote(1,37,4,16),-7);
 result("Queue2",SetVoiceQueueSize(1,12),0);
 result("Note1",SetVoiceNote(1,37,4,0),0);
 result("Rest",SetVoiceNote(1,0,4,0),0);
 result("Full",SetVoiceNote(1,37,4,0),-4);
 result("ResizeOccupied",SetVoiceQueueSize(1,192),-3);
 result("Count2",CountVoiceNotes(1),2);
 result("Flush",StopSound(),0); result("Count0",CountVoiceNotes(1),0);
 result("Resize32",SetVoiceQueueSize(1,192),0);
 result("Threshold",SetVoiceThreshold(1,1),0);
 result("C4",SetVoiceNote(1,25,4,0),0);
 result("E4",SetVoiceNote(2,29,4,0),0);
 result("G4",SetVoiceNote(3,32,4,0),0);
 t=dt(); result("Start",StartSound(),0); result("StartAgain",StartSound(),0);
 start=GetTickCount();
 while(CountVoiceNotes(1) && GetTickCount()-start<3000UL) Yield();
 result("QueueFinished",CountVoiceNotes(1),0);
 result("CallbackRan",dt()!=t,1);
 result("ThresholdEvent",GetThresholdStatus(),1);
 result("ThresholdClear",GetThresholdStatus(),0);
 result("Stop",StopSound(),0); result("TimerRemoved",dh(),0);
 t=dt(); waitms(350); result("NoLateCallback",dt(),t);
 result("EnvelopeUnsupported",SetVoiceEnvelope(1,0,0),-11);
 result("NoiseUnsupported",SetSoundNoise(0,40),-15);
 result("SyncUnsupported",SyncAllVoices(),-16);
 CloseSound();
 for(i=0;i<12;++i) {
  result("Reopen",OpenSound(),3);
  result("Tone",SetVoiceSound(1,440UL<<16,400),0);
  result("Restart",StartSound(),0); waitms(70);
  CloseSound(); result("ClosedTimer",dh(),0);
 }
 result("OwnerOpen",OpenSound(),3);
 result("OwnerLongTone",SetVoiceSound(1,440UL<<16,2000),0);
 result("OwnerStart",StartSound(),0); waitms(100);
 result("LaunchIntruder",WinExec("C:\\INTRUD.EXE",SW_SHOWNORMAL)>31,1);
 waitms(700);
 f=_lopen("C:\\INTRUD.LOG",OF_READ);
 result("IntruderRan",f!=HFILE_ERROR,1); if(f!=HFILE_ERROR) _lclose(f);
 result("OwnerQueueSurvives",CountVoiceNotes(1),1);
 result("OwnerTimerSurvives",dh()!=0,1);
 result("ActiveStop",StopSound(),0);
 result("ActiveStopEmpty",CountVoiceNotes(1),0);
 result("ActiveStopTimerGone",dh(),0);
 t=dt(); waitms(700); result("ActiveStopNoLateCallback",dt(),t);
 CloseSound();
 result("FinalOpen",OpenSound(),3);
 result("WaitRest",SetVoiceNote(1,0,16,0),0);
 result("WaitStart",StartSound(),0);
 result("WaitQueue",WaitSoundState(S_QUEUEEMPTY),0);
 CloseSound();
 result("AlertMusicOpen",OpenSound(),3);
 result("AlertMusic1",SetVoiceSound(1,440UL<<16,400),0);
 result("AlertMusic2",SetVoiceSound(2,660UL<<16,400),0);
 result("AlertMusic3",SetVoiceSound(3,880UL<<16,400),0);
 result("AlertMusicStart",StartSound(),0); waitms(70);
 dpclear(); MessageBeep(MB_OK);
 result("MusicBeepWrites",dpc(),10);
 result("MusicBeepPatternAndRestore",
        traceis(musicBeepPattern,10),1);
 result("MusicQueuesPreserved",totalnotes(),3);
 result("MusicTimerShared",dh()!=0,1);
 result("MusicAfterBeepCompletes",
        WaitSoundState(S_QUEUEEMPTY),0);
 result("MusicQueuesComplete",totalnotes(),0);
 result("MusicStop",StopSound(),0);
 result("MusicTimerRemoved",dh(),0);
 CloseSound();
 result("DriftOpen",OpenSound(),3);
 for(i=0;i<8;++i) {
  if(SetVoiceSound(1,440UL<<16,120) ||
     SetVoiceSound(2,660UL<<16,120) ||
     SetVoiceSound(3,880UL<<16,120)) ++failures;
 }
 result("DriftQueuesSetup",notequeuesaligned(),1);
 result("DriftStart",StartSound(),0); waitms(60);
 result("DriftInitialElapsed",elapsedaligned(),1);
 for(i=0;i<4;++i) {
  MessageBeep(MB_OK);
  result("RepeatedBeepQueuesAligned",notequeuesaligned(),1);
  result("RepeatedBeepElapsedAligned",elapsedaligned(),1);
 }
 result("DriftQueuesComplete",WaitSoundState(S_QUEUEEMPTY),0);
 result("DriftAllQueuesEmpty",totalnotes(),0);
 result("DriftStop",StopSound(),0); CloseSound();
 result("StopCaseOpen",OpenSound(),3);
 result("StopCaseQueue",SetVoiceSound(1,440UL<<16,1200),0);
 result("StopCaseStart",StartSound(),0); waitms(70);
 dpclear();
 result("StopCaseLaunch",WinExec("C:\\INTRUD.EXE /BEEPCALL",
        SW_SHOWNORMAL)>31,1);
 result("StopCaseToneStarted",waittrace(3,1000),1);
 result("StopCaseAlertActive",dactive(),1);
 result("StopDuringBeep",StopSound(),0);
 result("StopBeepCanceled",dactive(),0);
 result("StopBeepQueueEmpty",CountVoiceNotes(1),0);
 result("StopBeepTimerRemoved",dh(),0);
 result("StopBeepMuted",tailmutes(),1);
 result("StopBeepCallerReturned",waitlog("C:\\BEEPCALL.LOG",
        "BeepCall got=0 want=0"),1);
 CloseSound();
 result("CloseCaseOpen",OpenSound(),3);
 result("CloseCaseQueue",SetVoiceSound(1,440UL<<16,1200),0);
 result("CloseCaseStart",StartSound(),0); waitms(70);
 dpclear();
 result("CloseCaseLaunch",WinExec("C:\\INTRUD.EXE /BEEPCALL",
        SW_SHOWNORMAL)>31,1);
 result("CloseCaseToneStarted",waittrace(3,1000),1);
 result("CloseCaseAlertActive",dactive(),1);
 CloseSound();
 result("CloseBeepCanceled",dactive(),0);
 result("CloseBeepTimerRemoved",dh(),0);
 result("CloseBeepMuted",tailmutes(),1);
 result("CloseBeepCallerReturned",waitlog("C:\\BEEPCALL.LOG",
        "BeepCall got=0 want=0"),1);
 dpclear();
 result("ShutdownCaseLaunch",WinExec("C:\\INTRUD.EXE /BEEPCALL",
        SW_SHOWNORMAL)>31,1);
 result("ShutdownCaseToneStarted",waittrace(3,1000),1);
 result("ShutdownAlertActive",dactive(),1);
 t=dt(); result("ShutdownWEP",dwep(0),1);
 result("ShutdownBeepCanceled",dactive(),0);
 result("ShutdownTimerRemoved",dh(),0);
 waitms(350); result("ShutdownNoLateCallback",dt(),t);
 result("ShutdownBeepCallerReturned",waitlog("C:\\BEEPCALL.LOG",
        "BeepCall got=0 want=0"),1);
end:
 result("TOTAL_FAILURES",failures,0); _lclose(log);
 ExitWindows(0,0); return 0;
}
