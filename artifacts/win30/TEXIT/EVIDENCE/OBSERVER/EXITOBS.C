#define WINVER 0x0300
#include <windows.h>
static DWORD began;
static int mode, launched, ending, queries, cancelSent;
static void record(char *s) {
 HFILE f=_lopen("C:\\EXITOBS.LOG",1);
 if(f==HFILE_ERROR)f=_lcreat("C:\\EXITOBS.LOG",0);
 if(f!=HFILE_ERROR){_llseek(f,0L,2);_lwrite(f,s,lstrlen(s));_lclose(f);}
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
 DWORD elapsed;HINSTANCE result;
 if(m==WM_CREATE){began=GetTickCount();SetTimer(w,1,55,NULL);record("START observer\r\n");return 0;}
 if(m==WM_QUERYENDSESSION){++queries;record(ending?"QUERY fixture cleanup TRUE\r\n":mode=='v'?"QUERY veto FALSE\r\n":"QUERY application TRUE\r\n");return ending || mode!='v';}
 if(m==WM_ENDSESSION){record(wp?"ENDSESSION TRUE\r\n":"ENDSESSION FALSE\r\n");return 0;}
 if(m==WM_TIMER){
  elapsed=GetTickCount()-began;
  if(!launched && elapsed>=300UL){
   launched=1;record(mode=='p'?"LAUNCH preview\r\n":mode=='b'?"LAUNCH busy\r\n":mode=='v'?"LAUNCH veto\r\n":"LAUNCH allow\r\n");
   result=WinExec((mode=='c'||mode=='k')?"C:\\TEXIT.EXE":mode=='p'?"C:\\TEXIT.EXE /testpreview":"C:\\TEXIT.EXE /testgo",SW_SHOWNORMAL);
   if((unsigned)result<32)record("FAIL launch\r\n");return 0;
  }
  if((mode=='c'||mode=='k') && launched && !cancelSent && elapsed>=700UL) {
   HWND prompt=FindWindow("TandyExitConfirm",NULL);
   if(prompt){if(mode=='k')PostMessage(prompt,WM_KEYDOWN,VK_TAB,0);PostMessage(prompt,WM_KEYDOWN,VK_RETURN,0);cancelSent=1;record(mode=='k'?"SEND Tab Return accept\r\n":"SEND default Return cancel\r\n");}
  }
  if(launched && elapsed>=4000UL && !ending){
   record(GetModuleHandle("TEXIT") || FindWindow("TandyPreExitChime",NULL)?"FAIL resident\r\n":"PASS unloaded\r\n");
   if(mode=='a'||mode=='k')record("FAIL allow did not exit\r\n");
   else if(mode=='v')record(queries?"PASS veto observed\r\n":"FAIL missing veto\r\n");
   else record(queries?"FAIL unexpected exit request\r\n":"PASS no exit request\r\n");
   ending=1;record("FIXTURE cleanup exit\r\n");ExitWindows(0L,0);return 0;
  }
 }
 if(m==WM_DESTROY){KillTimer(w,1);PostQuitMessage(0);return 0;}
 return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show){
 WNDCLASS c;MSG msg;HWND w;mode=*cmd;
 c.style=0;c.lpfnWndProc=WndProc;c.cbClsExtra=0;c.cbWndExtra=0;c.hInstance=h;
 c.hIcon=0;c.hCursor=0;c.hbrBackground=0;c.lpszMenuName=0;
 c.lpszClassName=mode=='b'?"TandyAutomaticMouth":"ExitObserver";
 RegisterClass(&c);w=CreateWindow(c.lpszClassName,"Exit observer",0,0,0,0,0,0,0,h,0);
 while(GetMessage(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
 return 0;
}
