/* Disposable original-fixture Buddy status regression. */
#define WINVER 0x0300
#include <windows.h>
static int stage,failures;static DWORD due;static HWND movie;
static void logText(char *s){HFILE f;f=_lopen("C:\\BUDQA.LOG",1);if(f==HFILE_ERROR)f=_lcreat("C:\\BUDQA.LOG",0);if(f==HFILE_ERROR)return;_llseek(f,0L,2);_lwrite(f,s,lstrlen(s));_lclose(f);}
static void check(char *s,int okay){char b[120];wsprintf(b,"%s %s\r\n",(LPSTR)(okay?"PASS":"FAIL"),(LPSTR)s);logText(b);if(!okay)failures++;}
static int contains(char *s,char *word){unsigned i,j;for(i=0;s[i];i++){for(j=0;word[j]&&s[i+j]==word[j];j++);if(!word[j])return 1;}return 0;}
static void readStop(char *reason){HFILE f;unsigned n;static char text[1500];f=_lopen("C:\\TINYVID.LOG",0);if(f==HFILE_ERROR){check("player report exists",0);return;}n=_lread(f,text,sizeof(text)-1);_lclose(f);if(n>=sizeof(text)){check("bounded player report",0);return;}text[n]=0;check(reason,contains(text,reason));check("owner released",contains(text,"owner_after_stop=0"));check("voices muted",contains(text,"sound_mask_after_stop=0"));check("no playback error",contains(text,"error=0"));}
static void finish(HWND w){char b[50];if(movie&&IsWindow(movie))SendMessage(movie,WM_CLOSE,0,0L);wsprintf(b,"TOTAL_FAILURES=%d\r\n",failures);logText(b);DestroyWindow(w);}
LONG FAR PASCAL BudQAProc(HWND w,UINT m,WPARAM wp,LPARAM lp){HFILE f;
 switch(m){
 case WM_CREATE:f=_lcreat("C:\\BUDQA.LOG",0);if(f!=HFILE_ERROR)_lclose(f);due=GetTickCount()+700L;SetTimer(w,1,55,NULL);return 0;
 case WM_TIMER:
  if((LONG)(GetTickCount()-due)<0)return 0;
  if(stage==0){check("launch player",WinExec("C:\\TINYVID.EXE",SW_SHOW)>=32);due=GetTickCount()+1200L;}
  else if(stage==1){movie=FindWindow("TinyFull",NULL);check("player window",movie!=NULL);if(!movie){finish(w);return 0;}SetActiveWindow(movie);SetFocus(movie);SendMessage(movie,WM_KEYDOWN,VK_SPACE,0L);due=GetTickCount()+800L;}
  else if(stage==2){SendMessage(movie,WM_KILLFOCUS,0,0L);readStop("Stopped: Space");due=GetTickCount()+800L;}
  else if(stage==3){SendMessage(movie,WM_KEYDOWN,VK_SPACE,0L);due=GetTickCount()+800L;}
  else if(stage==4){ShowWindow(movie,SW_MINIMIZE);readStop("Stopped: Space");due=GetTickCount()+600L;}
  else if(stage==5){ShowWindow(movie,SW_RESTORE);SetActiveWindow(movie);SetFocus(movie);SendMessage(movie,WM_KEYDOWN,VK_RETURN,0L);due=GetTickCount()+800L;}
  else if(stage==6){SendMessage(movie,WM_ACTIVATE,0,0L);readStop("Stopped: Space");due=GetTickCount()+600L;}
  else if(stage==7){SetActiveWindow(movie);SetFocus(movie);SendMessage(movie,WM_KEYDOWN,VK_SPACE,0L);due=GetTickCount()+5000L;}
  else{readStop("Done: Space song");finish(w);return 0;}
  stage++;return 0;
 case WM_CLOSE:finish(w);return 0;
 case WM_DESTROY:KillTimer(w,1);PostQuitMessage(0);return 0;
 }return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show){WNDCLASS wc;HWND w;MSG msg;
 wc.style=0;wc.lpfnWndProc=BudQAProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;wc.hCursor=NULL;wc.hbrBackground=NULL;wc.lpszMenuName=NULL;wc.lpszClassName="BudQA";
 if(!RegisterClass(&wc))return 2;w=CreateWindow("BudQA","Buddy QA",WS_POPUP,0,0,0,0,NULL,NULL,inst,NULL);if(!w)return 3;
 while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
 ExitWindows(0L,0);return failures;
}
