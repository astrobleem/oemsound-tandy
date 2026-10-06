/* Disposable native TEXIT interactions; not a desktop application. */
#define WINVER 0x0300
#include <windows.h>
static HFILE file;
static int scenario,stage,queries,owned,allowExit,failures,done;
static DWORD due;
static void logText(char *text){file=_lopen("C:\\EXITQA.LOG",1);if(file==HFILE_ERROR)file=_lcreat("C:\\EXITQA.LOG",0);if(file==HFILE_ERROR)return;_llseek(file,0L,2);_lwrite(file,text,lstrlen(text));_lclose(file);}
static void check(char *text,int okay){char b[110];wsprintf(b,"%s %s\r\n",(LPSTR)(okay?"PASS":"FAIL"),(LPSTR)text);logText(b);if(!okay)failures++;}
static void output(unsigned char value){_asm {mov dx,0c0h
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
 nop}}
static void silence(void){if(owned){output(0x9f);output(0xbf);output(0xdf);output(0xff);CloseSound();owned=0;}}
static void finish(HWND w){char b[60];if(done)return;done=1;silence();wsprintf(b,"TOTAL_FAILURES=%d QUERIES=%d\r\n",failures,queries);logText(b);DestroyWindow(w);}
LONG FAR PASCAL ExitQAProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
 HWND prompt,yes,no,notice;RECT r;char b[40];int result;
 switch(m){
 case WM_CREATE:
  due=GetTickCount()+700L;check("timer starts",SetTimer(w,1,55,NULL)!=0);return 0;
 case WM_TIMER:
  if((LONG)(GetTickCount()-due)<0)return 0;
  if(stage==0){
   if(scenario<=2 || scenario==5 || scenario==8){owned=OpenSound()>=0;check("parent owns sound",owned);if(owned){StopSound();output(0x9f);output(0xbf);output(0xdf);output(0xff);output(0x8c);output(0x1a);output(0x94);}}
   check("launch TEXIT",WinExec(scenario<=2?"C:\\TEXIT.EXE /go":scenario==3||scenario==5?"C:\\TEXIT.EXE /testpreview":(scenario==6||scenario==8)?"C:\\TEXIT.EXE":"C:\\TEXIT.EXE /testgo",SW_SHOW)>=32);
   due=GetTickCount()+1200L;stage=1;return 0;
  }
  if(stage==1 && (scenario<=2 || scenario==6 || scenario==8)){
   prompt=FindWindow("TandyExitConfirm",NULL);check("confirmation appears",prompt!=NULL);
   if(prompt){yes=GetDlgItem(prompt,IDOK);no=GetDlgItem(prompt,IDCANCEL);GetWindowText(yes,b,sizeof(b));check("correct exit choice",!lstrcmp(b,(scenario==6||scenario==8)?"Exit":"Exit silently"));check("Cancel gets initial focus",GetFocus()==no);GetWindowRect(prompt,&r);check("prompt fits screen",r.left>=0&&r.top>=0&&r.right<=GetSystemMetrics(SM_CXSCREEN)&&r.bottom<=GetSystemMetrics(SM_CYSCREEN));}
   due=GetTickCount()+800L;stage=2;return 0;
  }
  if(stage==2){prompt=FindWindow("TandyExitConfirm",NULL);if(prompt)PostMessage(prompt,WM_COMMAND,(scenario==2||scenario==8)?IDOK:IDCANCEL,0L);due=GetTickCount()+1200L;stage=3;return 0;}
  if(stage==3 && scenario==8){prompt=FindWindow("TandyExitConfirm",NULL);check("second busy prompt appears",prompt!=NULL);if(prompt){GetWindowText(GetDlgItem(prompt,IDOK),b,sizeof(b));check("second prompt offers silent exit",!lstrcmp(b,"Exit silently"));check("second Cancel focus",GetFocus()==GetDlgItem(prompt,IDCANCEL));PostMessage(prompt,WM_COMMAND,IDCANCEL,0L);}stage=4;due=GetTickCount()+1200L;return 0;}
  notice=FindWindow(NULL,"Exit Windows");if(notice)PostMessage(notice,WM_COMMAND,IDOK,0L);
  if(scenario==2||scenario==4)check("application can veto",queries>0);else check("no unintended exit request",queries==0);
  check("confirm closed",!FindWindow("TandyExitConfirm",NULL));
  if(owned)check("parent retains own lease",OpenSound()<0);
  else{result=OpenSound();check("TEXIT released sound",result>=0);if(result>=0)CloseSound();}
  finish(w);return 0;
 case WM_QUERYENDSESSION:queries++;check("normal exit query received",1);return allowExit;
 case WM_ENDSESSION:if(wp){check("accepted exit completed",allowExit);finish(w);}return 0;
 case WM_CLOSE:finish(w);return 0;
 case WM_DESTROY:KillTimer(w,1);PostQuitMessage(0);return 0;
 }return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show){WNDCLASS wc;HWND w;MSG msg;
 scenario=1;if(!lstrcmp(cmd,"/silentveto"))scenario=2;if(!lstrcmp(cmd,"/preview"))scenario=3;if(!lstrcmp(cmd,"/goveto"))scenario=4;if(!lstrcmp(cmd,"/busypreview"))scenario=5;if(!lstrcmp(cmd,"/cancel"))scenario=6;if(!lstrcmp(cmd,"/accept")){scenario=7;allowExit=1;}if(!lstrcmp(cmd,"/twoprompt"))scenario=8;
 file=_lcreat("C:\\EXITQA.LOG",0);if(file!=HFILE_ERROR)_lclose(file);
 wc.style=0;wc.lpfnWndProc=ExitQAProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;wc.hCursor=NULL;wc.hbrBackground=NULL;wc.lpszMenuName=NULL;wc.lpszClassName="ExitQA";
 if(!RegisterClass(&wc))return 2;w=CreateWindow("ExitQA","Exit QA",WS_OVERLAPPED|WS_CAPTION,0,0,100,60,NULL,NULL,inst,NULL);check("window created",w!=NULL);if(!w)return 3;ShowWindow(w,SW_SHOW);UpdateWindow(w);
 while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
 allowExit=1;ExitWindows(0L,0);return failures;
}
