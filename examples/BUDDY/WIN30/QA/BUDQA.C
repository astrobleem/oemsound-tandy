#define WINVER 0x0300
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
static HWND player,control;static WORD phase;static DWORD stamp,begin;
static FARPROC thunk;
static volatile DWORD FAR *playerTrace;
static int runningForOneSecond(void)
{
    HFILE f;char row[96],*seg,*off;unsigned n;DWORD a,b;
    if(!playerTrace) {
        f=_lopen("C:\\BUDDY\\TVMARK.LOG",OF_READ);if(f==-1)return 0;
        n=_lread(f,row,95);_lclose(f);row[n]=0;
        seg=strstr(row,"seg=");off=strstr(row,"offset=");
        if(!seg||!off)return 0;
        a=strtoul(seg+4,NULL,10);b=strtoul(off+7,NULL,10);
        if(!a||a>65535UL||b>65470UL)return 0;
        playerTrace=(volatile DWORD FAR *)((a<<16)|b);
    }
    return playerTrace[0]==0x54435633UL&&playerTrace[1]==1&&playerTrace[3]>=1000UL;
}
static void mark(char *text)
{
    HFILE f;char path[40];path[0]=0;
    lstrcpy(path,"C:\\BUDDY\\QA_PHASE.LOG");f=_lcreat(path,0);
    if(f!=-1){_lwrite(f,text,lstrlen(text));_lclose(f);}
}
static void copyLog(char *name)
{
    HFILE a,b;char data[3072],path[80];unsigned n;
    a=_lopen("C:\\BUDDY\\TINYVID.LOG",OF_READ);if(a==-1)return;
    n=_lread(a,data,sizeof(data));_lclose(a);
    lstrcpy(path,"C:\\BUDDY\\");lstrcat(path,name);b=_lcreat(path,0);
    if(b!=-1){_lwrite(b,data,n);_lclose(b);}
}
static void restart(void)
{
    ShowWindow(player,SW_RESTORE);SetActiveWindow(player);SetFocus(player);
    PostMessage(player,WM_KEYDOWN,VK_SPACE,0L);
}
LONG FAR PASCAL __export Proc(HWND w,UINT m,WPARAM a,LPARAM b)
{
    DWORD now;HFILE f;char head[8];
    if(m==WM_TIMER) {
        now=GetTickCount();if(now-begin>650000UL){mark("TIMEOUT");DestroyWindow(w);return 0;}
        if(!player)player=FindWindow("TinyFull",NULL);
        if(!player)return 0;
        if(!phase){f=_lopen("C:\\BUDDY\\TINYVID.LOG",OF_READ);
            if(f==-1)return 0;head[0]=0;_lread(f,head,4);head[4]=0;_lclose(f);
            if(lstrcmp(head,"Done"))return 0;
            phase=1;stamp=now;mark("NATURAL");return 0;}
        if(now-stamp<1500UL)return 0;
        /* Measure actual running time, rather than queuing pause/close
           while another Win16 task is still preparing its first frame. */
        if((phase==2||phase==4||phase==5||phase==8||phase==10||
            phase==12||phase==14)&&!runningForOneSecond())return 0;
        stamp=now;
        switch(phase++) {
        case 1:copyLog("NATURAL.LOG");restart();mark("START_PAUSE");break;
        case 2:PostMessage(player,WM_KEYDOWN,VK_SPACE,0L);mark("PAUSE1");break;
        case 3:copyLog("PAUSE1.LOG");restart();mark("RESUME1");break;
        case 4:PostMessage(player,WM_KEYDOWN,VK_RIGHT,0L);mark("FORWARD");break;
        case 5:PostMessage(player,WM_KEYDOWN,VK_SPACE,0L);mark("PAUSE2");break;
        case 6:copyLog("PAUSE2.LOG");PostMessage(player,WM_KEYDOWN,VK_LEFT,0L);mark("BACK_PAUSED");break;
        case 7:copyLog("BACK.LOG");restart();mark("RESUME2");break;
        case 8:ShowWindow(player,SW_MINIMIZE);mark("MINIMIZED");break;
        case 9:copyLog("MIN.LOG");restart();mark("START_FOCUS");break;
        case 10:ShowWindow(control,SW_SHOWNOACTIVATE);SetActiveWindow(control);
            SetFocus(control);mark("FOCUS_LOST");break;
        case 11:copyLog("FOCUS.LOG");ShowWindow(control,SW_HIDE);restart();mark("START_ESCAPE");break;
        case 12:PostMessage(player,WM_KEYDOWN,VK_ESCAPE,0L);mark("ESCAPE");break;
        case 13:copyLog("ESC.LOG");ShowWindow(player,SW_RESTORE);SetActiveWindow(player);SetFocus(player);PostMessage(player,WM_KEYDOWN,VK_RETURN,0L);mark("START_FULL");break;
        case 14:PostMessage(player,WM_CLOSE,0,0L);mark("CLOSE");break;
        case 15:
            /* Posted close is asynchronous across Win16 tasks. Wait for its
               acknowledged completion before measuring or exiting Windows. */
            if(FindWindow("TinyFull",NULL)) {
                PostMessage(player,WM_CLOSE,0,0L);phase--;return 0;
            }
            if(GetModuleHandle("SPKCLK")||GetModuleHandle("SPKFAST")) {
                phase--;return 0;
            }
            copyLog("CLOSE.LOG");
            f=_lcreat("C:\\BUDDY\\QA_DONE.LOG",0);
            if(f!=-1){int acquired;
                head[0]=FindWindow("TinyFull",NULL)?'1':'0';
                head[1]=(char)('0'+(inp(0x61)&3));
                head[2]=(GetModuleHandle("SPKCLK")||GetModuleHandle("SPKFAST"))?'1':'0';
                acquired=OpenSound();head[3]=acquired<0?'1':'0';
                if(acquired>=0)CloseSound();
                _lwrite(f,head,4);_lclose(f);}
            mark("DONE");ExitWindows(0L,0);break;
        }
        return 0;
    }
    if(m==WM_DESTROY){KillTimer(w,1);PostQuitMessage(0);return 0;}
    return DefWindowProc(w,m,a,b);
}
int PASCAL WinMain(HINSTANCE i,HINSTANCE p,LPSTR c,int show)
{
    WNDCLASS wc;MSG msg;HWND w;(void)c;(void)show;
    thunk=MakeProcInstance((FARPROC)Proc,i);
    wc.style=0;wc.lpfnWndProc=(WNDPROC)thunk;wc.cbClsExtra=wc.cbWndExtra=0;
    wc.hInstance=i;wc.hIcon=0;wc.hCursor=0;wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=0;wc.lpszClassName="BuddyNativeQA";
    if(!RegisterClass(&wc))return 1;
    w=CreateWindow("BuddyNativeQA","",WS_POPUP,130,90,1,1,0,0,i,0);control=w;
    SetCursorPos(150,94);
    begin=GetTickCount();SetTimer(w,1,110,NULL);
    {char cmd[96],args[16];HFILE f;unsigned n;
        args[0]=0;f=_lopen("C:\\BUDDY\\QACMD.TXT",OF_READ);
        if(f!=-1){n=_lread(f,args,15);args[n]=0;_lclose(f);}
        lstrcpy(cmd,"C:\\BUDDY\\TINYVID.EXE ");lstrcat(cmd,args);
        WinExec(cmd,SW_SHOWNORMAL);}
    while(GetMessage(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
    FreeProcInstance(thunk);return 0;
}
