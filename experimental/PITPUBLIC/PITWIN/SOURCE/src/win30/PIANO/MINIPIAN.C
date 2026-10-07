/* Mini Piano 0.1. Native Win3.0 real-mode 8088 app-local instrument.
            GPL-3.0-or-later. Musical notes are event-driven; only a cooperative
            mouse-capture safety watchdog and optional test sequencer use WM_TIMER. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include "PIANO.H"
#include "PITCLNT.H"
#define OCTDOWN 101
#define OCTUP 102
#define PANIC 103
#define WATCH 1
#define TEST 2
static HINSTANCE instance,module;
static HWND mainWindow;
static MIDIPROC midi;
static DEBUGPROC debug;
static PI_STATE state;
static PIT_CLIENT pitClient;
static int pitEnabled;
static int opened,active,mouseDown,automatic,stage,failures,faultPending;
static unsigned sent;
static HFILE logFile=HFILE_ERROR;
static char status[19]="Ready: hold a key";
static unsigned char whiteNotes[8]={0,2,4,5,7,9,11,12};
static unsigned char blackNotes[5]={1,3,6,8,10};
static unsigned char blackX[5]={16,34,70,88,106};
static char *whiteNames="CDEFGABC",*whiteKeys="ASDFGHJK",*blackKeys="WETYU";
static void logText(char *s){if(logFile!=HFILE_ERROR){_lwrite(logFile,s,lstrlen(s));_lwrite(logFile,"\r\n",2);}}
static void check(char *s,int ok){char b[100];wsprintf(b,"%s %s",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)s);logText(b);if(!ok)failures++;}
static void notice(char *s){unsigned i;RECT r;for(i=0;i<18&&s[i];i++)status[i]=s[i];status[i]=0;if(mainWindow){SetRect(&r,0,122,148,138);InvalidateRect(mainWindow,&r,FALSE);}}
static void redrawKeys(void){RECT r;if(mainWindow){SetRect(&r,0,0,148,18);InvalidateRect(mainWindow,&r,FALSE);SetRect(&r,0,46,148,121);InvalidateRect(mainWindow,&r,FALSE);}}
static void redrawNote(unsigned n){RECT r;unsigned i,base=(state.octave+1u)*12u;if(!mainWindow||n<base||n>base+12u)return;n-=base;for(i=0;i<8;i++)if(n==whiteNotes[i]){SetRect(&r,2+(int)i*18,91,20+(int)i*18,120);InvalidateRect(mainWindow,&r,FALSE);return;}for(i=0;i<5;i++)if(n==blackNotes[i]){SetRect(&r,(int)blackX[i],76,(int)blackX[i]+11,86);InvalidateRect(mainWindow,&r,FALSE);return;}}
static DWORD message(WORD m,DWORD v){if(faultPending&&m==MODM_DATA){faultPending=0;return MMSYSERR_ERROR;}return midi?midi(0,m,MIDI_COOKIE,v,0L):MMSYSERR_ERROR;}
static void releaseDriver(void){if(opened){
    if(pitClient.proc)check("PIT off",PitClientOff(&pitClient,1)==0);check("driver reset",message(MODM_RESET,0L)==0L);check("voices silent",!debug||debug(0)==0);check("driver close",message(MODM_CLOSE,0L)==0L);opened=0;check("owner released",!debug||debug(1)==0);}PitClientForget(&pitClient);}
static void releaseMouse(void){mouseDown=0;if(mainWindow)KillTimer(mainWindow,WATCH);if(mainWindow&&GetCapture()==mainWindow)ReleaseCapture();}
static int syncPit(void)
{
    unsigned n,a,b,c,chosen=0;int result;
    if(!pitEnabled || !opened || !state.held){
        if(pitClient.note && opened)return PitClientOff(&pitClient,1)==0;
        PitClientForget(&pitClient);return 1;
    }
    if(!debug)return 0;
    a=debug(4);b=debug(5);c=debug(6);
    for(n=45;n<=96;n++)if(state.refs[n] && n!=a && n!=b && n!=c){
        chosen=n;break;
    }
    result=chosen?PitClientNote(&pitClient,opened,chosen):
        (pitClient.note?PitClientOff(&pitClient,opened):0);
    return result==0;
}
static int sendOutput(PI_OUT *o){unsigned i;DWORD r;char b[80];for(i=0;i<o->count&&opened;i++){
    r=message(MODM_DATA,(DWORD)o->event[i].status|((DWORD)o->event[i].note<<8)|((DWORD)o->event[i].velocity<<16));
    if(r){PI_OUT discarded;releaseMouse();releaseDriver();pi_panic(&state,&discarded);notice("MIDI send failed");redrawKeys();return 0;}
    sent++;redrawNote(o->event[i].note);if(automatic){wsprintf(b,"EVENT %u %u %u",(unsigned)o->event[i].status,(unsigned)o->event[i].note,(unsigned)o->event[i].velocity);logText(b);}}
    if(!syncPit()){
        PI_OUT discarded;releaseMouse();releaseDriver();
        pi_panic(&state,&discarded);notice("Extra voice failed");
        redrawKeys();return 0;
    }
    if(!state.held)releaseDriver();return 1;
}
static void mute(char *why){PI_OUT o;releaseMouse();pi_panic(&state,&o);sendOutput(&o);releaseDriver();notice(why);}
static int acquireDriver(void){TOPEN desc;DWORD cookie=0,r;TCAPS caps;
    if(opened)return 1;if(!midi){notice("Driver missing");return 0;}
    if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||caps.wVoices!=3||caps.vDriverVersion<0x0101){notice("Wrong driver");return 0;}
    desc.hMidi=0;desc.dwCallback=0L;desc.dwInstance=0L;r=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L);
    if(r){notice(r==MMSYSERR_ALLOCATED?"Sound busy; retry":"Driver open failed");return 0;}opened=1;
    if(cookie!=MIDI_COOKIE){releaseDriver();notice("Bad driver cookie");return 0;}return 1;
}
static void press(unsigned src,unsigned off){PI_OUT o;char b[40];if(!active||src>=PI_SOURCES||off>=PI_KEYS)return;if((unsigned)state.source[src]==(state.octave+1u)*12u+off)return;if(!acquireDriver())return;pi_press(&state,src,off,&o);if(sendOutput(&o)){wsprintf(b,"Held %u / %u voices",(unsigned)state.held,pitEnabled?4u:3u);notice(b);}}
static void release(unsigned src){PI_OUT o;if(src>=PI_SOURCES||!state.source[src])return;pi_release(&state,src,&o);if(sendOutput(&o))notice(state.held?"Hold / release keys":"Ready: hold a key");}
static void changeOctave(int d){PI_OUT o;releaseMouse();pi_octave(&state,d,&o);sendOutput(&o);notice("Octave: notes off");redrawKeys();}
static void togglePit(void)
{
    mute("Extra voice: off");
    if(pitEnabled)pitEnabled=0;
    else if(debug && PitClientDiscover(&pitClient)==0)pitEnabled=1;
    else notice("Extra unavailable");
    if(pitEnabled)notice("Extra voice: on");
    redrawKeys();
}
static int hit(int x,int y){unsigned i;if(y<48||y>=120||x<2||x>=146)return -1;if(y<90)for(i=0;i<5;i++)if(x>=(int)blackX[i]&&x<(int)blackX[i]+11)return blackNotes[i];return whiteNotes[(x-2)/18];}
static void mouseMove(HWND w,int x,int y){int n;if(!mouseDown)return;if(GetCapture()!=w){mute("Capture lost: off");return;}n=hit(x,y);if(n<0)release(PI_MOUSE);else press(PI_MOUSE,(unsigned)n);}
static int key(UINT m,WPARAM wp,LPARAM lp){int k;unsigned scan;if(m==WM_SYSKEYDOWN){mute("Menu: notes off");return 0;}if(m!=WM_KEYDOWN&&m!=WM_KEYUP)return 0;
    if(wp==VK_F3){
        if(m==WM_KEYDOWN && !(lp&0x40000000L))togglePit();return 1;
    }
    if(wp==VK_ESCAPE){if(m==WM_KEYDOWN)mute("Panic: all off");return 1;}
    if(m==WM_KEYDOWN&&(wp==VK_LEFT||wp==VK_RIGHT)){if(!(lp&0x40000000L))changeOctave(wp==VK_LEFT?-1:1);return 1;}
    if(lp&0x01000000L)return 0;scan=(unsigned)((lp>>16)&255);k=pi_scan(scan);if(k<0)return 0;
    if(m==WM_KEYDOWN){if(!(lp&0x40000000L))press((unsigned)k,(unsigned)k);}else release((unsigned)k);return 1;
}
static void drawText(HDC dc,int x,int y,char *s){TextOut(dc,x,y,s,lstrlen(s));}
static void paint(HWND w){HDC dc;PAINTSTRUCT ps;RECT r,clip;char b[40],c[2];unsigned i,n;int x,on;dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));SetRect(&r,0,0,148,18);if(IntersectRect(&clip,&r,&ps.rcPaint)){FillRect(dc,&r,GetStockObject(WHITE_BRUSH));wsprintf(b,"C%d-C%d %s",(int)state.octave,(int)state.octave+1,(LPSTR)(pitEnabled?"3 + PIT":"3 voices"));drawText(dc,3,2,b);}
    c[1]=0;for(i=0;i<8;i++){x=2+(int)i*18;n=(state.octave+1u)*12u+whiteNotes[i];on=state.refs[n]!=0;SetRect(&r,x,48,x+18,120);if(!RectVisible(dc,&r))continue;FillRect(dc,&r,GetStockObject(WHITE_BRUSH));FrameRect(dc,&r,GetStockObject(BLACK_BRUSH));if(on){SetRect(&r,x+2,91,x+16,118);FillRect(dc,&r,GetStockObject(BLACK_BRUSH));}SetBkColor(dc,on?RGB(0,0,0):RGB(255,255,255));SetTextColor(dc,on?RGB(255,255,255):RGB(0,0,0));c[0]=whiteKeys[i];drawText(dc,x+5,91,c);c[0]=whiteNames[i];drawText(dc,x+5,104,c);}
    for(i=0;i<5;i++){x=blackX[i];n=(state.octave+1u)*12u+blackNotes[i];on=state.refs[n]!=0;SetRect(&r,x,48,x+11,90);if(!RectVisible(dc,&r))continue;FillRect(dc,&r,GetStockObject(BLACK_BRUSH));SetTextColor(dc,RGB(255,255,255));SetBkColor(dc,RGB(0,0,0));c[0]=blackKeys[i];drawText(dc,x+2,54,c);if(on){SetRect(&r,x+3,76,x+8,85);FillRect(dc,&r,GetStockObject(WHITE_BRUSH));}}
    SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));SetRect(&r,0,122,148,138);if(IntersectRect(&clip,&r,&ps.rcPaint)){FillRect(dc,&r,GetStockObject(WHITE_BRUSH));drawText(dc,3,122,status);}SetRect(&r,0,138,148,177);if(IntersectRect(&clip,&r,&ps.rcPaint)){FillRect(dc,&r,GetStockObject(WHITE_BRUSH));drawText(dc,3,139,"F3 extra   Esc off");drawText(dc,3,155,"Hold mouse or keys");}EndPaint(w,&ps);
}
static void button(HWND w,char *s,int x,int width,int id){HWND b;b=CreateWindow("BUTTON",s,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,22,width,22,w,(HMENU)id,instance,NULL);if(b)SendMessage(b,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);}
static LPARAM kl(unsigned scan,int up,int repeat){return 1L|((DWORD)scan<<16)|(up?0xc0000000L:repeat?0x40000000L:0L);}
static void kd(unsigned scan,int repeat){SendMessage(mainWindow,WM_KEYDOWN,0,kl(scan,0,repeat));}
static void ku(unsigned scan){SendMessage(mainWindow,WM_KEYUP,0,kl(scan,1,0));}
static void testStep(void){unsigned before;int n;static unsigned char melody[8]={0x1e,0x20,0x22,0x25,0x24,0x22,0x20,0x1e};
    if(automatic==9){
        SendMessage(mainWindow,WM_KEYDOWN,VK_F3,0L);
        check("old SOUND declines PIT",!pitEnabled&&!pitClient.proc);
        kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
        check("old SOUND retains PSG",opened&&debug&&debug(0)==7
            && !pitClient.note);
        PostMessage(mainWindow,WM_CLOSE,0,0L);return;
    }
    if(automatic==8){
        switch(stage++){
        case 0:
            check("PIT default off",!pitEnabled&&!pitClient.note&&!opened);
            SendMessage(mainWindow,WM_KEYDOWN,VK_F3,0L);
            check("PIT opt in",pitEnabled&&pitClient.proc);
            check("PIT no lease refuses",
                PitClientNote(&pitClient,0,60)==PITCLIENT_NOT_OWNED
                && !pitClient.note);
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);
            check("PIT three unchanged",debug&&debug(0)==7&&!pitClient.note);
            check("PIT pitch bounds",
                PitClientNote(&pitClient,opened,44)==PITCLIENT_BAD_NOTE
                && PitClientNote(&pitClient,opened,97)==PITCLIENT_BAD_NOTE
                && !pitClient.note);
            kd(0x25,0);
            check("PIT fourth distinct",debug&&debug(0)==7&&pitClient.note==60);
            before=sent;kd(0x25,1);
            check("PIT repeat no duplicate",sent==before&&pitClient.note==60);
            break;
        case 1:
            SendMessage(mainWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(10,110));
            check("PIT duplicate ref",state.refs[60]==2&&pitClient.note==60);
            ku(0x1e);check("PIT duplicate stays",state.refs[60]==1&&pitClient.note==60);
            SendMessage(mainWindow,WM_LBUTTONUP,0,MAKELONG(10,110));
            check("PIT released extra off",!pitClient.note&&debug&&debug(0)==7);
            ku(0x20);ku(0x22);ku(0x25);
            check("PIT last release",!pitClient.note&&!opened&&!state.held);
            break;
        case 2:
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
            check("PIT replay",pitClient.note==60);
            SendMessage(mainWindow,WM_KILLFOCUS,0,0L);
            check("PIT focus cleanup",!pitClient.note&&!opened&&!state.held);
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
            changeOctave(1);
            check("PIT octave cleanup",!pitClient.note&&!opened&&!state.held);
            break;
        case 3:
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
            check("PIT new octave",pitClient.note==72);
            SendMessage(mainWindow,WM_KEYDOWN,VK_ESCAPE,0L);
            check("PIT Escape cleanup",!pitClient.note&&!opened&&!state.held);
            SendMessage(mainWindow,WM_KEYDOWN,VK_F3,0L);
            check("PIT toggle off",!pitEnabled&&!pitClient.note);
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
            check("PIT off retains PSG",!pitClient.note&&debug&&debug(0)==7);
            SendMessage(mainWindow,WM_KEYDOWN,VK_F3,0L);
            check("PIT toggle panics",pitEnabled&&!opened&&!state.held);
            kd(0x1e,0);kd(0x20,0);kd(0x22,0);kd(0x25,0);
            PostMessage(mainWindow,WM_CLOSE,0,0L);break;
        }
        return;
    }
    if(automatic==7){if(stage>0)ku(melody[stage-1]);if(stage<8)kd(melody[stage],0);else PostMessage(mainWindow,WM_CLOSE,0,0L);stage++;return;}
    if(automatic==4){kd(0x1e,0);check("missing stays muted",!opened&&!state.held&&sent==0);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);return;}
    if(automatic==6){faultPending=1;kd(0x1e,0);check("send failure mutes",!opened&&!state.held&&debug&&debug(0)==0&&debug(1)==0);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);return;}
    if(automatic==2){check("SOUND lease",OpenSound()>=0);kd(0x1e,0);check("busy stays muted",!opened&&!state.held&&sent==0);CloseSound();ku(0x1e);kd(0x1e,0);check("retry sounds",opened&&state.held==1&&debug&&debug(0)==1);ku(0x1e);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);return;}
    switch(stage++){
    case 0:check("controls exist",GetDlgItem(mainWindow,OCTDOWN)&&GetDlgItem(mainWindow,OCTUP)&&GetDlgItem(mainWindow,PANIC));check("starts muted",!opened&&!state.held&&sent==0);kd(0x1e,0);check("A sounds C4",opened&&state.source[0]==60&&debug&&debug(4)==60&&debug(0)==1);before=sent;kd(0x1e,1);kd(0x1e,0);check("repeat no duplicate",sent==before&&state.held==1);break;
    case 1:SendMessage(mainWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(10,110));check("shared mouse key",state.held==2&&state.refs[60]==2&&debug&&debug(0)==1);before=sent;ku(0x1e);check("key release keeps mouse",state.held==1&&state.refs[60]==1&&sent==before&&debug&&debug(0)==1);break;
    case 2:SendMessage(mainWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELONG(39,60));check("mouse glissando D sharp",state.source[PI_MOUSE]==63&&debug&&debug(4)==63);SendMessage(mainWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELONG(0,130));check("outside piano off",!state.held&&!opened);SendMessage(mainWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELONG(10,110));check("drag back sounds",state.held==1&&opened);SendMessage(mainWindow,WM_LBUTTONUP,0,MAKELONG(10,110));check("mouse release cleanup",!mouseDown&&!opened&&!state.held&&GetCapture()!=mainWindow);break;
    case 3:kd(0x1e,0);kd(0x20,0);kd(0x22,0);check("three voices",debug&&debug(0)==7);kd(0x25,0);check("fourth steals oldest",debug&&debug(0)==7&&debug(4)==72);ku(0x1e);check("stolen release harmless",debug&&debug(0)==7);ku(0x20);ku(0x22);ku(0x25);check("chord cleanup",!opened&&!state.held);break;
    case 4:kd(0x1e,0);SendMessage(mainWindow,WM_COMMAND,OCTUP,0L);check("octave mutes old",state.octave==5&&!opened&&!state.held);before=sent;kd(0x1e,1);ku(0x1e);check("old hold does not retrigger",sent==before&&!opened);kd(0x1e,0);check("new octave sounds C5",state.source[0]==72&&debug&&debug(4)==72);ku(0x1e);SendMessage(mainWindow,WM_COMMAND,OCTDOWN,0L);break;
    case 5:kd(0x1e,0);SendMessage(mainWindow,WM_KILLFOCUS,0,0L);check("focus loss cleanup",!state.held&&!opened&&debug&&debug(0)==0);kd(0x1e,0);SendMessage(mainWindow,WM_ACTIVATE,0,0L);check("deactivate cleanup",!state.held&&!opened);SendMessage(mainWindow,WM_ACTIVATE,1,0L);break;
    case 6:SendMessage(mainWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(10,110));ReleaseCapture();SendMessage(mainWindow,WM_TIMER,WATCH,0L);check("capture watchdog cleanup",!mouseDown&&!opened&&!state.held);SendMessage(mainWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(10,110));SendMessage(mainWindow,WM_CANCELMODE,0,0L);check("cancelmode cleanup",!mouseDown&&!opened&&!state.held);kd(0x1e,0);SendMessage(mainWindow,WM_INITMENU,0,0L);check("menu entry cleanup",!opened&&!state.held&&debug&&debug(0)==0);break;
    case 7:kd(0x11,0);check("black W sounds C sharp",state.source[1]==61&&debug&&debug(4)==61);SendMessage(mainWindow,WM_KEYDOWN,VK_ESCAPE,0L);check("Esc panic cleanup",!opened&&!state.held);kd(0x1e,0);check("duplicate launch",WinExec("C:\\MINIPIAN.EXE",SW_SHOWNORMAL)>=32);check("duplicate preserved notes",state.held==1&&opened&&debug&&debug(0)==1);if(automatic==5){check("intruder launch",WinExec("C:\\MIDIINTR.EXE",SW_SHOWNORMAL)>=32);check("intruder preserved notes",opened&&state.held==1&&debug&&debug(0)==1);}break;
    case 8:ku(0x1e);for(n=0;n<5;n++)changeOctave(1);check("top octave clamped",state.octave==6);kd(0x25,0);check("top C7 is note 96",state.source[12]==96);ku(0x25);for(n=0;n<7;n++)changeOctave(-1);check("bottom octave clamped",state.octave==3);kd(0x1e,0);check("bottom C3 is note 48",state.source[0]==48);PostMessage(mainWindow,WM_CLOSE,0,0L);break;
    }
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp){int n;
    switch(m){
    case WM_CREATE:mainWindow=w;button(w,"Oct -",2,47,OCTDOWN);button(w,"Oct +",51,47,OCTUP);button(w,"Panic",100,46,PANIC);if(!GetDlgItem(w,OCTDOWN)||!GetDlgItem(w,OCTUP)||!GetDlgItem(w,PANIC))return -1;if(automatic&&!SetTimer(w,TEST,1200,NULL))return -1;return 0;
    case WM_COMMAND:if(wp==OCTDOWN)changeOctave(-1);if(wp==OCTUP)changeOctave(1);if(wp==PANIC)mute("Panic: all off");SetFocus(w);return 0;
    case WM_LBUTTONDOWN:n=hit((int)LOWORD(lp),(int)HIWORD(lp));if(n<0)return 0;SetFocus(w);SetCapture(w);mouseDown=1;if(GetCapture()!=w||!SetTimer(w,WATCH,100,NULL)){mute("Mouse unavailable");return 0;}press(PI_MOUSE,(unsigned)n);if(!opened)releaseMouse();return 0;
    case WM_MOUSEMOVE:mouseMove(w,(int)LOWORD(lp),(int)HIWORD(lp));return 0;
    case WM_LBUTTONUP:releaseMouse();release(PI_MOUSE);return 0;
    case WM_CANCELMODE:mute("Cancelled: all off");return 0;
    case WM_NCLBUTTONDOWN:case WM_NCRBUTTONDOWN:case WM_SYSCOMMAND:case WM_INITMENU:mute("Menu: notes off");break;
    case WM_KEYDOWN:case WM_KEYUP:case WM_SYSKEYDOWN:if(key(m,wp,lp))return 0;break;
    case WM_KILLFOCUS:mute("Focus lost: off");return 0;
    case WM_ACTIVATE:active=wp!=0;if(!active)mute("Focus lost: off");return 0;
    case WM_ACTIVATEAPP:if(!wp)mute("Focus lost: off");break;
    case WM_TIMER:if(wp==WATCH&&mouseDown&&(GetCapture()!=w||(!automatic&&GetAsyncKeyState(VK_LBUTTON)>=0)))mute("Mouse released");if(wp==TEST&&automatic)testStep();return 0;
    case WM_PAINT:paint(w);return 0;
    case WM_QUERYENDSESSION:mute("Session: all off");return TRUE;
    case WM_ENDSESSION:if(wp)mute("Session: all off");return 0;
    case WM_CLOSE:mute("Closed: all off");DestroyWindow(w);return 0;
    case WM_DESTROY:KillTimer(w,WATCH);KillTimer(w,TEST);mainWindow=0;releaseDriver();PostQuitMessage(0);return 0;
    }return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show){WNDCLASS wc;HWND w;MSG msg;OFSTRUCT of;char path[144],b[100];int i,result=0;
    if(previous){w=FindWindow("TandyMiniPiano",NULL);if(w){ShowWindow(w,SW_RESTORE);BringWindowToTop(w);}return 0;}
    if(GetWinFlags()&WF_PMODE)return 1;instance=inst;pi_init(&state);
    if(lstrcmp(cmd,"/test")==0)automatic=1;if(lstrcmp(cmd,"/busytest")==0)automatic=2;if(lstrcmp(cmd,"/missing")==0)automatic=4;if(lstrcmp(cmd,"/ownertest")==0)automatic=5;if(lstrcmp(cmd,"/sendtest")==0)automatic=6;if(lstrcmp(cmd,"/demo")==0)automatic=7;if(lstrcmp(cmd,"/pittest")==0)automatic=8;if(lstrcmp(cmd,"/pitold")==0)automatic=9;
    if(automatic)logFile=_lcreat("C:\\MINIPIAN.LOG",0);
    i=GetModuleFileName(inst,path,sizeof(path));path[143]=0;if(i<=0||i>=143){result=2;goto cleanup;}for(i=lstrlen(path)-1;i>=0;i--)if(path[i]=='\\')break;if(i<0||i>130){result=2;goto cleanup;}path[i+1]=0;lstrcat(path,"MIDIMAP.DRV");
    if(OpenFile(path,&of,OF_EXIST)!=HFILE_ERROR)module=LoadLibrary(path);if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}if(!midi)lstrcpy(status,"Driver missing");
    wc.style=0;wc.lpfnWndProc=WndProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszMenuName=NULL;wc.lpszClassName="TandyMiniPiano";
    if(!RegisterClass(&wc)){result=3;goto cleanup;}w=CreateWindow("TandyMiniPiano","Mini Piano",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,NULL,NULL,inst,NULL);if(!w){result=4;goto cleanup;}ShowWindow(w,show);UpdateWindow(w);SetFocus(w);
    while(GetMessage(&msg,NULL,0,0)){
        if(msg.message==WM_KEYDOWN&&msg.wParam==VK_TAB){HWND next;mute("Tab: notes off");next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);if(next)SetFocus(next);continue;}
        if((msg.message==WM_KEYDOWN||msg.message==WM_KEYUP||msg.message==WM_SYSKEYDOWN)&&msg.hwnd!=w&&key(msg.message,msg.wParam,msg.lParam))continue;
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
cleanup:mute("Closed: all off");check("final no owner",!opened);check("final PIT off",!pitClient.note);if(module>=32)FreeLibrary(module);wsprintf(b,"TOTAL_FAILURES=%d SENT=%u",failures,sent);logText(b);if(logFile!=HFILE_ERROR)_lclose(logFile);if(automatic)ExitWindows(0L,0);return result?result:failures;
}
