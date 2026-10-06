/* JOYMIDI: bounded Win3.0 real-mode /8088 app-local joystick instrument. */
#define WINVER 0x0300
static int showDetails;
#include <windows.h>
#include "MIDIAPI.H"
#include "JMCORE.H"
#define RANGE 101
#define CENTER 102
#define ARM 103
#define PANIC 104
#define PULSE 1
static HINSTANCE instance,module;
static HWND mainWindow;
static MIDIPROC midi;
static DEBUGPROC debug;
static int opened,active,collecting=1,port,automatic,stage,failures,faultPending;
static unsigned polls,hardwarePolls,sent;
static DWORD beginTick,lastSample,lastPaint,simTime;
static JOY_SAMPLE sample;
static JOY_CAL cal;
static JM_STATE state;
static unsigned char raw[JOY_SAMPLES];
static char status[19]="Sweep full range";
static char lastMessage[19]="MIDI: none";
static HFILE logFile=HFILE_ERROR;
static void logText(char *s){if(logFile!=HFILE_ERROR){_lwrite(logFile,s,lstrlen(s));_lwrite(logFile,"\r\n",2);}}
static void check(char *s,int ok){char b[100];wsprintf(b,"%s %s",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)s);logText(b);if(!ok)failures++;}
static void redraw(void){if(mainWindow)InvalidateRect(mainWindow,NULL,FALSE);}
static void statusText(char *s){unsigned i;for(i=0;i+1<sizeof(status)&&s[i];i++)status[i]=s[i];status[i]=0;}
static void notice(char *s){statusText(s);redraw();}
static DWORD message(WORD m,DWORD v){if(faultPending&&m==MODM_DATA){faultPending=0;return MMSYSERR_ERROR;}return midi?midi(0,m,MIDI_COOKIE,v,0L):MMSYSERR_ERROR;}
static void releaseDriver(void){
 if(opened){check("driver reset",message(MODM_RESET,0L)==0L);check("voices silent",debug&&debug(0)==0);check("driver close",message(MODM_CLOSE,0L)==0L);opened=0;check("owner released",debug&&debug(1)==0);}
}
static void sendOutput(JM_OUT *o){unsigned i;DWORD r;char b[80];
 for(i=0;i<o->count&&opened;i++){
  r=message(MODM_DATA,(DWORD)o->msg[i].status|((DWORD)o->msg[i].a<<8)|((DWORD)o->msg[i].b<<16));
  if(r){releaseDriver();jm_init(&state);if(mainWindow)SetWindowText(GetDlgItem(mainWindow,ARM),"&Arm");notice("MIDI send failed");return;}
  sent++;wsprintf(lastMessage,"MIDI %02X %02X %02X",(unsigned)o->msg[i].status,(unsigned)o->msg[i].a,(unsigned)o->msg[i].b);
  if(automatic){wsprintf(b,"EVENT %u %u %u",(unsigned)o->msg[i].status,(unsigned)o->msg[i].a,(unsigned)o->msg[i].b);logText(b);}
 }
 if(o->stopped)releaseDriver();
 if(o->stopped&&mainWindow)SetWindowText(GetDlgItem(mainWindow,ARM),"&Arm");
}
static void mute(char *why){JM_OUT o;jm_stop(&state,&o);sendOutput(&o);releaseDriver();notice(why);}
static int acquireDriver(void){TOPEN desc;DWORD cookie=0,r;TCAPS caps;
 if(!midi){notice("Driver missing");return 0;}
 if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||caps.wVoices!=3||caps.vDriverVersion<0x0102){notice("Wrong driver");return 0;}
 desc.hMidi=0;desc.dwCallback=0L;desc.dwInstance=0L;r=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L);
 if(r){notice(r==MMSYSERR_ALLOCATED?"PSG is busy":"Driver open failed");return 0;}
 opened=1;if(cookie!=MIDI_COOKIE){releaseDriver();notice("Bad driver cookie");return 0;}
    if(message(MODM_SYNTHCONFIG,3L)){releaseDriver();notice("Synth setup failed");return 0;}
    return 1;
}
static void process(DWORD now){int x=0,y=0,valid;JM_OUT o;
 joy_decode(raw,JOY_SAMPLES,&sample);polls++;lastSample=now;
 if(collecting&&!state.armed)joy_track(&cal,&sample);
 valid=jm_ready(&cal,&sample,(unsigned)port,&x,&y);
 jm_tick(&state,valid,x,y,(sample.buttons>>(port*2))&3,now,&o);
 sendOutput(&o);if(o.stopped)notice("Muted: re-arm");else if(state.armed)statusText(state.releaseGate?"Release B1 to play":state.sounding?"Playing: hold B1":"Armed: hold B1");
}
static void poll(void){joy_capture(raw);hardwarePolls++;process(GetTickCount());}
static void range(void){mute("Sweep full range");joy_reset(&cal);collecting=1;}
static void center(void){int x,y;mute("Center failed");if(!automatic)poll();joy_center(&cal,&sample);
 if(jm_ready(&cal,&sample,(unsigned)port,&x,&y)){collecting=0;notice("Ready: press Arm");}
 else{collecting=1;notice("Need wider range");}
}
static void arm(void){int x,y;DWORD now=automatic?simTime:GetTickCount();
 if(state.armed){mute("Muted");return;}if(!automatic){poll();now=GetTickCount();}
 if(collecting||now-lastSample>500UL||!jm_ready(&cal,&sample,(unsigned)port,&x,&y)){notice("Range then Center");return;}
 if(!acquireDriver())return;jm_arm(&state,x,y,now);SetWindowText(GetDlgItem(mainWindow,ARM),"&Mute");notice("Release B1 to play");
}
static void changePort(void){port=!port;range();notice(port?"Left: sweep range":"Right: sweep range");}
static void drawLine(HDC dc,int y,char *s){char b[19];unsigned i;for(i=0;i<18&&s[i];i++)b[i]=s[i];while(i<18)b[i++]=' ';b[18]=0;TextOut(dc,4,y,b,18);}
static void paint(HWND w){HDC dc;PAINTSTRUCT ps;char b[64],c[6];unsigned i,a;int x,y,valid;
 dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));
 if(showDetails)wsprintf(b,"%s %s S%u",(LPSTR)(port?"L":"R"),(LPSTR)(state.armed?"ARMED":"MUTED"),sent);
 else wsprintf(b,"%s %s",(LPSTR)(port?"Left":"Right"),(LPSTR)(state.armed?"Armed":"Muted"));
 drawLine(dc,2,b);
 if(collecting || showDetails) {
  for(i=0;i<2;i++){a=port*2+i;if(cal.centered&(1u<<a))wsprintf(c,"%03u",cal.center[a]);else lstrcpy(c,"---");
   if(cal.seen&(1u<<a))wsprintf(b,"%s%03u-%03u C%s",(LPSTR)(i?"Y":"X"),cal.low[a],cal.high[a],(LPSTR)c);else wsprintf(b,"%s--- --- C---",(LPSTR)(i?"Y":"X"));drawLine(dc,18+(int)i*16,b);}
 } else {drawLine(dc,18,"Range calibrated");drawLine(dc,34,"B1 play  B2 mute");}
 valid=jm_ready(&cal,&sample,(unsigned)port,&x,&y);
 if(!valid)lstrcpy(b,(sample.timeout&(3u<<(port*2)))?"Input timeout":"Input/cal unknown");
 else if(showDetails)wsprintf(b,"X%3d Y%3d B%d%d",x,y,(sample.buttons>>(port*2))&1,(sample.buttons>>(port*2+1))&1);
 else {static char *names[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  if(state.note>=0 && state.note<=127)wsprintf(b,"Note %s%d",(LPSTR)names[state.note%12],state.note/12-1);
  else lstrcpy(b,"Note --");}
 drawLine(dc,50,b);
 wsprintf(b,"Next attack %d",state.velocity);drawLine(dc,66,b);
 drawLine(dc,82,showDetails?lastMessage:"");drawLine(dc,98,status);drawLine(dc,162,"P port Esc panic");EndPaint(w,&ps);
}
static void button(HWND w,char *s,int x,int y,int id){HWND b;b=CreateWindow("BUTTON",s,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,68,20,w,(HMENU)id,instance,NULL);if(b)SendMessage(b,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);}
/* Test-only synthetic raw input still passes through production decoding,
   calibration and state machine; no joystick I/O in synthetic modes 1, 2, 4, 5 and 6. */
