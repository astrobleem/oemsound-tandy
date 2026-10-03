/* Tandy Automatic Mouth: bounded word/sound experiment, not general TTS. */
#define WINVER 0x0300
#include <windows.h>
#define SPEAK 101
#define STOP 102
#define EDIT 103
#define TIMER 1
static HINSTANCE instance;
static HWND edit;
static HDC artDC;
static HBITMAP artBitmap, oldBitmap;
static int active, owned, pending, shape, automatic, step;
static unsigned pos, length;
static DWORD began, stopped;
static char *status = "HELLO TANDY YES NO";
/* Duration is in approximately 55-ms stock timer ticks. Oscillator clusters
   are deliberately crude vowel cues, not vocal-tract formant filters. */
struct frame { unsigned p[3]; BYTE a[3], noise, na, mouth, ticks; };
static struct frame sounds[] = {
 {{1,1,1},{15,15,15},4,15,0,2},       /* silence */
 {{1,1,1},{15,15,15},4,9,1,2},        /* breath H */
 {{185,66,43},{5,8,10},4,15,2,4},     /* EH */
 {{280,95,42},{6,9,11},4,15,1,2},     /* L */
 {{260,124,44},{5,7,11},4,15,3,4},    /* OH */
 {{360,160,45},{7,9,12},4,15,3,2},    /* rounded tail */
 {{1,1,1},{15,15,15},4,5,1,1},        /* stop burst */
 {{155,64,43},{5,7,10},4,15,2,4},     /* AE */
 {{450,120,47},{7,12,14},4,15,1,2},   /* nasal cue */
 {{340,48,38},{5,7,10},4,15,4,4},     /* EE */
 {{330,60,42},{7,9,11},4,15,4,2},     /* Y glide */
 {{1,1,1},{15,15,15},4,6,4,3}         /* S */
};
static BYTE hello[] = {1,2,3,4,5,0};
static BYTE tandy[] = {6,7,8,6,9,0};
static BYTE yes[] = {10,2,11,0};
static BYTE no[] = {8,4,5,0};
static BYTE *phrase;
static HFILE logFile = HFILE_ERROR;
static void record(char *label, unsigned value)
{
    char line[64];
    if (logFile == HFILE_ERROR) return;
    wsprintf(line,"%lu %s %u\r\n",GetTickCount(),(LPSTR)label,value);
    _lwrite(logFile,line,lstrlen(line));
}
static void output(BYTE value)
{
    _asm { mov dx, 0c0h
           mov al, value
           out dx, al
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
           nop }
    record("OUT",value);
}
static void mute(void)
{
    output(0x9f); output(0xbf); output(0xdf); output(0xff);
}
static void repaint(HWND w)
{
    RECT r;
    r.left=0; r.top=46; r.right=152; r.bottom=182;
    InvalidateRect(w,&r,TRUE);
}
static void stop(HWND w)
{
    KillTimer(w,TIMER); active=0; pending=0; shape=0;
    if (owned) { mute(); CloseSound(); owned=0; }
    status="Stopped"; repaint(w);
}
static void playFrame(HWND w)
{
    struct frame *f; unsigned v; RECT r;
    f=&sounds[phrase[pos]];
    for(v=0;v<3;v++) {
        output((BYTE)(0x80|(v<<5)|(f->p[v]&15)));
        output((BYTE)((f->p[v]>>4)&63));
        output((BYTE)(0x90|(v<<5)|f->a[v]));
    }
    output((BYTE)(0xe0|f->noise)); output((BYTE)(0xf0|f->na));
    shape=f->mouth; began=GetTickCount();
    record("FRAME",phrase[pos]);
    r.left=61; r.top=93; r.right=98; r.bottom=113;
    InvalidateRect(w,&r,FALSE);
}
static void speak(HWND w)
{
    char text[17]; unsigned i;
    stop(w); GetWindowText(edit,text,sizeof(text));
    record("INPUT",lstrlen(text));
    for(i=0;text[i];i++)
        if(text[i]>='a' && text[i]<='z') text[i]-='a'-'A';
    phrase=NULL; length=0;
    if(!lstrcmp(text,"HELLO")) { phrase=hello; length=sizeof(hello); }
    if(!lstrcmp(text,"TANDY")) { phrase=tandy; length=sizeof(tandy); }
    if(!lstrcmp(text,"YES")) { phrase=yes; length=sizeof(yes); }
    if(!lstrcmp(text,"NO")) { phrase=no; length=sizeof(no); }
    if(!phrase) { status="Try HELLO or TANDY"; record("UNKNOWN",0);
        repaint(w); return; }
    /* Direct-I/O examples cannot share the PSG. Check known examples,
       and acquire the Windows sound owner lock for driver-based clients. */
    if(FindWindow("TandyPSGTest",NULL) ||
       FindWindow("TandyStartupChime",NULL) || OpenSound()<0) {
        status="Sound busy"; record("BUSY",0); repaint(w); return;
    }
    owned=1; StopSound(); mute();
    if(!SetTimer(w,TIMER,55,NULL)) {
        stop(w); status="No timer"; record("ERROR",1); repaint(w); return;
    }
    active=1; pending=1; pos=0; status="Speaking...";
    record("START",length); repaint(w);
}
static void line(HDC dc,int x,int y,int xx,int yy)
{
    MoveTo(dc,x,y); LineTo(dc,xx,yy);
}
static void drawMouth(HDC dc)
{
    int h,width; HBRUSH old; RECT r;
    r.left=61;r.top=93;r.right=98;r.bottom=113;
    FillRect(dc,&r,GetStockObject(WHITE_BRUSH));
    old=SelectObject(dc,GetStockObject(BLACK_BRUSH));
    h=shape==0?6:shape==2?16:shape==3?17:shape==4?5:10;
    width=shape==3?13:shape==4?29:23;
    Ellipse(dc,78-width/2,102-h/2,79+width/2,103+h/2);
    SelectObject(dc,old);
}
static void drawMascot(HDC dc)
{
    HBRUSH brush,old; RECT r; unsigned i,j; int h,width,colors;
    POINT body[4];
    static DWORD rainbow[7]={RGB(255,0,0),RGB(255,128,0),
        RGB(255,255,0),RGB(0,255,0),RGB(0,128,255),
        RGB(0,0,255),RGB(128,0,128)};
    colors=GetDeviceCaps(dc,NUMCOLORS);
    if(colors>=16) for(i=0;i<7;i++) {
        brush=CreateSolidBrush(rainbow[i]);
        if(brush) { r.left=4; r.right=144; r.top=62+i*15;
            r.bottom=r.top+15; FillRect(dc,&r,brush); DeleteObject(brush); }
    }
    old=SelectObject(dc,GetStockObject(WHITE_BRUSH));
    /* CRT cabinet, glass, feet and keyboard echo the supplied mascot. */
    body[0].x=39;body[0].y=74;body[1].x=49;body[1].y=68;
    body[2].x=49;body[2].y=120;body[3].x=39;body[3].y=114;
    Polygon(dc,body,4);
    Rectangle(dc,48,67,108,120); Rectangle(dc,53,72,103,114);
    line(dc,70,65,72,62); line(dc,78,65,80,62);
    line(dc,75,119,72,124); line(dc,83,119,80,124);
    body[0].x=48;body[0].y=123;body[1].x=91;body[1].y=120;
    body[2].x=113;body[2].y=144;body[3].x=35;body[3].y=148;
    Polygon(dc,body,4);
    for(i=0;i<4;i++) for(j=0;j<8;j++)
        Rectangle(dc,51+j*6+i*2,127+i*4,55+j*6+i*2,130+i*4);
    line(dc,42,134,22,122); line(dc,22,122,18,105);
    Ellipse(dc,12,97,23,109);
    line(dc,104,131,122,115); line(dc,122,115,133,98);
    Ellipse(dc,129,91,140,102);
    line(dc,131,94,127,88);line(dc,135,93,134,85);
    line(dc,138,94,142,88);
    line(dc,55,148,52,160);line(dc,52,160,41,164);
    line(dc,90,147,96,161);line(dc,96,161,105,164);
    Ellipse(dc,30,160,54,170);Ellipse(dc,96,160,120,170);
    SelectObject(dc,GetStockObject(BLACK_BRUSH));
    Ellipse(dc,64,79,69,87); Ellipse(dc,88,77,93,85);
    line(dc,77,85,75,92); line(dc,75,92,80,92);
    h=shape==0?6:shape==2?16:shape==3?17:shape==4?5:10;
    width=shape==3?13:shape==4?29:23;
    Ellipse(dc,78-width/2,102-h/2,79+width/2,103+h/2);
    SelectObject(dc,old);
}
LONG FAR PASCAL MouthProc(HWND w,UINT m,WPARAM wp,LPARAM lp)
{
    HDC dc; PAINTSTRUCT ps; DWORD elapsed; RECT r; unsigned pose;
    switch(m) {
    case WM_CREATE:
        dc=GetDC(w); artDC=CreateCompatibleDC(dc);
        artBitmap=CreateCompatibleBitmap(dc,148,172); ReleaseDC(w,dc);
        if(artDC && artBitmap) {
            oldBitmap=SelectObject(artDC,artBitmap);
            r.left=0;r.top=0;r.right=148;r.bottom=172;
            FillRect(artDC,&r,GetStockObject(WHITE_BRUSH));
            drawMascot(artDC);
            /* Store five 37x20 mouth poses in unused top rows of the
               same bitmap. Playback performs only clipped BitBlt. */
            for(pose=0;pose<5;pose++) {
                shape=pose;
                SetViewportOrg(artDC,(pose%3)*40-61,(pose/3)*22-93);
                drawMouth(artDC);
            }
            SetViewportOrg(artDC,0,0); shape=0;
        } else {
            if(artDC) DeleteDC(artDC);
            if(artBitmap) DeleteObject(artBitmap);
            artDC=NULL; artBitmap=NULL;
        }
        edit=CreateWindow("EDIT","HELLO",WS_CHILD|WS_VISIBLE|WS_BORDER|
            WS_TABSTOP|ES_AUTOHSCROLL,4,4,140,18,w,(HMENU)EDIT,instance,NULL);
        CreateWindow("BUTTON","&Speak",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
            4,25,68,20,w,(HMENU)SPEAK,instance,NULL);
        CreateWindow("BUTTON","S&top",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
            76,25,68,20,w,(HMENU)STOP,instance,NULL);
        if(!edit) return -1;
        SendMessage(edit,EM_LIMITTEXT,16,0L);
        if(automatic) SetTimer(w,2,500,NULL);
        return 0;
    case WM_COMMAND:
        if(wp==SPEAK) speak(w);
        if(wp==STOP) stop(w);
        return 0;
    case WM_TIMER:
        if(wp==2 && automatic) {
            if(step==0) {
                if(automatic==6) SetWindowText(edit,"TANDY");
                if(automatic==7) SetWindowText(edit,"YES");
                if(automatic==8) SetWindowText(edit,"NO");
                if(automatic==9) SetWindowText(edit,"ABCDEFGHIJKLMNOPQRST");
                step++; speak(w); stopped=GetTickCount();
            }
            else if(automatic==2 && step==1 &&
                    GetTickCount()-stopped>=220L) {
                stop(w); step++; stopped=GetTickCount();
            } else if(automatic==3 && step==1 &&
                    GetTickCount()-stopped>=220L) {
                step++; speak(w);
            } else if(automatic==4 && step==1) {
                SetWindowText(edit,"?!UNKNOWN"); speak(w); step++;
            } else if(automatic==5 && step==1) {
                PostMessage(w,WM_CLOSE,0,0L);
            } else if(!active && GetTickCount()-stopped>=1500L) {
                PostMessage(w,WM_CLOSE,0,0L);
            }
            return 0;
        }
        if(wp!=TIMER || !active) return 0;
        if(pending) { pending=0; playFrame(w); return 0; }
        elapsed=GetTickCount()-began;
        if(elapsed>1100L) { stop(w); status="Timer delayed";
            record("LATE",0); repaint(w); return 0; }
        if(elapsed<(DWORD)sounds[phrase[pos]].ticks*55L) return 0;
        if(++pos>=length) { stop(w); status="Ready";
            record("DONE",0); repaint(w); }
        else playFrame(w);
        return 0;
    case WM_PAINT:
        dc=BeginPaint(w,&ps);
        SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));
        TextOut(dc,4,49,status,lstrlen(status));
        if(!(ps.rcPaint.left>=61 && ps.rcPaint.top>=93 &&
             ps.rcPaint.right<=98 && ps.rcPaint.bottom<=113)) {
            if(artDC) BitBlt(dc,0,62,148,110,artDC,0,62,SRCCOPY);
            else drawMascot(dc);
        }
        if(artDC) BitBlt(dc,61,93,37,20,artDC,
            (shape%3)*40,(shape/3)*22,SRCCOPY);
        else drawMouth(dc);
        EndPaint(w,&ps); return 0;
    case WM_QUERYENDSESSION: return TRUE;
    case WM_ENDSESSION: if(wp) stop(w); return 0;
    case WM_CLOSE: stop(w); DestroyWindow(w); return 0;
    case WM_DESTROY:
        stop(w); KillTimer(w,2);
        if(artDC) { SelectObject(artDC,oldBitmap); DeleteDC(artDC); }
        if(artBitmap) DeleteObject(artBitmap);
        artDC=NULL; artBitmap=NULL;
        PostQuitMessage(0); return 0;
    }
    return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show)
{
    WNDCLASS wc; HWND w; MSG msg;
    if(previous || (GetWinFlags()&WF_PMODE)) return 1;
    if(*(BYTE FAR *)0xfc000000UL!=0x21) return 2;
    instance=inst;
    if(cmd && cmd[0]=='/' && cmd[1]>='1' && cmd[1]<='9') {
        automatic=cmd[1]-'0'; logFile=_lcreat("C:\\MOUTH.LOG",0);
    }
    wc.style=0; wc.lpfnWndProc=MouthProc;
    wc.cbClsExtra=0; wc.cbWndExtra=0; wc.hInstance=inst;
    wc.hIcon=NULL; wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=NULL; wc.lpszClassName="TandyAutomaticMouth";
    if(!RegisterClass(&wc)) return 3;
    w=CreateWindow("TandyAutomaticMouth","Tandy Mouth",WS_OVERLAPPED|
        WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,
        0,0,160,198,NULL,NULL,inst,NULL);
    if(!w) return 4;
    ShowWindow(w,show); UpdateWindow(w); SetFocus(edit);
    while(GetMessage(&msg,NULL,0,0)) {
        if(msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE) {
            stop(w); continue;
        }
        if(msg.message==WM_KEYDOWN && msg.wParam==VK_RETURN) {
            speak(w); continue;
        }
        if(!IsDialogMessage(w,&msg)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
    }
    if(logFile!=HFILE_ERROR) _lclose(logFile);
    if(automatic) ExitWindows(0L,0);
    return msg.wParam;
}
