/* Mini eight-step sequencer; Windows 3.0 real mode, Microsoft C 6. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include <io.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "BEATEXP.C"
#define PLAY 101
#define STOP 102
#define SLOW 103
#define FAST 104
#define LOWER 105
#define HIGHER 106
#define DEMO 107
#define CLEAR 108
#define PULSE 1
#define PATVIEW 110
#define MMLVIEW 111
#define COPYMML 112
#define SAVEMML 113
#define MMLTEXT 114
#define FILENAME 115
#define PIT 116
static int pitEnabled;
static HINSTANCE instance,module;
static HWND mainWindow;
static MIDIPROC midi;
static DEBUGPROC debug;
static int opened,playing,step,selLane,selStep,timerReady;
static int automatic,testStage,failures,stepsPlayed;
static unsigned tempo=120;
static int mmlView;
static HWND mmlEdit,fileEdit;
static FARPROC editThunk,oldEdit;
static char mmlText[BEAT_MML_CAP],testCopy[BEAT_MML_CAP],appdir[144];
static DWORD due,gate,ended,beginTick,lastTick;
static WORD sounding[5];
static BYTE pattern[5][8];
static BYTE defaults[5]={60,64,48,42,72};
static BYTE channels[5]={0,1,2,9,15};
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
        _lwrite(logFile,"\r\n",2);
        _lclose(logFile);logFile=_lopen("C:\\BEATS.LOG",OF_WRITE);
        if(logFile!=HFILE_ERROR)_llseek(logFile,0L,2); }
}
static void check(char *name,int okay)
{
    char line[80];
    wsprintf(line,"%s %s",(LPSTR)(okay ? "PASS":"FAIL"),(LPSTR)name); logText(line);
    if(!okay) ++failures;
}
static void redraw(void) { if(mainWindow) InvalidateRect(mainWindow,NULL,FALSE); }
static void showView(int view);
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
    for(i=0;i<5;++i) if(sounding[i]) {
        shortMessage(0x80|channels[i],sounding[i],0); sounding[i]=0;
    }
}
static void stop(void)
{
    playing=0;
    if(opened) {
        releaseNotes(); message(MODM_RESET,0L);
        message(MODM_CLOSE,0L); opened=0;
    }
    status="Stopped / released"; redraw();
}
static int acquire(void)
{
    static TOPEN desc;
    static DWORD cookie;
    static TCAPS caps;
    DWORD result;
    if(!midi) { status="Driver missing"; redraw(); return 0; }
    if(message(MODM_GETNUMDEVS,0L)!=1L ||
       midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps)) ||
       caps.wVoices!=3 || caps.vDriverVersion<0x0102) {
        status="Wrong driver"; redraw(); return 0;
    }
    if(pitEnabled && !GetProcAddress(GetModuleHandle("SOUND"),
        "TANDYPITVOICE")){status="PIT unavailable";redraw();return 0;}
    desc.hMidi=0;desc.dwCallback=0L;desc.dwInstance=0L;cookie=0;
    result=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,
                (DWORD)(LPVOID)&desc,0L);
    if(result) { status=result==MMSYSERR_ALLOCATED?"Sound busy":"Open failed";
        redraw(); return 0; }
    opened=1;
    if(cookie!=MIDI_COOKIE) { stop();status="Driver mismatch";redraw();return 0; }
    if(message(MODM_SYNTHCONFIG,3L)) { stop();status="Synth failed";redraw();return 0; }
    if(pitEnabled && midi(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1L,1L)) {
        stop();status="PIT unavailable";redraw();return 0;
    }
    return 1;
}
static void preset(void)
{
    int r,c;
    for(r=0;r<5;++r) for(c=0;c<8;++c) pattern[r][c]=0;
    pattern[0][0]=60; pattern[0][2]=67; pattern[0][4]=64; pattern[0][6]=72;
    pattern[1][0]=64; pattern[1][3]=67; pattern[1][4]=67; pattern[1][7]=71;
    pattern[2][0]=48; pattern[2][2]=48; pattern[2][4]=53; pattern[2][6]=55;
    for(c=0;c<8;++c) pattern[3][c]=(c==2||c==6) ? 38:42;
}
static void nextStep(DWORD now)
{
    int r; unsigned duration;
    releaseNotes();
    for(r=0;r<5;++r) if(pattern[r][step] && (r!=4 || pitEnabled)) {
        if(shortMessage(0x90|channels[r],pattern[r][step],r==3?76:96))
            sounding[r]=pattern[r][step];
    }
    duration=(unsigned)(30000UL/tempo); /* eighth notes */
    gate=now+(duration*3U)/4U; due+=duration;
    ++stepsPlayed; status="Playing MIDI"; redraw();
}
static void play(void)
{
    if(!timerReady) return;
    stop(); if(!acquire()) return; step=0; stepsPlayed=0; playing=1; due=GetTickCount(); nextStep(due);
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
    for(r=0;r<5;++r) {
        digit(r==3?8:r,1,18+r*13);
        for(c=0;c<8;++c) {
            x=16+c*16;y=15+r*13;
            for(a=0;a<11;++a){pixel(x+a,y);pixel(x+a,y+10);}
            for(b=1;b<10;++b){pixel(x,y+b);pixel(x+10,y+b);}
            if(pattern[r][c])for(b=3;b<9;++b)for(a=3;a<9;++a)
                pixel(x+a,y+b);
            if(r==selLane && c==selStep)for(a=1;a<10;a+=2) {
                pixel(x+a,y+1);pixel(x+a,y+9);
                pixel(x+1,y+a);pixel(x+9,y+a);
            }
            if(playing && c==step)for(a=0;a<11;++a)pixel(x+a,y+11);
        }
    }
    SetBitmapBits(gridBitmap,1440L,(LPSTR)gridBits);
}

