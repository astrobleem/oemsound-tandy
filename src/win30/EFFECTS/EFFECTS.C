/* SPDX-License-Identifier: MIT. Native Windows 3.0 real-mode Effects Lab. */
#define WINVER 0x0300
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "FXCORE.H"
#include "TIMECLK.H"
#include "TCBURST.H"
#include "TCXBASS.H"
#define PLAY 100
#define STOP 101
#define PAUSE 102
#define RANDOM 103
#define MUTATE 104
#define TARGET 105
#define LOAD 106
#define SAVE 107
#define EXPORT 108
#define FILEEDIT 109
#define LESS 110
#define MORE 111
#define ABOUT 112
#define PRESET 120
#define FIELD 140
typedef WORD(FAR PASCAL *ZERO)(void);
typedef WORD(FAR PASCAL *ONE)(WORD);
typedef WORD(FAR PASCAL *PUT)(TCBURST FAR *);
typedef WORD(FAR PASCAL *XPUT)(TCXBURST FAR *);
static ZERO openFn,startFn,endFn,closeFn,pauseFn,resumeFn,continueFn,statusFn;
static ONE debugFn,xopenFn;static PUT putFn;static XPUT xputFn;
static HINSTANCE instance,module;static HWND mainWindow,fileEdit;
static FXPATCH patch;static FXCLIP clip;
static char appdir[144],status[64]="Choose a preset. F5 plays, Esc stops.";
static int owned,playing,paused,dirty,selected,automatic,stage,failures;
static DWORD testAt;static WORD pausedSent;static HFILE logfile=HFILE_ERROR;
static void say(const char *s){lstrcpy(status,s);if(mainWindow)InvalidateRect(mainWindow,NULL,FALSE);}
static void check(const char *name,int okay){char b[120];if(!okay)failures++;
 if(logfile>=0){wsprintf(b,"%s %s\r\n",(LPSTR)(okay?"PASS":"FAIL"),(LPSTR)name);_lwrite(logfile,b,lstrlen(b));}}
static int stop(void){if(owned){if(closeFn()){say("Close failed: sound still owned");return 0;}owned=0;}
 playing=paused=0;SetDlgItemText(mainWindow,PAUSE,"Pause");say("Stopped: sound released");return 1;}
static void labels(void){char b[64];unsigned i;
 for(i=0;i<FX_FIELDS;i++){
  if(!FXEnabled(&patch,i))wsprintf(b,"%s: --",(LPSTR)FXNames[i]);
  else if(i==FX_NOISE)wsprintf(b,"Noise: %s",(LPSTR)FXNoiseNames[patch.v[i]]);
  else if(i==FX_TARGET)wsprintf(b,"Target: %s",(LPSTR)(patch.v[i]?"Speaker":"PSG"));
  else wsprintf(b,"%s: %d",(LPSTR)FXNames[i],patch.v[i]);
  SetDlgItemText(mainWindow,FIELD+i,b);EnableWindow(GetDlgItem(mainWindow,FIELD+i),FXEnabled(&patch,i));}
 SetDlgItemText(mainWindow,TARGET,patch.v[FX_TARGET]?"Speaker":"PSG");
 wsprintf(b,"Tandy Effects Lab%s",(LPSTR)(dirty?" *":""));SetWindowText(mainWindow,b);
 InvalidateRect(mainWindow,NULL,FALSE);}
static int bindClock(void){char p[144];lstrcpy(p,appdir);lstrcat(p,"RAWCLK.DLL");
 module=LoadLibrary(p);if(module<32){module=0;return 0;}
 openFn=(ZERO)GetProcAddress(module,(LPSTR)21L);putFn=(PUT)GetProcAddress(module,(LPSTR)22L);
 endFn=(ZERO)GetProcAddress(module,(LPSTR)23L);startFn=(ZERO)GetProcAddress(module,(LPSTR)3L);
 closeFn=(ZERO)GetProcAddress(module,(LPSTR)5L);debugFn=(ONE)GetProcAddress(module,(LPSTR)6L);
 pauseFn=(ZERO)GetProcAddress(module,(LPSTR)14L);resumeFn=(ZERO)GetProcAddress(module,(LPSTR)15L);
 continueFn=(ZERO)GetProcAddress(module,(LPSTR)11L);
 xopenFn=(ONE)GetProcAddress(module,(LPSTR)25L);xputFn=(XPUT)GetProcAddress(module,(LPSTR)26L);statusFn=(ZERO)GetProcAddress(module,(LPSTR)28L);
 if(openFn&&putFn&&endFn&&startFn&&closeFn&&debugFn&&pauseFn&&resumeFn&&continueFn&&xopenFn&&xputFn&&statusFn)return 1;
 FreeLibrary(module);module=0;return 0;}
