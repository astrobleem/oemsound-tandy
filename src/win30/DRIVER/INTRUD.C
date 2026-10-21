#define WINVER 0x0300
#include <windows.h>
typedef int (FAR PASCAL *DBEEP)(unsigned);
typedef unsigned (FAR PASCAL *DBG0)(void);
typedef void (FAR PASCAL *DBGV)(void);
static HFILE log;
static int failures;
static void result(char *name,int value,int want) {
 char s[96]; wsprintf(s,"%s got=%d want=%d\r\n",(LPSTR)name,value,want);
 _lwrite(log,s,lstrlen(s)); if(value!=want) ++failures;
}
static int beepchild(char *path,int waitactive) {
 HMODULE m; DBEEP db; DBG0 active,rejects; DWORD start;
 log=_lcreat(path,0); if(log==HFILE_ERROR) return 1;
 m=GetModuleHandle("SOUND");
 db=(DBEEP)GetProcAddress(m,"DOBEEP");
 active=(DBG0)GetProcAddress(m,"DEBUGBEEPACTIVE");
 rejects=(DBG0)GetProcAddress(m,"DEBUGBEEPREJECTS");
 if(!db || !active || !rejects) {
  result("BeepExports",0,1); result("TOTAL_FAILURES",failures,0);
  _lclose(log); return 1;
 }
 if(waitactive) {
  start=GetTickCount();
  while(!active() && GetTickCount()-start<3000UL) Yield();
  result("BeepObserved",active(),1);
 }
 result("BeepCall",db(0),0);
 if(waitactive) result("BeepRejected",rejects(),1);
 result("TOTAL_FAILURES",failures,0); _lclose(log); return 0;
}
static int completionchild(void) {
 HMODULE m; DBEEP db; DBG0 pending,active,rejects; DBGV release;
 DWORD start; unsigned before;
 log=_lcreat("C:\\BEEPCOMP.LOG",0);
 if(log==HFILE_ERROR) return 1;
 m=GetModuleHandle("SOUND");
 db=(DBEEP)GetProcAddress(m,"DOBEEP");
 pending=(DBG0)GetProcAddress(m,"DEBUGBEEPRETURNWAITING");
 active=(DBG0)GetProcAddress(m,"DEBUGBEEPACTIVE");
 rejects=(DBG0)GetProcAddress(m,"DEBUGBEEPREJECTS");
 release=(DBGV)GetProcAddress(m,"DEBUGRELEASEBEEPRETURN");
 if(!db || !pending || !active || !rejects || !release) {
  result("CompletionExports",0,1); goto end;
 }
 start=GetTickCount();
 while(!pending() && GetTickCount()-start<3000UL) Yield();
 result("CompletionHeld",pending(),1);
 result("CompletionToneEnded",active(),0);
 before=rejects();
 result("CompletionSecondBeepRejected",db(0),0);
 result("CompletionRejectCount",rejects()-before,1);
 release();
end:
 result("TOTAL_FAILURES",failures,0); _lclose(log); return 0;
}
static int openbeepchild(void) {
 HMODULE m; DBEEP db; DBG0 active,rejects,count; DWORD start; unsigned before;
 log=_lcreat("C:\\OPENBEEP.LOG",0);
 if(log==HFILE_ERROR) return 1;
 m=GetModuleHandle("SOUND");
 db=(DBEEP)GetProcAddress(m,"DOBEEP");
 active=(DBG0)GetProcAddress(m,"DEBUGBEEPACTIVE");
 rejects=(DBG0)GetProcAddress(m,"DEBUGBEEPREJECTS");
 count=(DBG0)GetProcAddress(m,"DEBUGPSGCOUNT");
 if(!db || !active || !rejects || !count) {
  result("OpenBeepExports",0,1); goto end;
 }
 start=GetTickCount();
 while(count()<3 && GetTickCount()-start<3000UL) Yield();
 result("OpenToneStarted",count()>=3,1);
 result("OpenSawBeep",active(),1);
 result("OpenDuringBeep",OpenSound(),3);
 before=rejects();
 result("OpenReentryRejected",db(0),0);
 result("OpenReentryRejectCount",rejects()-before,1);
 CloseSound();
end:
 result("TOTAL_FAILURES",failures,0); _lclose(log); return 0;
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show) {
 if(!lstrcmpi(cmd,"/BEEPRACE"))
  return beepchild("C:\\BEEPRACE.LOG",1);
 if(!lstrcmpi(cmd,"/BEEPCALL"))
  return beepchild("C:\\BEEPCALL.LOG",0);
 if(!lstrcmpi(cmd,"/BEEPCOMP")) return completionchild();
 if(!lstrcmpi(cmd,"/OPENBEEP")) return openbeepchild();
 log=_lcreat("C:\\INTRUD.LOG",0);
 result("NonownerOpen",OpenSound(),-1);
 result("NonownerStart",StartSound(),-1);
 result("NonownerStop",StopSound(),-1);
 result("NonownerNote",SetVoiceNote(1,25,4,0),-1);
 result("NonownerSound",SetVoiceSound(1,440UL<<16,80),-1);
 result("NonownerResize",SetVoiceQueueSize(1,0),-1);
 result("NonownerAccent",SetVoiceAccent(1,120,0,0,0),-1);
 result("NonownerThreshold",SetVoiceThreshold(1,5),-1);
 result("NonownerEvent",GetThresholdEvent()==0,1);
 result("NonownerCount",CountVoiceNotes(1),0);
 CloseSound();
 result("TOTAL_FAILURES",failures,0); _lclose(log); return 0;
}