static void inject(int x,int y,unsigned buttons){unsigned i,b,tx=50+x*2,ty=50+y*2;
 for(i=0;i<JOY_SAMPLES;i++){b=(~(buttons<<(port*2))&15)<<4;if(i<tx)b|=5;if(i<ty)b|=10;raw[i]=(unsigned char)b;}simTime+=55UL;process(simTime);
}
static void repeat(int x,int y,unsigned b,unsigned n){while(n--)inject(x,y,b);}
static void fixture(void){range();inject(0,0,0);inject(100,100,0);inject(50,50,0);center();check("fixture calibrated",!collecting);}
static void testStep(void){unsigned before;char caption[16];
 if(automatic==6){if(!stage){fixture();arm();repeat(50,50,0,1);faultPending=1;repeat(50,50,1,2);GetWindowText(GetDlgItem(mainWindow,ARM),caption,sizeof(caption));check("send error fails closed",!opened&&!state.armed&&debug&&debug(0)==0&&debug(1)==0&&sent==0);check("send error restores Arm caption",lstrcmp(caption,"&Arm")==0);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);}return;}
 if(automatic==4){if(!stage){fixture();arm();check("missing driver stays muted",!opened&&!state.armed);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);}return;}
 if(automatic==3){if(!stage){check("real reader returns",hardwarePolls>0);check("starts muted",!opened&&!state.armed&&sent==0);stage++;PostMessage(mainWindow,WM_CLOSE,0,0L);}return;}
 if(automatic==2){
  if(stage==0){fixture();check("SOUND lease",OpenSound()>=0);arm();check("busy refusal stays muted",!opened&&!state.armed);CloseSound();arm();check("retry acquired",opened&&state.armed);repeat(50,50,0,1);repeat(50,50,1,2);check("retry sounds",debug&&debug(0)==1);stage++;}
  else{mute("Panic / muted");check("busy test cleanup",!opened&&debug&&debug(0)==0&&debug(1)==0);PostMessage(mainWindow,WM_CLOSE,0,0L);stage++;}return;
 }
 switch(stage++){
 case 0:check("window controls",GetDlgItem(mainWindow,RANGE)&&GetDlgItem(mainWindow,CENTER)&&GetDlgItem(mainWindow,ARM)&&GetDlgItem(mainWindow,PANIC));check("startup muted",!opened&&!state.armed&&sent==0);fixture();arm();check("arm owns driver",opened&&state.armed&&debug&&debug(1)==1);repeat(50,50,1,4);check("held at arm stays silent",debug&&debug(0)==0&&sent==0);break;
 case 1:repeat(50,50,0,1);repeat(50,50,1,2);check("button note on",debug&&debug(4)==66&&debug(0)==1);before=sent;repeat(49,50,1,1);repeat(51,50,1,1);repeat(50,50,1,6);check("jitter emits nothing",sent==before);break;
 case 2:repeat(100,0,1,4);check("horizontal changes note",debug&&debug(4)==72&&debug(0)==1);check("vertical attack loud",debug&&debug(12)==0);break;
 case 3:repeat(0,100,1,4);check("low note quiet",debug&&debug(4)==60&&debug(12)==13);break;
 case 4:repeat(0,0,0,1);check("release immediate",debug&&debug(0)==0);repeat(50,50,0,4);repeat(50,50,1,2);check("repress sounds",debug&&debug(4)==66);mute("Panic / muted");check("panic release",!opened&&!state.armed&&debug&&debug(0)==0&&debug(1)==0);break;
 case 5:arm();repeat(50,50,0,1);repeat(50,50,1,2);raw[0]=0xff;{unsigned i;for(i=0;i<JOY_SAMPLES;i++)raw[i]=0xff;}simTime+=55;process(simTime);check("timeout mutes and releases",!opened&&!state.armed&&debug&&debug(0)==0&&debug(1)==0);repeat(50,50,1,4);check("recovery requires arm",!opened&&debug&&debug(0)==0);break;
 case 6:repeat(50,50,0,1);arm();repeat(50,50,0,1);repeat(50,50,1,2);SendMessage(mainWindow,WM_ACTIVATE,0,0L);check("focus loss mutes",!opened&&!state.armed&&debug&&debug(0)==0&&debug(1)==0);SendMessage(mainWindow,WM_ACTIVATE,1,0L);break;
 case 7:changePort();fixture();arm();repeat(50,50,0,1);repeat(50,50,1,2);check("left port button routing",port==1&&state.sounding==66);check("duplicate instance launch",WinExec("C:\\JMIDI\\JOYMIDI.EXE",SW_SHOWNORMAL)>=32);check("duplicate preserves owner",opened&&state.armed&&debug&&debug(1)==1);if(automatic==5){check("intruder launch",WinExec("C:\\MIDIINTR.EXE",SW_SHOWNORMAL)>=32);check("intruder preserves owner",opened&&state.armed&&debug&&debug(1)==1&&debug(0)==1);}break;
 case 8:check("no test joystick reads",hardwarePolls==0);check("close while sounding",opened&&state.sounding);PostMessage(mainWindow,WM_CLOSE,0,0L);break;
 }
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp){DWORD now;
 switch(m){
 case WM_CREATE:mainWindow=w;button(w,"&Range",4,116,RANGE);button(w,"&Center",76,116,CENTER);button(w,"&Arm",4,140,ARM);button(w,"Panic",76,140,PANIC);if(!GetDlgItem(w,RANGE)||!GetDlgItem(w,CENTER)||!GetDlgItem(w,ARM)||!GetDlgItem(w,PANIC))return -1;if(!SetTimer(w,PULSE,55,NULL))return -1;beginTick=GetTickCount();return 0;
 case WM_COMMAND:if(wp==RANGE)range();if(wp==CENTER)center();if(wp==ARM)arm();if(wp==PANIC)mute("Panic / muted");return 0;
 case WM_ACTIVATE:active=wp!=0;if(!active&&state.armed)mute("Focus lost: muted");return 0;
 case WM_ACTIVATEAPP:if(!wp&&state.armed)mute("Focus lost: muted");break;
 case WM_TIMER:if(wp==PULSE){if(opened)message(MODM_SYNTHUPDATE,0L);now=GetTickCount();if((!automatic||automatic==3)&&active&&!IsIconic(w))poll();if(now-lastPaint>=330UL){redraw();lastPaint=now;}if(automatic&&now-beginTick>800UL+(DWORD)stage*700UL)testStep();}return 0;
 case WM_PAINT:paint(w);return 0;
 case WM_QUERYENDSESSION:mute("Session: muted");return TRUE;
 case WM_ENDSESSION:if(wp)mute("Session: muted");return 0;
 case WM_CLOSE:mute("Closed / muted");DestroyWindow(w);return 0;
 case WM_DESTROY:KillTimer(w,PULSE);mainWindow=0;mute("Closed / muted");PostQuitMessage(0);return 0;
 }return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show){WNDCLASS wc;HWND w;MSG msg;OFSTRUCT of;char path[144],b[100];int i,result=0;
 if(previous){w=FindWindow("TandyJoyMIDI",NULL);if(w){ShowWindow(w,SW_RESTORE);BringWindowToTop(w);}return 0;}
 if(GetWinFlags()&WF_PMODE)return 1;instance=inst;
 if(lstrcmp(cmd,"/diag")==0)showDetails=1;
 if(lstrcmp(cmd,"/test")==0)automatic=1;if(lstrcmp(cmd,"/busytest")==0)automatic=2;if(lstrcmp(cmd,"/porttest")==0)automatic=3;if(lstrcmp(cmd,"/missing")==0)automatic=4;if(lstrcmp(cmd,"/ownertest")==0)automatic=5;if(lstrcmp(cmd,"/sendtest")==0)automatic=6;
 if(automatic)logFile=_lcreat("C:\\JOYMIDI.LOG",0);joy_reset(&cal);joy_decode(raw,0,&sample);jm_init(&state);
 i=GetModuleFileName(inst,path,sizeof(path));path[143]=0;if(i<=0||i>=143){result=2;goto cleanup;}
 for(i=lstrlen(path)-1;i>=0;i--)if(path[i]=='\\')break;if(i<0||i>130){result=2;goto cleanup;}path[i+1]=0;lstrcat(path,"MIDIMAP.DRV");
 if(OpenFile(path,&of,OF_EXIST)!=HFILE_ERROR)module=LoadLibrary(path);if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}if(!midi)lstrcpy(status,"Driver missing");
 wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszMenuName=NULL;wc.lpszClassName="TandyJoyMIDI";
 if(!RegisterClass(&wc)){result=3;goto cleanup;}w=CreateWindow("TandyJoyMIDI","Joystick MIDI",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,NULL,NULL,inst,NULL);
 if(!w){result=4;goto cleanup;}ShowWindow(w,show);UpdateWindow(w);
 while(GetMessage(&msg,NULL,0,0)){
  if(msg.message==WM_KEYDOWN){if(msg.wParam==VK_ESCAPE){mute("Panic / muted");continue;}if(msg.wParam=='Q'){PostMessage(w,WM_CLOSE,0,0L);continue;}
   if(msg.wParam=='R'){range();continue;}if(msg.wParam=='C'){center();continue;}if(msg.wParam=='A'){arm();continue;}if(msg.wParam=='P'){changePort();continue;}
   if(msg.wParam==VK_TAB){HWND next;next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);if(next)SetFocus(next);continue;}}
  TranslateMessage(&msg);DispatchMessage(&msg);
 }
cleanup:mute("Closed / muted");check("final no owner",!opened);if(module>=32)FreeLibrary(module);wsprintf(b,"TOTAL_FAILURES=%d SENT=%u POLLS=%u HARDWARE_POLLS=%u",failures,sent,polls,hardwarePolls);logText(b);if(logFile!=HFILE_ERROR)_lclose(logFile);if(automatic)ExitWindows(0L,0);return result?result:failures;
}