static int play(void){unsigned t;WORD r;static TCXBURST b;
 if(!stop())return 0;if(!module){say("RAWCLK missing: extract complete kit");return 0;}
 if(!FXCompile(&patch,&clip)){say("Invalid patch: playback refused");return 0;}
 r=clip.target?xopenFn(1):openFn();if(r){say("Sound busy/unavailable: stop other apps");return 0;}owned=1;
 if(clip.target&&statusFn()){stop();say("Speaker busy/unavailable: use kit Setup");return 0;}
 for(t=0;t<clip.count;t++){
  b.at=(DWORD)t*3579545UL/180UL;b.count=FXBytes(&clip,t,b.data);b.pitDivisor=clip.row[t].pit;
  r=clip.target?xputFn(&b):putFn((TCBURST FAR *)&b);
  if(r){stop();say("Prepare failed: playback stopped");return 0;}}
 if(endFn()||startFn()){stop();say("Clock failed: use normal sound launcher");return 0;}
 playing=1;say(clip.target?"Speaker: fixed level, pitch + gate":"PSG: exact compiled export trajectory");return 1;}
static void pauseSound(void){WORD r;if(!playing)return;
 r=paused?resumeFn():pauseFn();if(r){stop();say("Pause failed: sound released");return;}
 paused=!paused;SetDlgItemText(mainWindow,PAUSE,paused?"Resume":"Pause");say(paused?"Paused: silent, position held":"Resumed");}
static int discard(void){return !dirty||automatic||MessageBox(mainWindow,"Discard unsaved patch edits?","Effects Lab",MB_YESNO|MB_ICONQUESTION)==IDYES;}
static int filename(char *p){char name[13];GetWindowText(fileEdit,name,sizeof(name));
 if(!FXName(name)){say("Use an 8.3 name such as MYFX.FXP");return 0;}
 if(lstrlen(appdir)+lstrlen(name)>=144){say("Folder too long: move complete kit");return 0;}
 lstrcpy(p,appdir);lstrcat(p,name);return 1;}
