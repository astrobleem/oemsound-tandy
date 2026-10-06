/* Isolated native ownership regression; never install in startup. */
#define WINVER 0x0300
#include <windows.h>
static HFILE logFile;
static unsigned stage,failures;
static DWORD due;
static int owned;
static void check(char *s,int ok) {
 char b[100];wsprintf(b,"%s %s\r\n",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)s);
 _lwrite(logFile,b,lstrlen(b));_lclose(logFile);
 logFile=_lopen("C:\\OWNER.LOG",1);_llseek(logFile,0L,2);if(!ok)failures++;
}
static void byteOut(unsigned char value) {
 _asm {mov dx,0c0h
       mov al,value
       out dx,al
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop
       nop}
}
static void mute(void){if(owned){byteOut(0x9f);byteOut(0xbf);byteOut(0xdf);byteOut(0xff);CloseSound();owned=0;}}
static void launch(char *cmd){check(cmd,WinExec(cmd,SW_SHOW)>=32);}
static void probe(void){int n;n=OpenSound();check("SOUND free after child",n>=0);if(n>=0)CloseSound();}
LONG FAR PASCAL OwnerProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 DWORD now;
 switch(m){
 case WM_CREATE:
  logFile=_lcreat("C:\\OWNER.LOG",0);check("real mode",!(GetWinFlags()&WF_PMODE));
  owned=OpenSound()>=0;check("parent owns SOUND",owned);
  if(owned){StopSound();byteOut(0x9f);byteOut(0xbf);byteOut(0xdf);byteOut(0xff);
   byteOut(0x8c);byteOut(0x1a);byteOut(0x94);}
  due=GetTickCount()+500L;check("timer created",SetTimer(w,1,55,NULL)!=0);return 0;
 case WM_TIMER:
  now=GetTickCount();if((LONG)(now-due)<0)return 0;
  if(stage==0){launch("C:\\TCHIME.EXE /test");launch("C:\\XPCHIME.EXE /test");launch("C:\\PSGPLAY.EXE /busytest");due=now+3000L;}
  else if(stage==1){check("blocked children leave parent lease",owned);mute();probe();launch("C:\\TCHIME.EXE /test");due=now+3000L;}
  else if(stage==2){probe();launch("C:\\XPCHIME.EXE /test");due=now+4500L;}
  else if(stage==3){probe();launch("C:\\PSGPLAY.EXE /ownertest");due=now+4500L;}
  else{probe();check("children closed",!FindWindow("TandyPSGTest",NULL)&&!FindWindow("TandyStartupChime",NULL)&&!FindWindow("TandyXPStartupChime",NULL));PostMessage(w,WM_CLOSE,0,0L);}
  stage++;return 0;
 case WM_CLOSE:mute();DestroyWindow(w);return 0;
 case WM_DESTROY:KillTimer(w,1);PostQuitMessage(0);return 0;
 }return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE i,HINSTANCE p,LPSTR c,int show){
 WNDCLASS wc;HWND w;MSG msg;char b[60];
 wc.style=0;wc.lpfnWndProc=OwnerProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=i;wc.hIcon=NULL;wc.hCursor=NULL;wc.hbrBackground=NULL;wc.lpszMenuName=NULL;wc.lpszClassName="SoundOwnerQA";
 if(!RegisterClass(&wc))return 2;w=CreateWindow("SoundOwnerQA","Owner QA",WS_POPUP,0,0,0,0,NULL,NULL,i,NULL);if(!w)return 3;
 while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
 wsprintf(b,"TOTAL_FAILURES=%u\r\n",failures);_lwrite(logFile,b,lstrlen(b));_lclose(logFile);ExitWindows(0L,0);return failures;
}