/* Win3.0 has no ES_READONLY. Keep selection/scrolling but reject mutation. */
LONG FAR PASCAL ReadOnlyProc(HWND w,UINT m,WPARAM wp,LPARAM lp)
{
    if(m==WM_CHAR||m==WM_CUT||m==WM_PASTE||m==WM_CLEAR||m==WM_UNDO||
       m==EM_REPLACESEL)return 0;
    if(m==WM_KEYDOWN&&(wp==VK_DELETE||wp==VK_BACK))return 0;
    return CallWindowProc(oldEdit,w,m,wp,lp);
}
static int exportText(void)
{
    if(!BeatExportPit(mmlText,sizeof(mmlText),pattern,tempo,pitEnabled)) {
        status="Export failed";redraw();return 0;
    }
    return 1;
}
static void showView(int view)
{
    int ids[6]={SLOW,FAST,LOWER,HIGHER,DEMO,CLEAR};int i;
    mmlView=view;
    if(view) {exportText();SetWindowText(mmlEdit,mmlText);}
    for(i=0;i<6;i++)ShowWindow(GetDlgItem(mainWindow,ids[i]),view?SW_HIDE:SW_SHOW);
    ShowWindow(mmlEdit,view?SW_SHOW:SW_HIDE);
    ShowWindow(fileEdit,view?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(mainWindow,COPYMML),view?SW_SHOW:SW_HIDE);
    ShowWindow(GetDlgItem(mainWindow,SAVEMML),view?SW_SHOW:SW_HIDE);
    SetDlgItemText(mainWindow,PATVIEW,view?"Grid":"*Grid");
    SetDlgItemText(mainWindow,MMLVIEW,view?"*MML":"MML");
    SetFocus(GetDlgItem(mainWindow,view?MMLVIEW:PATVIEW));
    InvalidateRect(mainWindow,NULL,TRUE);
}
static int copyMML(void)
{
    HANDLE h;LPSTR dst;unsigned i,n;
    if(!exportText())return 0;n=lstrlen(mmlText);
    h=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,n+1);
    if(!h){status="Copy: no memory";redraw();return 0;}
    dst=GlobalLock(h);
    if(!dst){GlobalFree(h);status="Copy: lock failed";redraw();return 0;}
    for(i=0;i<=n;i++)dst[i]=mmlText[i];GlobalUnlock(h);
    if(!OpenClipboard(mainWindow)) {
        GlobalFree(h);status="Clipboard busy";redraw();return 0;
    }
    if(!EmptyClipboard()||!SetClipboardData(CF_TEXT,h)) {
        CloseClipboard();GlobalFree(h);status="Copy failed";redraw();return 0;
    }
    CloseClipboard();status="MML copied";redraw();return 1;
}
static int saveMML(void)
{
    char name[16],path[160];unsigned n,i;int f,ok;
    GetWindowText(fileEdit,name,sizeof(name));n=lstrlen(name);
    if(n<5||n>12||lstrcmpi(name+n-4,".MML"))goto bad;
    for(i=0;i<n;i++)if(name[i]>='a'&&name[i]<='z')name[i]-=32;
    for(i=0;i<n-4;i++)if(!((name[i]>='A'&&name[i]<='Z')||
       (name[i]>='a'&&name[i]<='z')||(name[i]>='0'&&name[i]<='9')||name[i]=='_'))goto bad;
    if(!lstrcmpi(name,"CON.MML")||!lstrcmpi(name,"PRN.MML")||
       !lstrcmpi(name,"AUX.MML")||!lstrcmpi(name,"NUL.MML"))goto bad;
    if(n==8&&(!strncmp(name,"COM",3)||!strncmp(name,"LPT",3)||
       !strncmp(name,"com",3)||!strncmp(name,"lpt",3))&&name[3]>='1'&&name[3]<='9')goto bad;
    if(!exportText())return 0;
    lstrcpy(path,appdir);lstrcat(path,name);
    /* O_EXCL creates atomically: even a race cannot overwrite an existing file. */
    f=open(path,O_WRONLY|O_CREAT|O_EXCL|O_BINARY,S_IREAD|S_IWRITE);
    if(f<0){status="Exists / can't save";redraw();return 0;}
    n=lstrlen(mmlText);ok=write(f,mmlText,n)==(int)n;
    if(close(f))ok=0;
    if(!ok){status="Partial file kept";redraw();return 0;}
    status="Saved beside app";redraw();return 1;
bad:status="Use NAME.MML";redraw();return 0;
}