static int savePatch(void){char path[144];int r;if(!filename(path))return 0;r=FXSave(&patch,path,0);
 if(r==3){if(!automatic&&MessageBox(mainWindow,"Replace patch? Old file becomes .BAK.","Save patch",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;r=FXSave(&patch,path,1);}
 if(r){say(r==4?"Save failed: old patch in .BAK":"Save failed: check disk or use a new name");return 0;}
 dirty=0;labels();say("Patch saved beside app; edits preserved");return 1;}
static int loadPatch(void){char path[144];FXPATCH p;int r;if(!filename(path)||!discard())return 0;
 r=FXLoad(&p,path);if(r){say("Load rejected: current patch kept");return 0;}
 if(!stop())return 0;patch=p;dirty=0;labels();say("Patch loaded; F5 to preview");return 1;}
static int exportPatch(void){char path[144];int fd,bad;FILE *f;unsigned n;
 if(!filename(path)||!FXCompile(&patch,&clip))return 0;n=lstrlen(path);lstrcpy(path+n-3,"H");
 fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_BINARY,S_IREAD|S_IWRITE);if(fd<0){say("Export exists/disk error: change name");return 0;}
 f=fdopen(fd,"wb");if(!f){close(fd);remove(path);say("Export could not open stream");return 0;}
 bad=!FXHeader(&clip,f);if(fclose(f))bad=1;if(bad){remove(path);say("Export write failed: check disk");return 0;}
 say("C header saved (.H): use supplied FXPLAY.H");return 1;}
static void adjust(int delta){if(!FXEnabled(&patch,(unsigned)selected)||!stop())return;FXAdjust(&patch,(unsigned)selected,delta);dirty=1;labels();play();}
static void preset(unsigned i){int target=patch.v[FX_TARGET];if(!discard()||!stop())return;
 FXPreset(&patch,i);patch.v[FX_TARGET]=target;dirty=1;labels();play();}
static void button(HWND w,const char *s,int x,int y,int width,int id){CreateWindow("BUTTON",s,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,width,15,w,(HMENU)id,instance,NULL);}
static void paint(HWND w){PAINTSTRUCT ps;HDC dc;char b[80];RECT r;
 dc=BeginPaint(w,&ps);GetClientRect(w,&r);r.top=147;FillRect(dc,&r,GetStockObject(WHITE_BRUSH));SelectObject(dc,GetStockObject(SYSTEM_FONT));SetBkMode(dc,TRANSPARENT);
 wsprintf(b,"%s  -/+ edit  Arrows select/change",(LPSTR)FXNames[selected]);TextOut(dc,3,148,b,lstrlen(b));
 TextOut(dc,3,160,status,lstrlen(status));EndPaint(w,&ps);}
static void about(void){stop();MessageBox(mainWindow,"Tandy Effects Lab\nCoin / laser / explosion / jump / blip\nFields: click, then -/+ or Left/Right\nUp/Down select; Tab moves controls\nF5 Play; Esc Stop; F6 Pause/Resume\nCtrl+S Save; Ctrl+O Load\n60 ticks = 1 sec. Atten step = 2dB\nSpeaker: Hold length + pitch + gate\nExport: .H + FXPLAY.H for libtandy","Effects Lab",MB_OK);}
static void nativeTest(DWORD now){static FXPATCH saved;unsigned i;RECT r;
 if(!stage&&now-testAt<2000UL)return;
 if(!stage){GetClientRect(mainWindow,&r);check("client fits",r.right>=302&&r.bottom>=174);
  for(i=0;i<5;i++){FXPreset(&patch,i);check("preset compile",FXCompile(&patch,&clip));}
  FXPreset(&patch,1);patch.v[FX_HOLD]=80;patch.v[FX_RELEASE]=30;labels();saved=patch;
  SetWindowText(fileEdit,"NATIVE.FXP");check("save",savePatch());patch.v[FX_PITCH]=200;dirty=0;
  check("load",loadPatch());check("reopened equal",!memcmp(&patch,&saved,sizeof(patch)));check("export",exportPatch());
  selected=FX_SLIDE;SendMessage(mainWindow,WM_KEYDOWN,VK_RIGHT,0L);check("keyboard edits",patch.v[FX_SLIDE]==saved.v[FX_SLIDE]+100);stop();
  patch=saved;dirty=0;check("preview starts",play()&&owned);testAt=now;stage=1;
 }else if(stage==1&&now-testAt>200UL){pauseSound();pausedSent=debugFn(2);check("pause stops clock",paused&&debugFn(20)==0);testAt=now;stage=2;
 }else if(stage==2&&now-testAt>300UL){check("pause holds",paused&&playing&&debugFn(2)==pausedSent);pauseSound();check("resume",!paused&&playing);testAt=now;stage=3;
 }else if(stage==3&&now-testAt>250UL){check("stop",stop()&&!owned&&debugFn(10)==0);patch.v[FX_TARGET]=1;
  patch.v[FX_HOLD]=80;labels();check("speaker starts",play()&&statusFn()==0);testAt=now;stage=4;
 }else if(stage==4&&now-testAt>200UL){check("speaker routed",debugFn(29)>=151&&debugFn(31));pauseSound();
  check("speaker pause stops clock",paused&&debugFn(20)==0);testAt=now;stage=5;
 }else if(stage==5&&now-testAt>1500UL){check("speaker stop releases",stop()&&debugFn(10)==0&&debugFn(31)==0);
  patch.v[FX_TARGET]=0;play();SendMessage(mainWindow,WM_ACTIVATE,0,0L);check("deactivate releases",!owned&&!playing);
  play();dirty=0;PostMessage(mainWindow,WM_CLOSE,0,0L);stage=6;}}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM a,LPARAM b){unsigned i;DWORD now;
 switch(m){case WM_CREATE:mainWindow=w;
  button(w,"Coin",2,1,57,PRESET);button(w,"Laser",61,1,57,PRESET+1);button(w,"Boom",120,1,57,PRESET+2);button(w,"Jump",179,1,57,PRESET+3);button(w,"Blip",238,1,64,PRESET+4);
  button(w,"Play",2,18,42,PLAY);button(w,"Stop",46,18,42,STOP);button(w,"Pause",90,18,48,PAUSE);button(w,"Random",140,18,55,RANDOM);button(w,"Mutate",197,18,53,MUTATE);button(w,"PSG",252,18,50,TARGET);
  fileEdit=CreateWindow("EDIT","SOUND.FXP",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,2,36,100,17,w,(HMENU)FILEEDIT,instance,NULL);SendMessage(fileEdit,EM_LIMITTEXT,12,0L);
  button(w,"Load",104,36,42,LOAD);button(w,"Save",148,36,42,SAVE);button(w,"Export",192,36,52,EXPORT);button(w,"?",246,36,18,ABOUT);button(w,"-",266,36,17,LESS);button(w,"+",285,36,17,MORE);
  for(i=0;i<FX_FIELDS;i++)button(w,"",2+(i/6)*152,56+(i%6)*15,148,FIELD+i);
  if(!fileEdit||!SetTimer(w,1,60,NULL))return -1;labels();return 0;
 case WM_COMMAND:
  if(a>=PRESET&&a<PRESET+5)preset(a-PRESET);
  else if(a>=FIELD&&a<FIELD+FX_FIELDS){selected=a-FIELD;say("Left/Right or -/+ changes selected field");}
  else if(a==PLAY)play();else if(a==STOP)stop();else if(a==PAUSE)pauseSound();
  else if(a==RANDOM||a==MUTATE){if(stop()){FXRandom(&patch,a==MUTATE);dirty=1;labels();play();}}
  else if(a==TARGET){if(stop()){patch.v[FX_TARGET]=!patch.v[FX_TARGET];dirty=1;labels();say(patch.v[FX_TARGET]?"Speaker: Hold is duration; level fixed":"PSG tone/noise + attenuation envelope");}}
  else if(a==LESS)adjust(-1);else if(a==MORE)adjust(1);else if(a==SAVE)savePatch();else if(a==LOAD)loadPatch();else if(a==EXPORT)exportPatch();else if(a==ABOUT)about();
  if(a!=FILEEDIT)SetFocus(w);return 0;
 case WM_KEYDOWN:
  if(a==VK_ESCAPE)stop();else if(a==VK_F5)play();else if(a==VK_F6)pauseSound();else if(a==VK_F1)about();
  else if(GetKeyState(VK_CONTROL)<0){if(a=='S')savePatch();else if(a=='O')loadPatch();}
  else if(a==VK_LEFT||a==VK_SUBTRACT||a==0xbd)adjust(-1);
  else if(a==VK_RIGHT||a==VK_ADD||a==0xbb)adjust(1);
  else if(a==VK_UP||a==VK_DOWN){do{selected=(selected+(a==VK_UP?11:1))%FX_FIELDS;}while(!FXEnabled(&patch,selected));labels();}
  return 0;
 case WM_TIMER:now=GetTickCount();if(owned&&playing&&!paused){
  if(debugFn(6)){stop();say("Transport error: sound released");}
  else if(debugFn(7)){stop();say("Finished: sound released");}
  else if(debugFn(1)&&!debugFn(20)&&continueFn()){stop();say("Resume error: sound released");}}
  if(automatic)nativeTest(now);return 0;
 case WM_ACTIVATE:if(!LOWORD(a))stop();return 0;
 case WM_CANCELMODE:stop();return 0;
 case WM_PAINT:paint(w);return 0;
 case WM_QUERYENDSESSION:stop();return discard();
 case WM_ENDSESSION:if(a)stop();return 0;
 case WM_CLOSE:if(!discard()||!stop())return 0;DestroyWindow(w);return 0;
 case WM_DESTROY:KillTimer(w,1);stop();PostQuitMessage(0);return 0;}
 return DefWindowProc(w,m,a,b);}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show){WNDCLASS wc;MSG msg;HWND w;unsigned n;int i;char path[144],line[80];
 if(prev){w=FindWindow("TandyEffects",NULL);if(w){ShowWindow(w,SW_RESTORE);SetActiveWindow(w);}return 0;}
 if(GetWinFlags()&WF_PMODE)return 2;instance=inst;automatic=!lstrcmp(cmd,"/test");
 if(automatic)logfile=_lcreat("C:\\EFFECTS.LOG",0);
 n=GetModuleFileName(inst,path,sizeof(path));if(!n||n>=sizeof(path))return 3;
 for(i=(int)n-1;i>=0&&path[i]!='\\';i--);if(i<0||i+13>=sizeof(path))return 3;
 if(!lstrcmpi(path+i+1,"RAWGUI.EXE")){automatic=1;if(logfile<0)logfile=_lcreat("C:\\EFFECTS.LOG",0);}
 lstrcpy(appdir,path);appdir[i+1]=0;if(!bindClock())say("Transport missing: extract complete kit");
 FXPreset(&patch,0);memset(&wc,0,sizeof(wc));wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;wc.hInstance=inst;
 wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszClassName="TandyEffects";
 if(!RegisterClass(&wc))return 4;
 w=CreateWindow("TandyEffects","Tandy Effects Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,312,200,NULL,NULL,inst,NULL);
 if(!w)return 5;ShowWindow(w,show);UpdateWindow(w);SetFocus(w);testAt=GetTickCount();
 while(GetMessage(&msg,NULL,0,0)){
  if(msg.message==WM_KEYDOWN){if(msg.wParam==VK_TAB){HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);if(next)SetFocus(next);continue;}
   if(msg.wParam==VK_ESCAPE||msg.wParam==VK_F5||msg.wParam==VK_F6||msg.wParam==VK_F1||
      (GetKeyState(VK_CONTROL)<0&&(msg.wParam=='S'||msg.wParam=='O'))||msg.hwnd==w){SendMessage(w,WM_KEYDOWN,msg.wParam,msg.lParam);continue;}}
  TranslateMessage(&msg);DispatchMessage(&msg);}
 stop();if(automatic){check("close clean",!owned&&debugFn&&debugFn(10)==0);
  wsprintf(line,"TOTAL_FAILURES=%d\r\n",failures);_lwrite(logfile,line,lstrlen(line));_lclose(logfile);}
 if(module>=32)FreeLibrary(module);if(automatic)ExitWindows(0L,0);return failures?6:0;}
