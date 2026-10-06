/* Original native Win3.0 instrument/latency test and A/B audition. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
static MIDIPROC midi;
static DEBUGPROC debug;
static HMODULE module;
static HWND window;
static HFILE logFile;
static DWORD begun,last,stageAt;
static WORD failures,checks,stage,services,maxGap,vibSeen,previous;
static WORD loadPaint,paintCount,audioMode=0xffff;
static int opened;
static void line(char *s) {_lwrite(logFile,s,lstrlen(s));_lwrite(logFile,"\r\n",2);}
static void check(char *name,int ok) {
    char s[100];++checks;if(!ok)++failures;
    wsprintf(s,"%s %s",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)name);line(s);
}
static DWORD call(WORD msg,DWORD p) {return midi(0,msg,MIDI_COOKIE,p,0L);}
static DWORD data(WORD s,WORD a,WORD b) {
    return call(MODM_DATA,(DWORD)s|((DWORD)a<<8)|((DWORD)b<<16));
}
static void closeDriver(void) {
    if(opened){check("reset",call(MODM_RESET,0L)==0L);
        check("reset silent",debug(0)==0);
        check("close",call(MODM_CLOSE,0L)==0L);opened=0;
        check("owner released",debug(1)==0);}
}
static int openDriver(void) {
    TOPEN desc;DWORD cookie=0,r;
    desc.hMidi=0;desc.dwCallback=desc.dwInstance=0;
    r=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L);
    check("open",r==0L&&cookie==MIDI_COOKIE);opened=r==0L;return opened;
}
static void finish(void) {
    char s[120];closeDriver();
    wsprintf(s,"CHECKS=%u TOTAL_FAILURES=%u SERVICES=%u MAX_GAP_MS=%u VIB_SEEN=%u PAINTS=%u",checks,failures,services,maxGap,vibSeen,paintCount);line(s);
    PostMessage(window,WM_CLOSE,0,0L);
}
static void trace(DWORD elapsed) {
    char s[130];wsprintf(s,"TRACE %lu %u %u %u %u %u %u %u",elapsed,
        debug(0),debug(8),debug(9),debug(10),debug(12),debug(13),debug(14));line(s);
}
static void audio(DWORD elapsed) {
    if(elapsed<stageAt)return;
    switch(stage++) {
    case 0:check("audio config",call(MODM_SYNTHCONFIG,(DWORD)audioMode)==0L);
        data(0xc0,0,0);data(0x90,60,127);stageAt=2000;break;
    case 1:data(0x80,60,0);stageAt=2400;break;
    case 2:data(0xc0,48,0);data(0x90,60,127);stageAt=4400;break;
    case 3:data(0x80,60,0);stageAt=4800;break;
    case 4:data(0xc0,80,0);data(0x90,64,127);stageAt=5900;break;
    case 5:data(0xc0,96,0);data(0x90,67,112);stageAt=7000;break;
    case 6:data(0x80,64,0);data(0x80,67,0);stageAt=7400;break;
    case 7:data(0xc0,32,0);data(0x90,48,127);
        data(0xc1,0,0);data(0x91,60,120);
        data(0xc2,80,0);data(0x92,67,112);data(0x99,38,112);
        stageAt=8500;break;
    case 8:call(MODM_RESET,0L);stageAt=9000;break;
    default:finish();break;
    }
}
static void test(DWORD elapsed) {
    DWORD t;WORD before,i;
    if(elapsed<stageAt)return;
    switch(stage++) {
    case 0:
        check("config flags",call(MODM_SYNTHCONFIG,3L)==0L);
        check("flags enabled",debug(15)==3);
        data(0x90,60,127);check("keys attack",debug(12)==0);
        data(0xc0,48,0);check("program channel state",debug(32)==48);
        check("program snapshot unchanged",debug(48)==0);
        data(0xc1,48,0);data(0x91,64,127);
        check("pad attack",debug(13)==12&&debug(49)==3);
        data(0xc2,16,0);data(0x92,67,127);
        check("organ attack",debug(14)==0&&debug(50)==1);
        previous=debug(9);stageAt=elapsed+1100;break;
    case 1:
        check("keys shaped",debug(12)==8);
        check("pad shaped",debug(13)==1);
        check("organ held",debug(14)==0);
        check("vibrato observed",vibSeen!=0);
        data(0x80,60,0);check("note off immediate",debug(4)==0);
        data(0xc1,80,0);check("held snapshot retained",debug(49)==3);
        data(0x90,72,127);data(0x93,76,127);
        check("oldest tone stolen",debug(5)==76);
        data(0x81,64,0);check("stale off harmless",debug(5)==76);
        data(0xb3,123,0);check("channel all notes off",debug(5)==0);
        check("other channels survive",debug(0)==5);
        data(0x99,42,127);check("hat starts",debug(7)==42);
        stageAt=elapsed+500;break;
    case 2:
        check("drum self expires",debug(7)==0);
        call(MODM_RESET,0L);check("reset silent",debug(0)==0);
        check("reset flags survive",debug(15)==3);
        check("reset programs default",debug(32)==0&&debug(33)==0);
        data(0xc0,96,0);data(0x90,60,127);
        data(0xc1,48,0);data(0x91,64,127);
        loadPaint=1;stageAt=elapsed+300;break;
    case 3:
        before=debug(2);t=GetTickCount();
        /* Deliberately do not Yield or dispatch for 880 ms. */
        while(GetTickCount()-t<880UL){}
        check("bounded catchup",call(MODM_SYNTHUPDATE,0L)==0L);
        check("catchup writes bounded",(WORD)(debug(2)-before)<=10);
        check("expired bell not replayed",debug(4)==0);
        check("late pad catches up",debug(13)==1);
        check("missed updates counted",debug(17)>0);
        check("starvation gap recorded",debug(18)>=880);
        stageAt=elapsed+900;break;
    case 4:
        loadPaint=0;call(MODM_RESET,0L);
        check("no pending voices",debug(0)==0);
        call(MODM_SYNTHCONFIG,1L);data(0xc0,80,0);data(0x90,60,127);
        stageAt=elapsed+700;break;
    case 5:
        check("vibrato off base pitch",debug(8)==428);
        check("per-pass write bound",debug(19)<=10);
        closeDriver();
        for(i=0;i<16;++i) {
            if(!openDriver())break;
            check("reopen legacy default",debug(15)==0);
            check("reopen programs default",debug(32)==0);
            call(MODM_SYNTHCONFIG,3L);data(0x90,60,100);
            closeDriver();
        }
        check("SOUND lease released",OpenSound()>=0);CloseSound();
        stageAt=elapsed+400;break;
    default:finish();break;
    }
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
    PAINTSTRUCT ps;HDC dc;DWORD now,gap,elapsed;WORD i;
    switch(m) {
    case WM_CREATE:window=w;if(!SetTimer(w,1,55,NULL))return -1;
        begun=last=GetTickCount();return 0;
    case WM_TIMER:
        if(wp!=1)return 0;
        now=GetTickCount();gap=now-last;last=now;
        if(gap>maxGap)maxGap=gap>65535UL?65535:(WORD)gap;
        ++services;elapsed=now-begun;
        if(opened)call(MODM_SYNTHUPDATE,0L);
        if(stage==1&&audioMode==0xffff&&debug(9)!=previous)++vibSeen;
        if(audioMode!=0xffff){audio(elapsed);trace(elapsed);}
        else test(elapsed);
        if(loadPaint){InvalidateRect(w,NULL,TRUE);UpdateWindow(w);}
        return 0;
    case WM_PAINT:
        dc=BeginPaint(w,&ps);++paintCount;
        TextOut(dc,4,4,"MIDI instruments",16);
        TextOut(dc,4,22,"3 tones + noise",15);
        TextOut(dc,4,40,"Windows 3 / 8088",16);
        if(loadPaint)for(i=0;i<24;++i) {
            Rectangle(dc,i*3,65,i*3+60,120);
            TextOut(dc,4,(i%4)*12+124,"Redraw load",11);
        }
        EndPaint(w,&ps);return 0;
    case WM_CLOSE:closeDriver();DestroyWindow(w);return 0;
    case WM_DESTROY:KillTimer(w,1);PostQuitMessage(0);return 0;
    }
    return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show) {
    WNDCLASS wc;MSG msg;HWND w;char s[70];
    logFile=_lcreat("C:\\INSTEST.LOG",0);
    wsprintf(s,"FLAGS=%u",GetWinFlags());line(s);
    if(lstrcmp(cmd,"/audio0")==0)audioMode=0;
    if(lstrcmp(cmd,"/audio1")==0)audioMode=1;
    if(lstrcmp(cmd,"/audio3")==0)audioMode=3;
    module=LoadLibrary("C:\\MIDIMAP.DRV");
    if(module<32){line("FAIL load");goto end;}
    midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
    debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    if(!midi||!debug||!openDriver()){line("FAIL entries/open");goto end;}
    wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;
    wc.cbClsExtra=wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=NULL;wc.lpszClassName="InstrumentTest";
    if(!RegisterClass(&wc)){line("FAIL class");goto end;}
    w=CreateWindow("InstrumentTest","PSG instruments",WS_OVERLAPPED|
        WS_CAPTION|WS_SYSMENU,0,0,156,195,NULL,NULL,inst,NULL);
    if(!w){line("FAIL window");goto end;}ShowWindow(w,show);UpdateWindow(w);
    while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
end:
    closeDriver();if(module>=32)FreeLibrary(module);_lclose(logFile);
    ExitWindows(0L,0);return failures;
}
