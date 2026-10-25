/* Tandy Beats Lab. Win3.0 / 8088 foreground editor; existing mapper only. */
#define WINVER 0x0300
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "MIDIAPI.H"
#include "BEATPAT.H"
#define PLAY 101
#define STOP 102
#define DEMO 103
#define PIT 104
#define VIEW 105
#define ABOUT 106
#define FILEEDIT 107
#define LOAD 108
#define SAVE 109
#define EXPORT 110
#define COPY 111
#define MMLTEXT 112
#define SLOW 113
#define FAST 114
#define CLEAR 115
#define LOWER 116
#define HIGHER 117
#define LANE0 120
#define PULSE 1
static HINSTANCE instance,module;
static HWND mainWindow,fileEdit,mmlEdit;
static MIDIPROC midi;
static DEBUGPROC debug;
static FARPROC editThunk,oldEdit;
static BPAT pat;
static WORD sounding[5];
static BYTE channel[5]={0,1,2,9,15};
static BYTE defaultNote[5]={60,64,48,42,33};
static char *noteNames[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static char appdir[144],mml[BP_MML_CAP],status[64]="DAWs With Attitude: Play / Demo";
static char patternName[32]="Starter";
static int opened,playing,timerReady,step,selLane,selStep,mmlView,dirty;
static unsigned demoIndex;
static DWORD due,gate;
static int automatic,testStage,failures,testAttackFailure,testServiceFailure,visualMode;
static DWORD testAt;
static HFILE logFile=HFILE_ERROR;
static void redraw(void){if(mainWindow)InvalidateRect(mainWindow,NULL,FALSE);}
static void say(const char *s){strncpy(status,s,sizeof(status)-1);status[sizeof(status)-1]=0;redraw();}
static void check(const char *s,int okay)
{
    char line[120];if(!okay)++failures;
    if(logFile!=HFILE_ERROR){wsprintf(line,"%s %s\r\n",(LPSTR)(okay?"PASS":"FAIL"),(LPSTR)s);_lwrite(logFile,line,lstrlen(line));}
}
static DWORD message(WORD m,DWORD value)
{
    if(automatic&&testServiceFailure&&m==MODM_SYNTHUPDATE)return MMSYSERR_ERROR;
    return midi?midi(0,m,MIDI_COOKIE,value,0L):MMSYSERR_ERROR;
}
static int send(unsigned s,unsigned a,unsigned b)
{
    if(automatic&&testAttackFailure&&(s&0xf0)==0x90)return 0;
    return message(MODM_DATA,(DWORD)s|((DWORD)a<<8)|((DWORD)b<<16))==0;
}
static void releaseNotes(void)
{
    int i;for(i=0;i<5;++i)if(sounding[i]){
        send(0x80|channel[i],sounding[i],0);sounding[i]=0;
    }
}
static void stop(void)
{
    playing=0;
    if(opened){releaseNotes();message(MODM_RESET,0L);message(MODM_CLOSE,0L);opened=0;}
    say("Stopped: sound released");
}
static void fail(const char *s){stop();say(s);}
static void labels(void)
{
    char text[64];unsigned r;
    for(r=0;r<3;++r){wsprintf(text,"%u %s",r+1,(LPSTR)BPPreset(pat.program[r]));SetDlgItemText(mainWindow,LANE0+r,text);}
    SetDlgItemText(mainWindow,PIT,pat.pit?"PIT: on":"PIT: off");
    SetDlgItemText(mainWindow,VIEW,mmlView?"Grid":"MML");
    if(mmlView&&mmlEdit){BPMML(&pat,mml,sizeof(mml));SetWindowText(mmlEdit,mml);}
    wsprintf(text,"Tandy Beats Lab - %s%s",(LPSTR)patternName,(LPSTR)(dirty?" *":""));SetWindowText(mainWindow,text);
}
static void changed(void)
{
    dirty=1;labels();redraw();
}
static int acquire(void)
{
    static TOPEN desc;static DWORD cookie;static TCAPS caps;DWORD result;unsigned r;
    if(!midi){say("Mapper missing: extract complete kit");return 0;}
    if(message(MODM_GETNUMDEVS,0L)!=1L||midi(0,MODM_GETDEVCAPS,0L,
        (DWORD)(LPVOID)&caps,sizeof(caps))||caps.wVoices!=3||caps.vDriverVersion<0x0102){
        say("Wrong mapper: use complete Beats kit");return 0;
    }
    if(pat.pit&&caps.vDriverVersion<PIT_LOW_VERSION)
        for(r=0;r<8;++r)if(pat.note[4][r]&&pat.note[4][r]<45){
            say("Low bass needs current complete kit");return 0;
        }
    if(pat.pit&&!GetProcAddress(GetModuleHandle("SOUND"),"TANDYPITVOICE")){
        say("PIT unavailable: run kit Setup in DOS");return 0;
    }
    desc.hMidi=0;desc.dwCallback=desc.dwInstance=0;cookie=0;
    result=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L);
    if(result){say(result==MMSYSERR_ALLOCATED?"Sound busy: stop the other player":"Open failed: run kit Setup in DOS");return 0;}
    opened=1;
    if(cookie!=MIDI_COOKIE||message(MODM_SYNTHCONFIG,3L)){
        fail("Mapper mismatch: use complete kit");return 0;
    }
    if(pat.pit&&midi(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1L,1L)){
        fail("PIT busy or unavailable");return 0;
    }
    for(r=0;r<3;++r)if(!send(0xc0|channel[r],pat.program[r],0)){
        fail("Preset failed: playback stopped");return 0;
    }
    return 1;
}
static int nextStep(DWORD now)
{
    unsigned r,duration;releaseNotes();
    for(r=0;r<5;++r)if(pat.note[r][step]&&(r!=4||pat.pit)){
        if(!send(0x90|channel[r],pat.note[r][step],r==3?76:96)){
            fail("Note failed: playback stopped");return 0;
        }
        sounding[r]=pat.note[r][step];
    }
    duration=(unsigned)(30000UL/pat.tempo);
    gate=now+((DWORD)duration*3U)/4U;due+=duration;redraw();return 1;
}
static void play(void)
{
    stop();if(!timerReady){say("Timer failed: close and reopen");return;}
    if(!acquire())return;
    step=0;playing=1;due=GetTickCount();
    if(nextStep(due))say(pat.pit?"Playing + PC speaker; F5 stops":"Playing PSG voices; F5 stops");
}
static int discard(void)
{
    return !dirty||automatic||MessageBox(mainWindow,"Discard unsaved pattern changes?",
        "Tandy Beats Lab",MB_YESNO|MB_ICONQUESTION)==IDYES;
}
static void demo(void)
{
    if(!discard())return;stop();demoIndex=(demoIndex+1)%BP_DEMOS;
    lstrcpy(patternName,BPDemo(&pat,demoIndex));
    SetWindowText(fileEdit,demoIndex==1?"POCKET.BTS":demoIndex==2?"LOWRIDER.BTS":demoIndex==3?"FIVE.BTS":demoIndex==4?"NIGHT.BTS":"STARTER.BTS");dirty=0;labels();
    say(pat.pit?"Demo loaded: PC speaker ON":"Demo loaded: PSG voices only");
}
static void editPitch(int delta)
{
    int n=pat.note[selLane][selStep];
    if(!n)n=defaultNote[selLane];else n+=delta;
    if(n<(selLane==3?35:selLane==4?26:45))n=selLane==3?35:selLane==4?26:45;
    if(n>(selLane==3?81:96))n=selLane==3?81:96;
    pat.note[selLane][selStep]=(BYTE)n;changed();
}
static void toggle(void)
{
    pat.note[selLane][selStep]=pat.note[selLane][selStep]?0:defaultNote[selLane];changed();
}
static int filename(char *name,char *path,const char *ext)
{
    GetWindowText(fileEdit,name,13);
    if(!BPName(name,ext)){say("Use a name like GROOVE.BTS");return 0;}
    if(lstrlen(appdir)+lstrlen(name)>=144){say("Folder path too long: move kit");return 0;}
    lstrcpy(path,appdir);lstrcat(path,name);return 1;
}
static void fileError(int r)
{
    if(r==BP_FORMAT)say("Invalid pattern: current edit kept");
    else if(r==BP_OPEN)say("Cannot open: check name and disk");
    else if(r==BP_EXISTS)say("Temp file exists: use a new name");
    else if(r==BP_RECOVER)say("Save failed: old pattern is in .BAK");
    else say("Disk error: check space/write access");
}
static int savePattern(void)
{
    char name[13],path[144];int r,replaced;
    if(!filename(name,path,"BTS"))return 0;
    r=BPSave(&pat,path,0);replaced=r==BP_EXISTS;
    if(r==BP_EXISTS){
        if(!automatic&&MessageBox(mainWindow,"Replace this pattern? The previous file becomes .BAK.",
            "Save pattern",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;
        r=BPSave(&pat,path,1);
    }
    if(r){fileError(r);return 0;}
    lstrcpy(patternName,name);dirty=0;labels();say(replaced?"Saved; previous pattern is in .BAK":"Pattern saved beside app");return 1;
}
static int loadPattern(void)
{
    char name[13],path[144];BPAT loaded;int r;
    if(!filename(name,path,"BTS")||!discard())return 0;
    r=BPLoad(&loaded,path);if(r){fileError(r);return 0;}
    stop();pat=loaded;dirty=0;lstrcpy(patternName,name);labels();say(pat.pit?"Loaded: PC speaker ON":"Loaded: PSG voices only");return 1;
}
static int exportMML(void)
{
    char name[13],path[144];unsigned n;int f,bad;
    if(!filename(name,path,"BTS"))return 0;
    lstrcpy(path+lstrlen(path)-3,"MML");n=BPMML(&pat,mml,sizeof(mml));
    if(!n){say("Export failed: pattern unchanged");return 0;}
    f=open(path,O_WRONLY|O_CREAT|O_EXCL|O_BINARY,S_IREAD|S_IWRITE);
    if(f<0){say("MML exists/disk error: change name");return 0;}
    bad=write(f,mml,n)!=(int)n;if(close(f))bad=1;
    say(bad?"MML write failed: check disk":"MML saved beside app (.MML)");return !bad;
}
static int copyMML(void)
{
    HANDLE h;LPSTR dst;unsigned i,n=BPMML(&pat,mml,sizeof(mml));
    if(!n){say("Cannot export this pattern");return 0;}
    h=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,n+1);
    if(!h){say("Clipboard: not enough memory");return 0;}
    dst=GlobalLock(h);if(!dst){GlobalFree(h);say("Clipboard memory unavailable");return 0;}
    for(i=0;i<=n;++i)dst[i]=mml[i];GlobalUnlock(h);
    if(!OpenClipboard(mainWindow)){GlobalFree(h);say("Clipboard busy: try Copy again");return 0;}
    EmptyClipboard();
    if(!SetClipboardData(CF_TEXT,h)){CloseClipboard();GlobalFree(h);say("Clipboard copy failed");return 0;}
    CloseClipboard();say("MML copied to clipboard");return 1;
}
LONG FAR PASCAL ReadOnlyProc(HWND w,UINT m,WPARAM a,LPARAM b)
{
    if(m==WM_CHAR||m==WM_PASTE||m==WM_CLEAR||m==WM_CUT||m==WM_UNDO||m==EM_REPLACESEL)return 0;
    if(m==WM_KEYDOWN&&(a==VK_DELETE||a==VK_BACK))return 0;
    return CallWindowProc(oldEdit,w,m,a,b);
}
static void view(void)
{
    unsigned r;mmlView=!mmlView;
    if(mmlView){BPMML(&pat,mml,sizeof(mml));SetWindowText(mmlEdit,mml);}
    ShowWindow(mmlEdit,mmlView?SW_SHOW:SW_HIDE);
    for(r=0;r<5;++r)ShowWindow(GetDlgItem(mainWindow,LANE0+r),mmlView?SW_HIDE:SW_SHOW);
    ShowWindow(GetDlgItem(mainWindow,CLEAR),mmlView?SW_HIDE:SW_SHOW);
    ShowWindow(GetDlgItem(mainWindow,SLOW),mmlView?SW_HIDE:SW_SHOW);ShowWindow(GetDlgItem(mainWindow,FAST),mmlView?SW_HIDE:SW_SHOW);
    ShowWindow(GetDlgItem(mainWindow,LOWER),mmlView?SW_HIDE:SW_SHOW);ShowWindow(GetDlgItem(mainWindow,HIGHER),mmlView?SW_HIDE:SW_SHOW);
    labels();SetFocus(mainWindow);redraw();
}
static void about(void)
{
    MessageBox(mainWindow,"DWA: DAWs With\nAttitude\nStraight Outta\nLLMompton\nProduced by\nDr. DRAM\n\nF5: Play / Stop\nCtrl+O/S: files",
        "Tandy Beats Lab",MB_OK);SetFocus(mainWindow);
}
static void paint(HWND w)
{
    HDC dc;PAINTSTRUCT ps;RECT box;unsigned r,c,n;char text[80];int x,y;
    dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));
    SelectObject(dc,GetStockObject(NULL_BRUSH));SetBkMode(dc,OPAQUE);SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));
    if(!mmlView){
        box.left=62;box.top=44;box.right=302;box.bottom=141;FillRect(dc,&box,GetStockObject(WHITE_BRUSH));
        for(c=0;c<8;++c){wsprintf(text,"%u",c+1);TextOut(dc,74+c*29,46,text,1);}
        for(r=0;r<5;++r)for(c=0;c<8;++c){
            x=64+c*29;y=59+r*16;box.left=x;box.top=y;box.right=x+27;box.bottom=y+14;
            FillRect(dc,&box,GetStockObject(r==selLane&&c==selStep?BLACK_BRUSH:WHITE_BRUSH));
            Rectangle(dc,x,y,x+27,y+14);
            SetBkColor(dc,r==selLane&&c==selStep?RGB(0,0,0):RGB(255,255,255));
            SetTextColor(dc,r==selLane&&c==selStep?RGB(255,255,255):RGB(0,0,0));
            n=pat.note[r][c];if(!n)lstrcpy(text," -");
            else if(r==3)wsprintf(text,"%u",n);
            else wsprintf(text,"%s%u",(LPSTR)noteNames[n%12],n/12-1);
            TextOut(dc,x+2,y+3,text,lstrlen(text));
            if(playing&&c==(unsigned)step){MoveTo(dc,x,y+14);LineTo(dc,x+26,y+14);}
        }
        SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));
        wsprintf(text,"%u BPM",pat.tempo);
    }else lstrcpy(text,"MML: Export saves; Copy copies");
    box.left=mmlView?2:64;box.top=141;box.right=302;box.bottom=169;FillRect(dc,&box,GetStockObject(WHITE_BRUSH));
    TextOut(dc,mmlView?2:184,141,text,lstrlen(text));
    box.left=2;box.top=157;FillRect(dc,&box,GetStockObject(WHITE_BRUSH));TextOut(dc,2,157,status,lstrlen(status));
    EndPaint(w,&ps);
}
static void button(HWND w,const char *s,int x,int y,int width,int id)
{
    CreateWindow("BUTTON",s,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,width,18,w,(HMENU)id,instance,NULL);
}
static void nativeTest(DWORD now)
{
    static BPAT saved;unsigned i;RECT client,control;POINT pt;
    if(automatic==2){
        char title[80];
        GetClientRect(mainWindow,&client);GetWindowText(mainWindow,title,sizeof(title));
        check("primary product name",!strncmp(title,"Tandy Beats Lab",15));
        check("secondary label",!strncmp(status,"DAWs With Attitude",18));
        for(i=LOWER;i<=HIGHER;++i){GetWindowRect(GetDlgItem(mainWindow,i),&control);pt.x=control.right;pt.y=control.bottom;ScreenToClient(mainWindow,&pt);check("mouse pitch control fits",pt.x<=client.right&&pt.y<=client.bottom&&IsWindowVisible(GetDlgItem(mainWindow,i)));}
        selLane=0;selStep=0;SendMessage(mainWindow,WM_COMMAND,HIGHER,0L);check("mouse pitch plus",pat.note[0][0]==61);
        SendMessage(mainWindow,WM_COMMAND,LOWER,0L);check("mouse pitch minus",pat.note[0][0]==60);
        view();check("MML hides pitch controls",!IsWindowVisible(GetDlgItem(mainWindow,LOWER))&&!IsWindowVisible(GetDlgItem(mainWindow,HIGHER)));
        view();check("grid restores pitch controls",IsWindowVisible(GetDlgItem(mainWindow,LOWER))&&IsWindowVisible(GetDlgItem(mainWindow,HIGHER)));
        dirty=0;PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=3;automatic=3;return;
    }
    if(automatic==3)return;
    if(!testStage){
        GetClientRect(mainWindow,&client);check("client fits 320x200",client.right>=302&&client.bottom>=168&&GetSystemMetrics(SM_CXSCREEN)>=312);
        for(i=PLAY;i<=COPY;++i){GetWindowRect(GetDlgItem(mainWindow,i),&control);pt.x=control.right;pt.y=control.bottom;ScreenToClient(mainWindow,&pt);check("toolbar fits",pt.x<=client.right&&pt.y<=client.bottom);}
        check("five-lane default",BPValid(&pat)&&pat.note[3][7]==42);
        for(i=0;i<BP_DEMOS;++i){BPDemo(&pat,i);check("editable demo valid",BPValid(&pat));}
        BPDemo(&pat,3);labels();saved=pat;
        SetWindowText(fileEdit,"NATIVE.BTS");check("save pattern",savePattern());
        pat.note[4][0]=34;dirty=0;check("load pattern",loadPattern());
        check("load restores fifth lane",!memcmp(&saved,&pat,sizeof(pat)));
        check("MML export",exportMML());check("clipboard export",copyMML());
        view();check("MML visible",mmlView&&IsWindowVisible(mmlEdit));view();
        selLane=4;selStep=0;editPitch(1);check("fifth edit",pat.note[4][0]==34);toggle();check("fifth rest",pat.note[4][0]==0);toggle();
        dirty=0;BPDemo(&pat,3);labels();
        selLane=0;selStep=0;SendMessage(mainWindow,WM_KEYDOWN,VK_LEFT,0L);check("keyboard wraps step",selStep==7);
        SendMessage(mainWindow,WM_KEYDOWN,VK_UP,0L);check("keyboard reaches PC speaker",selLane==4);
        SendMessage(mainWindow,WM_LBUTTONDOWN,0,MAKELONG(75,130));check("mouse edits PC speaker",pat.note[4][0]==0);
        SendMessage(mainWindow,WM_LBUTTONDOWN,0,MAKELONG(75,130));check("mouse restores PC speaker",pat.note[4][0]==33);
        BPDemo(&pat,3);dirty=0;play();check("Play starts",playing&&opened);
        check("all five voices",debug&&debug(0)==31);
        check("low speaker bass without folding",debug&&debug(20)==33&&debug(21)==55);testAt=now;testStage=1;
    }else if(testStage==1&&now-testAt>1800UL){
        stop();check("Stop mutes",debug&&debug(0)==0);check("Stop releases owner",!opened&&debug&&debug(1)==0);
        play();check("replay reacquires",playing&&opened);testAt=now;testStage=2;
    }else if(testStage==2&&now-testAt>500UL){
        SendMessage(mainWindow,WM_ACTIVATE,0,0);check("deactivate stops",!playing&&!opened&&debug&&debug(0)==0);
        testAttackFailure=1;play();check("attack failure stops/releases",!playing&&!opened&&debug&&debug(1)==0);testAttackFailure=0;
        play();testServiceFailure=1;SendMessage(mainWindow,WM_TIMER,PULSE,0L);
        check("service failure stops/releases",!playing&&!opened&&debug&&debug(1)==0);testServiceFailure=0;
        play();check("close during play",playing);dirty=0;PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=3;
    }
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp)
{
    DWORD now;int x,y;unsigned r;static const char *lanes[5]={"1 KEYS","2 KEYS","3 KEYS","4 Noise","PCspkr"};
    switch(m){
    case WM_CREATE:
        mainWindow=w;
        button(w,"Play",2,2,40,PLAY);button(w,"Stop",44,2,40,STOP);button(w,"Demo",86,2,44,DEMO);
        button(w,"PIT: off",132,2,76,PIT);button(w,"MML",210,2,44,VIEW);button(w,"About",256,2,46,ABOUT);
        fileEdit=CreateWindow("EDIT","PATTERN.BTS",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,2,23,110,18,w,(HMENU)FILEEDIT,instance,NULL);
        SendMessage(fileEdit,EM_LIMITTEXT,12,0L);
        button(w,"Load",116,23,42,LOAD);button(w,"Save",162,23,42,SAVE);button(w,"Export",208,23,46,EXPORT);button(w,"Copy",258,23,44,COPY);
        button(w,"-",2,42,28,SLOW);button(w,"+",32,42,28,FAST);button(w,"Clear",2,139,59,CLEAR);
        button(w,"Pitch-",64,139,56,LOWER);button(w,"Pitch+",124,139,56,HIGHER);
        for(r=0;r<5;++r)CreateWindow("BUTTON",lanes[r],WS_CHILD|WS_VISIBLE|WS_TABSTOP,2,59+r*16,59,14,w,(HMENU)(LANE0+r),instance,NULL);
        mmlEdit=CreateWindow("EDIT","",WS_CHILD|WS_BORDER|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,2,45,300,94,w,(HMENU)MMLTEXT,instance,NULL);
        if(!fileEdit||!mmlEdit)return -1;
        editThunk=MakeProcInstance((FARPROC)ReadOnlyProc,instance);if(!editThunk)return -1;
        oldEdit=(FARPROC)SetWindowLong(mmlEdit,GWL_WNDPROC,(LONG)editThunk);
        timerReady=SetTimer(w,PULSE,30,NULL)!=0;if(!timerReady)return -1;
        labels();return 0;
    case WM_COMMAND:
        if(wp==PLAY)play();else if(wp==STOP)stop();else if(wp==DEMO)demo();
        else if(wp==PIT){stop();pat.pit=!pat.pit;changed();say(pat.pit?"PC speaker enabled for lane 5":"Lane 5 muted: PIT is off");}
        else if(wp==VIEW)view();else if(wp==ABOUT)about();else if(wp==LOAD)loadPattern();else if(wp==SAVE)savePattern();
        else if(wp==LOWER)editPitch(-1);else if(wp==HIGHER)editPitch(1);
        else if(wp==SLOW&&pat.tempo>40){pat.tempo=pat.tempo<50?40:pat.tempo-10;changed();}
        else if(wp==FAST&&pat.tempo<240){pat.tempo+=10;if(pat.tempo>240)pat.tempo=240;changed();}
        else if(wp==CLEAR&&discard()){stop();memset(pat.note,0,sizeof(pat.note));changed();say("Cleared: click cells to add notes");}
        else if(wp==EXPORT)exportMML();else if(wp==COPY)copyMML();
        else if(wp>=LANE0&&wp<LANE0+3){stop();r=wp-LANE0;pat.program[r]=(pat.program[r]+16)&127;changed();}
        else if(wp==LANE0+4)SendMessage(w,WM_COMMAND,PIT,0L);
        if((wp>=PLAY&&wp<=HIGHER&&wp!=FILEEDIT&&wp!=MMLTEXT&&wp!=VIEW&&wp!=ABOUT)||
           (wp>=LANE0&&wp<LANE0+5))SetFocus(w);return 0;
    case WM_LBUTTONDOWN:
        x=LOWORD(lp);y=HIWORD(lp);if(!mmlView&&x>=64&&x<296&&y>=59&&y<139){selStep=(x-64)/29;selLane=(y-59)/16;toggle();SetFocus(w);}return 0;
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE){stop();return 0;}
        if(wp==VK_F5){if(playing)stop();else play();return 0;}
        if(wp==VK_F1){about();return 0;}
        if(GetKeyState(VK_CONTROL)<0){if(wp=='S')savePattern();if(wp=='O')loadPattern();return 0;}
        if(wp=='P'){play();return 0;}if(wp=='S'){stop();return 0;}
        if(wp=='D'){demo();return 0;}if(wp=='V'){view();return 0;}
        if(mmlView)return 0;
        if(wp==VK_LEFT)selStep=(selStep+7)%8;else if(wp==VK_RIGHT)selStep=(selStep+1)%8;
        else if(wp==VK_UP)selLane=(selLane+4)%5;else if(wp==VK_DOWN)selLane=(selLane+1)%5;
        else if(wp==VK_RETURN||wp==VK_SPACE)toggle();else if(wp==VK_ADD||wp==0xbb)editPitch(1);else if(wp==VK_SUBTRACT||wp==0xbd)editPitch(-1);
        else if(wp==VK_PRIOR&&pat.tempo<240){pat.tempo+=10;if(pat.tempo>240)pat.tempo=240;changed();}
        else if(wp==VK_NEXT&&pat.tempo>40){pat.tempo=pat.tempo<50?40:pat.tempo-10;changed();}
        else if(wp==VK_DELETE){pat.note[selLane][selStep]=0;changed();}
        redraw();return 0;
    case WM_TIMER:
        if(wp!=PULSE)return 0;
        if(opened&&message(MODM_SYNTHUPDATE,0L)){fail("Synth failed: playback stopped");return 0;}
        now=GetTickCount();
        if(playing){if((LONG)(now-gate)>=0)releaseNotes();if((LONG)(now-due)>=0){if(now-due>1000UL)due=now;step=(step+1)%8;nextStep(now);}}
        if(automatic)nativeTest(now);return 0;
    case WM_ACTIVATE:if(!LOWORD(wp)&&playing)stop();return 0;
    case WM_CANCELMODE:stop();return 0;
    case WM_PAINT:paint(w);return 0;
    case WM_QUERYENDSESSION:stop();return discard()?TRUE:FALSE;
    case WM_ENDSESSION:if(wp)stop();return 0;
    case WM_CLOSE:if(!discard())return 0;stop();DestroyWindow(w);return 0;
    case WM_DESTROY:KillTimer(w,PULSE);timerReady=0;stop();PostQuitMessage(0);return 0;
    }
    return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show)
{
    WNDCLASS wc;HWND w;MSG msg;int i;unsigned n;char path[144],line[64];
    if(previous){HWND old=FindWindow("DWABeats",NULL);if(old){ShowWindow(old,SW_RESTORE);SetActiveWindow(old);}return 0;}
    if(GetWinFlags()&WF_PMODE)return 2;instance=inst;
    automatic=!lstrcmp(cmd,"/test")?1:!lstrcmp(cmd,"/uitest")?2:0;visualMode=lstrcmp(cmd,"/visual")==0;if(automatic)logFile=_lcreat("C:\\BEATS.LOG",0);
    n=GetModuleFileName(inst,path,sizeof(path));
    if(!n||n>=sizeof(path)){MessageBox(NULL,"App path too long: move the complete kit to a shorter folder.","Tandy Beats Lab",MB_OK);return 3;}
    path[n]=0;for(i=(int)n-1;i>=0;--i)if(path[i]=='\\')break;
    if(i<0||(unsigned)i+12>=sizeof(path)){MessageBox(NULL,"App folder too long: move the complete kit.","Tandy Beats Lab",MB_OK);return 3;}
    lstrcpy(appdir,path);appdir[i+1]=0;lstrcpy(path+i+1,"MIDIMAP.DRV");module=LoadLibrary(path);
    if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}
    if(!midi)lstrcpy(status,"Mapper missing: extract complete kit");BPDefault(&pat);
    wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;wc.cbClsExtra=wc.cbWndExtra=0;wc.hInstance=inst;
    wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszMenuName=NULL;wc.lpszClassName="DWABeats";
    if(!RegisterClass(&wc))return 4;
    w=CreateWindow("DWABeats","Tandy Beats Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,312,198,NULL,NULL,inst,NULL);
    if(!w){if(module>=32)FreeLibrary(module);return 5;}
    ShowWindow(w,show);UpdateWindow(w);SetFocus(w);
    if(visualMode){HFILE ready=_lcreat("C:\\BEATSRDY.TXT",0);if(ready!=HFILE_ERROR)_lclose(ready);}
    while(GetMessage(&msg,NULL,0,0)){
        if(msg.message==WM_KEYDOWN){
            if(msg.wParam==VK_TAB){HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);if(next)SetFocus(next);continue;}
            if(msg.wParam==VK_ESCAPE||msg.wParam==VK_F5||msg.wParam==VK_F1||(GetKeyState(VK_CONTROL)<0&&(msg.wParam=='S'||msg.wParam=='O'))||
               (msg.hwnd==w&&(msg.wParam=='P'||msg.wParam=='S'||msg.wParam=='D'||msg.wParam=='V'||msg.wParam==VK_SPACE||msg.wParam==VK_RETURN||msg.wParam==VK_LEFT||msg.wParam==VK_RIGHT||msg.wParam==VK_UP||msg.wParam==VK_DOWN||msg.wParam==VK_ADD||msg.wParam==VK_SUBTRACT||msg.wParam==0xbb||msg.wParam==0xbd||msg.wParam==VK_PRIOR||msg.wParam==VK_NEXT||msg.wParam==VK_DELETE))){SendMessage(w,WM_KEYDOWN,msg.wParam,msg.lParam);continue;}
        }
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
    stop();if(editThunk)FreeProcInstance(editThunk);
    if(automatic){check("close releases mapper",!opened&&debug&&debug(1)==0);check("close mutes voices",debug&&debug(0)==0);wsprintf(line,"TOTAL_FAILURES=%d\r\n",failures);if(logFile!=HFILE_ERROR){_lwrite(logFile,line,lstrlen(line));_lclose(logFile);}}
    if(module>=32)FreeLibrary(module);if(automatic||visualMode)ExitWindows(0L,0);return failures?6:0;
}
