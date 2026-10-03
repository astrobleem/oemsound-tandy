/* Mini eight-step sequencer; Windows 3.0 real mode, Microsoft C 6. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#define PLAY 101
#define STOP 102
#define SLOW 103
#define FAST 104
#define LOWER 105
#define HIGHER 106
#define DEMO 107
#define CLEAR 108
#define PULSE 1
static HINSTANCE instance,module;
static HWND mainWindow;
static MIDIPROC midi;
static DEBUGPROC debug;
static int opened,playing,step,selLane,selStep,timerReady;
static int automatic,testStage,failures,stepsPlayed;
static unsigned tempo=120;
static DWORD due,gate,ended,beginTick,lastTick;
static WORD sounding[4];
static BYTE pattern[4][8];
static BYTE defaults[4]={60,64,48,42};
static BYTE channels[4]={0,1,2,9};
static char *notes[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static char *status="Driver ready";
static HFILE logFile=HFILE_ERROR;
static HDC gridDC;
static HBITMAP gridBitmap,oldBitmap;
static BYTE gridBits[1440];
static BYTE masks[8]={128,64,32,16,8,4,2,1};
static BYTE digits[9][5]={
 {2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},
 {5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},
 {7,1,2,2,2},{7,5,7,5,7},{5,7,7,7,5}};
static void logText(char *text)
{
    if(logFile!=HFILE_ERROR) { _lwrite(logFile,text,lstrlen(text));
        _lwrite(logFile,"\r\n",2); }
}
static void check(char *name,int okay)
{
    char line[80];
    wsprintf(line,"%s %s",(LPSTR)(okay ? "PASS":"FAIL"),(LPSTR)name); logText(line);
    if(!okay) ++failures;
}
static void redraw(void) { if(mainWindow) InvalidateRect(mainWindow,NULL,FALSE); }
static DWORD message(WORD m,DWORD value)
{
    if(!midi) return MMSYSERR_ERROR;
    return midi(0,m,MIDI_COOKIE,value,0L);
}
static int shortMessage(unsigned s,unsigned a,unsigned b)
{
    DWORD r=message(MODM_DATA,(DWORD)s|((DWORD)a<<8)|((DWORD)b<<16));
    if(r) { status="MIDI error"; ++failures; return 0; }
    return 1;
}
static void releaseNotes(void)
{
    int i;
    for(i=0;i<4;++i) if(sounding[i]) {
        shortMessage(0x80|channels[i],sounding[i],0); sounding[i]=0;
    }
}
static void stop(void)
{
    playing=0;
    if(opened) { releaseNotes(); message(MODM_RESET,0L); }
    status="Stopped / muted"; redraw();
}
static void preset(void)
{
    int r,c;
    for(r=0;r<4;++r) for(c=0;c<8;++c) pattern[r][c]=0;
    pattern[0][0]=60; pattern[0][2]=67; pattern[0][4]=64; pattern[0][6]=72;
    pattern[1][0]=64; pattern[1][3]=67; pattern[1][4]=67; pattern[1][7]=71;
    pattern[2][0]=48; pattern[2][2]=48; pattern[2][4]=53; pattern[2][6]=55;
    for(c=0;c<8;++c) pattern[3][c]=(c==2||c==6) ? 38:42;
}
static void nextStep(DWORD now)
{
    int r; unsigned duration;
    releaseNotes();
    for(r=0;r<4;++r) if(pattern[r][step]) {
        if(shortMessage(0x90|channels[r],pattern[r][step],r==3?76:96))
            sounding[r]=pattern[r][step];
    }
    duration=(unsigned)(30000UL/tempo); /* eighth notes */
    gate=now+(duration*3U)/4U; due+=duration;
    ++stepsPlayed; status="Playing MIDI"; redraw();
}
static void play(void)
{
    if(!opened || !timerReady) return;
    stop(); step=0; stepsPlayed=0; playing=1; due=GetTickCount(); nextStep(due);
}
static void editPitch(int direction)
{
    int value=pattern[selLane][selStep];
    if(!value) value=defaults[selLane];
    else value+=direction;
    if(selLane==3) { if(value<35)value=35; if(value>81)value=81; }
    else { if(value<45)value=45; if(value>96)value=96; }
    pattern[selLane][selStep]=(BYTE)value; redraw();
}
static void toggle(void)
{
    pattern[selLane][selStep]=pattern[selLane][selStep]?0:defaults[selLane];
    redraw();
}
static void pixel(int x,int y)
{
    gridBits[y*18+(x>>3)]&=(BYTE)~masks[x&7];
}
static void digit(int index,int x,int y)
{
    int a,b;for(b=0;b<5;++b)for(a=0;a<3;++a)
        if(digits[index][b]&(4>>a))pixel(x+a,y+b);
}
static void drawGrid(void)
{
    int i,r,c,x,y,a,b;
    for(i=0;i<1440;++i)gridBits[i]=255;
    for(c=0;c<8;++c)digit(c,21+c*16,0);
    for(r=0;r<4;++r) {
        digit(r==3?8:r,1,18+r*16);
        for(c=0;c<8;++c) {
            x=16+c*16;y=15+r*16;
            for(a=0;a<14;++a){pixel(x+a,y);pixel(x+a,y+13);}
            for(b=1;b<13;++b){pixel(x,y+b);pixel(x+13,y+b);}
            if(pattern[r][c])for(b=3;b<11;++b)for(a=3;a<11;++a)
                pixel(x+a,y+b);
            if(r==selLane && c==selStep)for(a=1;a<13;a+=2) {
                pixel(x+a,y+1);pixel(x+a,y+12);
                pixel(x+1,y+a);pixel(x+12,y+a);
            }
            if(playing && c==step)for(a=0;a<14;++a)pixel(x+a,y+14);
        }
    }
    SetBitmapBits(gridBitmap,1440L,(LPSTR)gridBits);
}
static void paint(HWND w)
{
    HDC dc;PAINTSTRUCT ps;int v;char line[32];RECT rect;
    dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));
    SetBkMode(dc,OPAQUE);SetBkColor(dc,RGB(255,255,255));
    SetTextColor(dc,RGB(0,0,0));
    wsprintf(line,"%u BPM ",tempo);TextOut(dc,44,29,line,lstrlen(line));
    drawGrid();BitBlt(dc,2,45,144,80,gridDC,0,0,SRCCOPY);
    v=pattern[selLane][selStep];
    if(!v)wsprintf(line,"%d:%d Rest       ",selLane+1,selStep+1);
    else if(selLane==3)wsprintf(line,"N:%d Drum %d    ",selStep+1,v);
    else wsprintf(line,"%d:%d %s%d       ",selLane+1,selStep+1,
        (LPSTR)notes[v%12],v/12-1);
    TextOut(dc,2,126,line,lstrlen(line));
    rect.left=2;rect.top=165;rect.right=148;rect.bottom=178;
    FillRect(dc,&rect,GetStockObject(WHITE_BRUSH));
    TextOut(dc,2,165,status,lstrlen(status));
    EndPaint(w,&ps);
}
static void button(HWND w,char *label,int x,int y,int width,int id)
{
    CreateWindow("BUTTON",label,WS_CHILD|WS_VISIBLE|WS_TABSTOP,
        x,y,width,20,w,(HMENU)id,instance,NULL);
}
static void testTick(DWORD now)
{
    if(automatic==1 && playing && stepsPlayed>=17) {
        stop(); check("two loops completed",stepsPlayed==17);
        check("reset leaves all voices off",debug && debug(0)==0);
        testStage=1; ended=now; automatic=4;
    }
    if(automatic==2 && testStage==0 && now-beginTick>400L) {
        SendMessage(mainWindow,WM_LBUTTONDOWN,0,MAKELONG(21,63));
        check("mouse toggles first cell",pattern[0][0]==0);
        SendMessage(mainWindow,WM_KEYDOWN,VK_SPACE,0L);
        check("space restores first cell",pattern[0][0]==60);
        SendMessage(mainWindow,WM_COMMAND,HIGHER,0L);
        check("pitch plus edits selected cell",pattern[0][0]==61);
        SendMessage(mainWindow,WM_COMMAND,SLOW,0L);
        check("tempo decrease",tempo==110);
        SendMessage(mainWindow,WM_COMMAND,FAST,0L);
        check("tempo increase",tempo==120);
        SendMessage(mainWindow,WM_COMMAND,PLAY,0L);
        SendMessage(mainWindow,WM_COMMAND,PLAY,0L);
        check("repeat play restarts",playing && stepsPlayed==1);
        testStage=1; ended=now;
    } else if(automatic==2 && testStage==1 && now-ended>650L) {
        SendMessage(mainWindow,WM_COMMAND,STOP,0L);
        SendMessage(mainWindow,WM_COMMAND,STOP,0L);
        check("repeated stop mutes",!playing && debug && debug(0)==0);
        SendMessage(mainWindow,WM_COMMAND,CLEAR,0L);
        check("clear empties pattern",pattern[0][0]==0 && pattern[3][7]==0);
        SendMessage(mainWindow,WM_COMMAND,DEMO,0L);
        check("demo restores pattern",pattern[0][0]==60 && pattern[3][7]==42);
        testStage=2; ended=now;
    }
    if((automatic==4 || (automatic==2 && testStage==2)) && now-ended>700L)
        PostMessage(mainWindow,WM_CLOSE,0,0L);
    if(automatic==3 && now-beginTick>600L)
        PostMessage(mainWindow,WM_CLOSE,0,0L);
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp)
{
    DWORD now; int x,y,r,c;HDC temp;
    switch(m) {
    case WM_CREATE:
        mainWindow=w;
        temp=GetDC(w);gridDC=CreateCompatibleDC(temp);ReleaseDC(w,temp);
        gridBitmap=CreateBitmap(144,80,1,1,NULL);
        if(!gridDC || !gridBitmap)return -1;
        oldBitmap=SelectObject(gridDC,gridBitmap);
        button(w,"Play",2,3,44,PLAY); button(w,"Stop",50,3,44,STOP);
        button(w,"Demo",98,3,48,DEMO);
        button(w,"-",2,26,30,SLOW); button(w,"+",116,26,30,FAST);
        button(w,"Pitch-",2,141,48,LOWER);
        button(w,"Pitch+",52,141,48,HIGHER);
        button(w,"Clear",102,141,44,CLEAR);
        timerReady=SetTimer(w,PULSE,30,NULL)!=0;
        if(!timerReady) { status="Timer failed"; ++failures; return -1; }
        beginTick=GetTickCount();
        if(automatic==1 || automatic==3) PostMessage(w,WM_COMMAND,PLAY,0L);
        return 0;
    case WM_COMMAND:
        if(wp==PLAY) play();
        if(wp==STOP) stop();
        if(wp==SLOW && tempo>40) { tempo-=10; redraw(); }
        if(wp==FAST && tempo<240) { tempo+=10; redraw(); }
        if(wp==LOWER) editPitch(-1);
        if(wp==HIGHER) editPitch(1);
        if(wp==DEMO) { stop(); preset(); redraw(); }
        if(wp==CLEAR) { stop(); for(r=0;r<4;++r)
            for(c=0;c<8;++c)pattern[r][c]=0; redraw(); }
        return 0;
    case WM_LBUTTONDOWN:
        x=LOWORD(lp); y=HIWORD(lp);
        if(x>=18 && x<146 && y>=60 && y<124) {
            selStep=(x-18)/16; selLane=(y-60)/16; toggle(); SetFocus(w);
        }
        return 0;
    case WM_KEYDOWN:
        if(wp==VK_LEFT) selStep=(selStep+7)%8;
        if(wp==VK_RIGHT) selStep=(selStep+1)%8;
        if(wp==VK_UP) selLane=(selLane+3)%4;
        if(wp==VK_DOWN) selLane=(selLane+1)%4;
        if(wp==VK_SPACE) toggle();
        if(wp==VK_ESCAPE || wp=='S') stop();
        if(wp=='P') play();
        if(wp==VK_ADD || wp==0xbb) editPitch(1);
        if(wp==VK_SUBTRACT || wp==0xbd) editPitch(-1);
        redraw(); return 0;
    case WM_TIMER:
        if(wp!=PULSE) return 0;
        now=GetTickCount(); lastTick=now;
        if(playing) {
            if((LONG)(now-gate)>=0) releaseNotes();
            if((LONG)(now-due)>=0) {
                /* Drop stale intervals after a long cooperative stall;
                   never send a burst of catch-up notes. */
                if(now-due>1000UL) due=now;
                step=(step+1)%8; nextStep(now);
            }
        }
        if(automatic) testTick(now);
        return 0;
    case WM_PAINT: paint(w); return 0;
    case WM_QUERYENDSESSION: return TRUE;
    case WM_ENDSESSION: if(wp)stop(); return 0;
    case WM_CLOSE: stop(); DestroyWindow(w); return 0;
    case WM_DESTROY:
        KillTimer(w,PULSE);timerReady=0;stop();
        if(gridDC && oldBitmap)SelectObject(gridDC,oldBitmap);
        if(gridBitmap)DeleteObject(gridBitmap);
        if(gridDC)DeleteDC(gridDC);
        gridBitmap=0;gridDC=0;PostQuitMessage(0);return 0;
    }
    return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show)
{
    WNDCLASS wc; HWND w; MSG msg; char line[80],path[144]; int i;
    TOPEN desc; DWORD cookie=0; TCAPS caps;
    if(previous) return 1;
    if(GetWinFlags()&WF_PMODE) return 2;
    instance=inst;
    if(lstrcmp(cmd,"/test")==0)automatic=1;
    if(lstrcmp(cmd,"/uitest")==0)automatic=2;
    if(lstrcmp(cmd,"/closetest")==0)automatic=3;
    if(automatic)logFile=_lcreat("C:\\BEATS.LOG",0);
    GetModuleFileName(inst,path,sizeof(path));
    for(i=lstrlen(path)-1;i>=0;--i) if(path[i]=='\\')break;
    lstrcpy(path+i+1,"MIDIMAP.DRV");
    module=LoadLibrary(path);
    if(module>=32) {
        midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
        debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    }
    if(midi) {
        desc.hMidi=0;desc.dwCallback=0L;desc.dwInstance=0L;
        if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))==0L &&
           caps.wVoices==3 && caps.vDriverVersion>=0x0101 &&
           midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L)==0L)
            opened=1;
    }
    if(!opened) {
        logText("FAIL driver missing, old, unavailable or already in use");
        if(!automatic) MessageBox(NULL,
            "Use the paired MIDIMAP.DRV.\nPSG may be busy.","Beats",MB_OK);
        if(module>=32)FreeLibrary(module);
        if(logFile!=HFILE_ERROR)_lclose(logFile);
        if(automatic)ExitWindows(0L,0);
        return 3;
    }
    check("driver open cookie",cookie==MIDI_COOKIE);
    preset();
    wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;
    wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;
    wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=NULL;wc.lpszClassName="TandyBeatsLab";
    if(!RegisterClass(&wc)) {message(MODM_CLOSE,0L);FreeLibrary(module);return 4;}
    w=CreateWindow("TandyBeatsLab","Tandy Beats",WS_OVERLAPPED|WS_CAPTION|
        WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,
        NULL,NULL,inst,NULL);
    if(!w) {message(MODM_CLOSE,0L);FreeLibrary(module);return 5;}
    ShowWindow(w,show);UpdateWindow(w);
    while(GetMessage(&msg,NULL,0,0)) {
        if(msg.message==WM_KEYDOWN) {
            if(msg.wParam==VK_TAB) { /* basic tab cycle for native controls */
                HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);
                if(next)SetFocus(next); continue;
            }
            if(msg.wParam==VK_ESCAPE || msg.wParam=='P' || msg.wParam=='S' ||
               msg.wParam==VK_LEFT || msg.wParam==VK_RIGHT ||
               msg.wParam==VK_UP || msg.wParam==VK_DOWN ||
               msg.wParam==VK_ADD || msg.wParam==VK_SUBTRACT ||
               msg.wParam==0xbb || msg.wParam==0xbd ||
               (msg.wParam==VK_SPACE && msg.hwnd==w)) {
                SendMessage(w,WM_KEYDOWN,msg.wParam,msg.lParam);continue;
            }
        }
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
    stop();check("close mutes",debug && debug(0)==0);
    check("close returns success",message(MODM_CLOSE,0L)==0L);opened=0;
    check("driver releases owner",debug && debug(1)==0);
    FreeLibrary(module);
    wsprintf(line,"TOTAL_FAILURES=%d STEPS=%d",failures,stepsPlayed);logText(line);
    if(logFile!=HFILE_ERROR)_lclose(logFile);
    if(automatic)ExitWindows(0L,0);
    return failures?6:0;
}
