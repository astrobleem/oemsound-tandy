#define WINVER 0x0300
#include <windows.h>
static HFILE log;
static int failures;
static int (FAR PASCAL *dt)(void);
static int (FAR PASCAL *dh)(void);
static void result(char *name,int value,int want) {
 char s[96]; wsprintf(s,"%s got=%d want=%d\r\n",(LPSTR)name,value,want);
 _lwrite(log,s,lstrlen(s)); if(value!=want) ++failures;
}
static void waitms(unsigned ms) {
 DWORD t=GetTickCount(); while(GetTickCount()-t < ms) Yield();
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show) {
 HMODULE m; HFILE f; int i,t; DWORD start;
 log=_lcreat("C:\\DRVTEST.LOG",0);
 m=GetModuleHandle("SOUND");
 dt=(int (FAR PASCAL *)(void))GetProcAddress(m,"DEBUGTICKS");
 dh=(int (FAR PASCAL *)(void))GetProcAddress(m,"DEBUGTIMER");
 if(!dt || !dh) { result("candidate exports",0,1); goto end; }
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
end:
 result("TOTAL_FAILURES",failures,0); _lclose(log);
 ExitWindows(0,0); return 0;
}
