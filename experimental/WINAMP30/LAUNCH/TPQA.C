#define WINVER 0x0300
#include <windows.h>
static HWND player,window;
static HFILE log;
static WORD phase,frames,failures;
static DWORD started;
static char driverFile[128],expectedDriver[32],driverLog[160];
static void check(char *name,int okay)
{
    char b[128];wsprintf(b,"%s=%u\r\n",(LPSTR)name,okay?1:0);
    _lwrite(log,b,lstrlen(b));if(!okay)++failures;
}
LONG FAR PASCAL WndProc(HWND w,WORD m,WORD a,LONG b)
{
    if(m==WM_TIMER){
        if(!phase){
            player=FindWindow("MiniMCIPlayer",0);
            if(!player){if(GetTickCount()-started>15000UL){check("player_found",0);phase=3;}return 0L;}
            {
                HMODULE display;char *base;WORD i;
                GetProfileString("TPQA","driver","",expectedDriver,
                                 sizeof(expectedDriver));
                if(expectedDriver[0]){
                    display=GetModuleHandle("DISPLAY");
                    check("display_module",display!=0);
                    GetModuleFileName(display,driverFile,sizeof(driverFile));
                    base=driverFile;
                    for(i=0;driverFile[i];++i)
                        if(driverFile[i]=='\\')base=driverFile+i+1;
                    wsprintf(driverLog,"loaded_display=%s\r\n",
                             (LPSTR)driverFile);
                    _lwrite(log,driverLog,lstrlen(driverLog));
                    check("selected_display_file",
                          lstrcmpi(base,expectedDriver)==0);
                }
            }
            check("player_found",1);check("expected_width",(unsigned)GetSystemMetrics(SM_CXSCREEN)==GetProfileInt("TPQA","width",640));
            check("expected_height",(unsigned)GetSystemMetrics(SM_CYSCREEN)==GetProfileInt("TPQA","height",256));
            SendMessage(player,WM_COMMAND,101,0L);
            check("play_started",SendMessage(player,WM_USER+47,0,0L)!=0);
            phase=1;started=GetTickCount();
        }else if(phase==1){
            RECT r;HDC dc;HBRUSH brush;
            MoveWindow(player,(frames&1)?2:14,2,304,192,TRUE);
            InvalidateRect(player,0,TRUE);UpdateWindow(player);
            dc=GetDC(w);SetRect(&r,0,0,24,24);
            brush=CreateSolidBrush((frames&1)?RGB(255,0,0):RGB(0,255,0));
            FillRect(dc,&r,brush);DeleteObject(brush);ReleaseDC(w,dc);++frames;
            if(GetTickCount()-started>4000UL){
                check("graphics_updates",frames>=8);
                check("foreground_audio_service",SendMessage(player,WM_USER+42,0,0L)>0);
                check("playing_during_graphics",SendMessage(player,WM_USER+47,0,0L)!=0);
                SendMessage(player,WM_COMMAND,102,0L);
                check("stop_closed",SendMessage(player,WM_USER+47,0,0L)==0);
                check("sound_released",OpenSound()==3);CloseSound();
                PostMessage(player,WM_CLOSE,0,0L);phase=2;started=GetTickCount();
            }
        }else if(phase==2){
            if(!IsWindow(player)){
                check("player_closed",1);check("all_pass",failures==0);
                _lclose(log);log=HFILE_ERROR;KillTimer(w,1);ExitWindows(0L,0);
            }else if(GetTickCount()-started>5000UL){check("close_timeout",0);phase=3;}
        }else if(phase==3){_lclose(log);log=HFILE_ERROR;KillTimer(w,1);ExitWindows(0L,0);}
        return 0L;
    }
    return DefWindowProc(w,m,a,b);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
    WNDCLASS wc;MSG msg;WORD launched;char zero[sizeof(WNDCLASS)];WORD i;
    for(i=0;i<sizeof(zero);++i)zero[i]=0;wc=*(WNDCLASS *)zero;
    log=_lcreat("C:\\TPQA.LOG",0);check("entered",1);
    wc.lpfnWndProc=WndProc;wc.hInstance=inst;wc.lpszClassName="TPQA";
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);RegisterClass(&wc);
    window=CreateWindow("TPQA","Graphics activity",WS_OVERLAPPEDWINDOW,350,20,120,90,0,0,inst,0L);
    ShowWindow(window,SW_SHOW);SetTimer(window,1,200,0);started=GetTickCount();
    launched=WinExec("C:\\MMLAB\\MINIPLAY.EXE",SW_SHOWNORMAL);
    check("winexec",launched>=32);
    while(GetMessage(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
    return failures;
}
