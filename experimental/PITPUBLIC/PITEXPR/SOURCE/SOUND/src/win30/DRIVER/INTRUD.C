#define WINVER 0x0300
#include <windows.h>
static HFILE log;
static int failures;
static void result(char *name,int value,int want) {
 char s[96]; wsprintf(s,"%s got=%d want=%d\r\n",(LPSTR)name,value,want);
 _lwrite(log,s,lstrlen(s)); if(value!=want) ++failures;
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show) {
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