static void paint(HWND w)
{
    HDC dc;PAINTSTRUCT ps;int v;char line[32];RECT rect;
    dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));
    SetBkMode(dc,OPAQUE);SetBkColor(dc,RGB(255,255,255));
    SetTextColor(dc,RGB(0,0,0));
    if(!mmlView) {
    drawGrid();BitBlt(dc,2,45,144,80,gridDC,0,0,SRCCOPY);
    v=pattern[selLane][selStep];
    if(!v)wsprintf(line,"%u BPM %d:%d -",tempo,selLane+1,selStep+1);
    else if(selLane==3)wsprintf(line,"%u BPM N:%d %d",tempo,selStep+1,v);
    else wsprintf(line,"%u BPM %d:%d %s%d",tempo,selLane+1,selStep+1,
        (LPSTR)notes[v%12],v/12-1);
    TextOut(dc,2,149,line,lstrlen(line));
    }
    rect.left=2;rect.top=165;rect.right=148;rect.bottom=178;
    FillRect(dc,&rect,GetStockObject(WHITE_BRUSH));
    TextOut(dc,2,165,status,lstrlen(status));
    EndPaint(w,&ps);
}
static void button(HWND w,char *label,int x,int y,int width,int id)
{
    CreateWindow("BUTTON",label,WS_CHILD|WS_VISIBLE|WS_TABSTOP,
        x,y,width,18,w,(HMENU)id,instance,NULL);
}
static void mmlTest(DWORD now)
{
    char *copy=testCopy,name[16];HANDLE h;LPSTR text;HFILE f;unsigned n;
    RECT rect,client;POINT pt;int ids[6]={PATVIEW,MMLVIEW,COPYMML,SAVEMML,MMLTEXT,FILENAME};int i;
    if(!testStage&&now-beginTick>400L) {
        showView(1);check("MML view selected",mmlView&&IsWindowVisible(mmlEdit));
        check("export includes noise",strstr(mmlText,"[N]\r\n")!=0);
        GetWindowText(mmlEdit,copy,BEAT_MML_CAP);
        SendMessage(mmlEdit,WM_CHAR,'Z',0L);SendMessage(mmlEdit,WM_PASTE,0,0L);
        SendMessage(mmlEdit,EM_REPLACESEL,0,(LPARAM)(LPSTR)"BAD");
        GetWindowText(mmlEdit,copy,BEAT_MML_CAP);check("generated text read only",!lstrcmp(copy,mmlText));
        check("copy succeeds",copyMML());
        if(OpenClipboard(mainWindow)) {
            h=GetClipboardData(CF_TEXT);text=h?GlobalLock(h):0;
            check("clipboard text exact",text&&!lstrcmp(text,mmlText));
            if(text)GlobalUnlock(h);CloseClipboard();
        }else check("clipboard readable",0);
        SetWindowText(fileEdit,"../BAD.MML");check("reject path",!saveMML());
        SetWindowText(fileEdit,"CoM1.MML");check("reject device",!saveMML());
        SetWindowText(fileEdit,"NATIVE.MML");check("save new file",saveMML());
        check("no overwrite",!saveMML());
        lstrcpy(copy,appdir);lstrcat(copy,"NATIVE.MML");f=_lopen(copy,OF_READ);
        check("saved file readable",f!=HFILE_ERROR);
        if(f!=HFILE_ERROR) {n=_lread(f,copy,BEAT_MML_CAP-1);_lclose(f);
            if(n>=BEAT_MML_CAP)check("saved bytes readable",0);
            else {copy[n]=0;check("saved bytes exact",!lstrcmp(copy,mmlText));}}
        for(i=40;i<=240;i+=10) {tempo=i;wsprintf(name,"B%03d.MML",i);SetWindowText(fileEdit,name);
            check("tempo export saved",saveMML());}
        tempo=120;exportText();GetClientRect(mainWindow,&client);
        for(i=0;i<6;i++) {GetWindowRect(GetDlgItem(mainWindow,ids[i]),&rect);
            pt.x=rect.right;pt.y=rect.bottom;ScreenToClient(mainWindow,&pt);
            check("MML control fits",pt.x<=client.right&&pt.y<=client.bottom);}
        play();showView(0);check("view switch preserves playback",playing&&opened);
        showView(1);check("MML view keeps playback",playing&&opened);
        testStage=1;ended=now;
    } else if(testStage==1&&now-ended>700L) {
        stop();check("export play stop releases sound",!opened&&debug&&debug(1)==0);
        showView(0);SendMessage(mainWindow,WM_COMMAND,CLEAR,0L);showView(1);
        check("empty pattern exports",strstr(mmlText,"R8 R8 R8 R8")!=0);
        showView(0);preset();showView(1);check("demo noise restored",strstr(mmlText,"N38/16.")!=0);
        testStage=2;ended=now;
    } else if(testStage==2&&now-ended>700L)PostMessage(mainWindow,WM_CLOSE,0,0L);
}
static void pitTestTick(DWORD now)
{
    int before;unsigned i;
    if(!testStage && now-beginTick>500UL){
        if(automatic!=9)pitEnabled=1;
        pattern[4][0]=72;pattern[4][1]=72;
        SetDlgItemText(mainWindow,PIT,pitEnabled?"*PIT":"PIT");
        check("MML export",exportText());
        check("explicit version",!strncmp(mmlText,pitEnabled?"MML3":"MML2",4));
        check("P opt-in section",!!strstr(mmlText,"[P]")==!!pitEnabled);
        if(automatic==8){
            SetWindowText(fileEdit,"PIT.MML");
            check("MML3 native save",saveMML());
            check("MML3 native copy",copyMML());
        }
        if(automatic==9){
            for(i=0;i<8;++i)pattern[4][i]=0;
            pitEnabled=1;check("all rest P export",exportText()&&strstr(mmlText,"[P]"));
            pitEnabled=0;testStage=2;return;
        }
        before=debug?debug(2):0;
        if(automatic==12){
            check("P busy guard lease",OpenSound()>=0);play();
            check("P busy refusal",!playing&&!opened);
            check("P busy no PSG writes",debug&&debug(2)==before);
            CloseSound();testStage=2;return;
        }
        play();
        if(automatic==10){
            check("P missing capability refuses",!playing&&!opened);
            check("P refusal no PSG writes",debug&&debug(2)==before);
            testStage=2;return;
        }
        check("all five generators",playing&&debug&&debug(0)==31);
        play();check("P replay",playing&&debug&&debug(0)==31);
        testStage=1;return;
    }
    if(testStage==1 && now-beginTick>750UL){
        if(automatic==13){PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=3;return;}
        if(automatic==11)SendMessage(mainWindow,WM_ACTIVATE,0,0L);
        else SendMessage(mainWindow,WM_KEYDOWN,VK_ESCAPE,0L);
        check("P cleanup",!playing&&!opened&&debug&&debug(0)==0);
        stop();check("P cleanup owner",debug&&debug(1)==0);
        testStage=2;return;
    }
    if(testStage==2)PostMessage(mainWindow,WM_CLOSE,0,0L);
}
static void testTick(DWORD now)
{
    if(automatic>=8){pitTestTick(now);return;}
    if(automatic==5) {mmlTest(now);return;}
    if(automatic==6) {
        if(!testStage&&now-beginTick>400L) {showView(1);testStage=1;ended=now;}
        else if(testStage==1&&now-ended>5000L) {
            check("steady MML view",mmlView&&IsWindowVisible(mmlEdit));
            check("generated score visible text",strstr(mmlText,"O4 C16.")!=0);
            PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=2;
        }
        return;
    }
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
        check("stop releases mapper",!opened && debug && debug(1)==0);
        check("stopped editor releases SOUND",OpenSound()>=0);
        CloseSound();
        SendMessage(mainWindow,WM_COMMAND,PLAY,0L);
        check("play reacquires after stop",playing && opened);
        SendMessage(mainWindow,WM_COMMAND,STOP,0L);
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
        button(w,"Grid",2,3,48,PATVIEW);
        button(w,"MML",52,3,44,MMLVIEW);
        button(w,"PIT",98,3,48,PIT);
        button(w,"Play",2,24,34,PLAY); button(w,"Stop",38,24,34,STOP);
        button(w,"Demo",74,24,34,DEMO);button(w,"Clear",110,24,36,CLEAR);
        button(w,"-",104,128,20,SLOW); button(w,"+",126,128,20,FAST);
        button(w,"Pitch-",2,128,48,LOWER);
        button(w,"Pitch+",52,128,48,HIGHER);
        button(w,"Copy",2,128,68,COPYMML);button(w,"Save",76,128,70,SAVEMML);
        mmlEdit=CreateWindow("EDIT","",WS_CHILD|WS_BORDER|WS_TABSTOP|WS_VSCROLL|
            ES_MULTILINE|ES_AUTOVSCROLL,2,45,144,80,w,(HMENU)MMLTEXT,instance,NULL);
        fileEdit=CreateWindow("EDIT","BEAT.MML",WS_CHILD|WS_BORDER|WS_TABSTOP|
            ES_AUTOHSCROLL,2,149,144,16,w,(HMENU)FILENAME,instance,NULL);
        if(!mmlEdit||!fileEdit)return -1;
        SendMessage(fileEdit,EM_LIMITTEXT,12,0L);
        editThunk=MakeProcInstance((FARPROC)ReadOnlyProc,instance);
        if(!editThunk)return -1;
        oldEdit=(FARPROC)SetWindowLong(mmlEdit,GWL_WNDPROC,(LONG)editThunk);
        showView(0);
        timerReady=SetTimer(w,PULSE,30,NULL)!=0;
        if(!timerReady) { status="Timer failed"; ++failures; return -1; }
        beginTick=GetTickCount();
        if(automatic==1 || automatic==3) PostMessage(w,WM_COMMAND,PLAY,0L);
        return 0;
    case WM_COMMAND:
        if(wp==PIT){stop();pitEnabled=!pitEnabled;
            SetDlgItemText(w,PIT,pitEnabled?"*PIT":"PIT");
            status=pitEnabled?"PIT required":"PIT off";redraw();}
        if(wp==PATVIEW)showView(0);
        if(wp==MMLVIEW)showView(1);
        if(wp==COPYMML)copyMML();
        if(wp==SAVEMML)saveMML();
        if(wp==PLAY) play();
        if(wp==STOP) stop();
        if(wp==SLOW && tempo>40) { tempo-=10; redraw(); }
        if(wp==FAST && tempo<240) { tempo+=10; redraw(); }
        if(wp==LOWER) editPitch(-1);
        if(wp==HIGHER) editPitch(1);
        if(wp==DEMO) { stop(); preset(); redraw(); }
        if(wp==CLEAR) { stop(); for(r=0;r<5;++r)
            for(c=0;c<8;++c)pattern[r][c]=0; redraw(); }
        return 0;
    case WM_LBUTTONDOWN:
        x=LOWORD(lp); y=HIWORD(lp);
        if(!mmlView && x>=18 && x<146 && y>=60 && y<125) {
            selStep=(x-18)/16; selLane=(y-60)/13; toggle(); SetFocus(w);
        }
        return 0;
    case WM_KEYDOWN:
        if(mmlView) {if(wp==VK_ESCAPE)stop();return 0;}
        if(wp==VK_LEFT) selStep=(selStep+7)%8;
        if(wp==VK_RIGHT) selStep=(selStep+1)%8;
        if(wp==VK_UP) selLane=(selLane+4)%5;
        if(wp==VK_DOWN) selLane=(selLane+1)%5;
        if(wp==VK_SPACE) toggle();
        if(wp==VK_ESCAPE || wp=='S') stop();
        if(wp=='P') play();
        if(wp==VK_ADD || wp==0xbb) editPitch(1);
        if(wp==VK_SUBTRACT || wp==0xbd) editPitch(-1);
        redraw(); return 0;
    case WM_TIMER:
        if(wp!=PULSE) return 0;
        if(opened)message(MODM_SYNTHUPDATE,0L);
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
    case WM_ACTIVATE: if(pitEnabled && LOWORD(wp)==0)stop(); return 0;
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
    if(previous) return 1;
    if(GetWinFlags()&WF_PMODE) return 2;
    instance=inst;
    if(lstrcmp(cmd,"/pittest")==0)automatic=8;
    if(lstrcmp(cmd,"/pitoff")==0)automatic=9;
    if(lstrcmp(cmd,"/pitold")==0)automatic=10;
    if(lstrcmp(cmd,"/pitfocus")==0)automatic=11;
    if(lstrcmp(cmd,"/pitbusy")==0)automatic=12;
    if(lstrcmp(cmd,"/pitclose")==0)automatic=13;
    if(lstrcmp(cmd,"/test")==0)automatic=1;
    if(lstrcmp(cmd,"/uitest")==0)automatic=2;
    if(lstrcmp(cmd,"/closetest")==0)automatic=3;
    if(lstrcmp(cmd,"/mmltest")==0)automatic=5;
    if(lstrcmp(cmd,"/viewtest")==0)automatic=6;
    if(automatic)logFile=_lcreat("C:\\BEATS.LOG",0);
    GetModuleFileName(inst,path,sizeof(path));
    for(i=lstrlen(path)-1;i>=0;--i) if(path[i]=='\\')break;
    lstrcpy(path+i+1,"MIDIMAP.DRV");
    lstrcpy(appdir,path);appdir[i+1]=0;
    module=LoadLibrary(path);
    if(module>=32) {
        midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
        debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    }
    /* Loading an editor does not reserve hardware. Play acquires it. */
    if(!midi) status="Driver missing";
    preset();
    wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;
    wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;
    wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=NULL;wc.lpszClassName="TandyBeatsLab";
    if(!RegisterClass(&wc)) {if(module>=32)FreeLibrary(module);return 4;}
    w=CreateWindow("TandyBeatsLab","Tandy Beats",WS_OVERLAPPED|WS_CAPTION|
        WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,
        NULL,NULL,inst,NULL);
    if(!w) {if(module>=32)FreeLibrary(module);return 5;}
    ShowWindow(w,show);UpdateWindow(w);
    while(GetMessage(&msg,NULL,0,0)) {
        if(msg.message==WM_KEYDOWN) {
            if(msg.wParam==VK_TAB) { /* basic tab cycle for native controls */
                HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);
                if(next)SetFocus(next); continue;
            }
            if(msg.wParam==VK_ESCAPE || (!mmlView && (msg.wParam=='P' || msg.wParam=='S' ||
               msg.wParam==VK_LEFT || msg.wParam==VK_RIGHT ||
               msg.wParam==VK_UP || msg.wParam==VK_DOWN ||
               msg.wParam==VK_ADD || msg.wParam==VK_SUBTRACT ||
               msg.wParam==0xbb || msg.wParam==0xbd ||
               (msg.wParam==VK_SPACE && msg.hwnd==w)))) {
                SendMessage(w,WM_KEYDOWN,msg.wParam,msg.lParam);continue;
            }
        }
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
    if(editThunk)FreeProcInstance(editThunk);
    stop();check("close mutes",debug && debug(0)==0);
    check("close releases lease",!opened);
    check("driver releases owner",debug && debug(1)==0);
    if(module>=32)FreeLibrary(module);
    wsprintf(line,"TOTAL_FAILURES=%d STEPS=%d",failures,stepsPlayed);logText(line);
    if(logFile!=HFILE_ERROR)_lclose(logFile);
    if(automatic)ExitWindows(0L,0);
    return failures?6:0;
}

