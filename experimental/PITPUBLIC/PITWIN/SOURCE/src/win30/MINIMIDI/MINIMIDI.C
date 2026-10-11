/* Mini MIDI: bounded SMF player for Win3.0 real mode, Microsoft C6 /G0. */
#define WINVER 0x0300
#include <windows.h>
#include <dos.h>
#include <direct.h>
#include "MIDIAPI.H"
#include "SMF.H"
#define PLAY 101
#define STOP 102
#define BROWSE 103
#define FILEEDIT 104
#define FILELIST 105
#define PATHBOX 106
#define PIT 107
#define PULSE 1
static HINSTANCE instance,module;
static HWND mainWindow;
static MIDIPROC midi;
static DEBUGPROC debug;
static HGLOBAL fileMem;
static BYTE FAR *fileBytes;
static unsigned fileSize;
static int pitEnabled;
static int opened,playing,timerReady,pending;
static DWORD beginTick,startTick,duration,position,maxLate,paintTick;
static SMF_EVENT nextEvent;
static unsigned sent,skipped;
static char status[32]="Choose a MIDI file";
static char detail[40]="Type 0/1, 32K max";
static char filename[144],appdir[144],chosen[144],browseOld[144];
static char FAR *browseDirs;
static int browseDrive;
static HFILE logFile=HFILE_ERROR;
static int automatic,testStage,failures,completions;
static DWORD testTickAt;
static void copyText(char *dst,const char FAR *src,unsigned cap) {unsigned i=0;if(!cap)return;while(i+1<cap&&src[i]){dst[i]=src[i];++i;}dst[i]=0;}
static void logText(char *s) {if(logFile!=HFILE_ERROR){_lwrite(logFile,s,lstrlen(s));_lwrite(logFile,"\r\n",2);}}
static void check(char *s,int ok) {char b[96];wsprintf(b,"%s %s",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)s);logText(b);if(!ok)++failures;}
static void redraw(void) {if(mainWindow)InvalidateRect(mainWindow,NULL,FALSE);}
static void progressPaint(void) {RECT r;if(!mainWindow)return;r.left=4;r.top=94;r.right=144;r.bottom=140;InvalidateRect(mainWindow,&r,FALSE);}
static void setStatus(char *s) {copyText(status,s,sizeof(status));redraw();}
static DWORD message(WORD m,DWORD v) {return midi?midi(0,m,MIDI_COOKIE,v,0L):MMSYSERR_ERROR;}
static void releaseDriver(void) {
 if(opened){
  check("reset success",message(MODM_RESET,0L)==0L);
  check("reset voices silent",debug&&debug(0)==0);
  check("close success",message(MODM_CLOSE,0L)==0L);
  opened=0;check("owner released",debug&&debug(1)==0);
 }
}
static void freeFile(void) {smf_close();if(fileMem){if(fileBytes)GlobalUnlock(fileMem);GlobalFree(fileMem);}fileBytes=NULL;fileMem=0;fileSize=0;}
static void stop(void) {playing=0;pending=0;releaseDriver();freeFile();setStatus("Stopped / muted");}
static int acquireDriver(void) {
 TOPEN desc;DWORD cookie=0,r;TCAPS caps;
 if(!midi){setStatus("Driver missing");return 0;}
 desc.hMidi=0;desc.dwCallback=0L;desc.dwInstance=0L;
 if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||caps.wVoices!=3||caps.vDriverVersion<0x0101){setStatus("Wrong driver");return 0;}
 r=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&desc,0L);
 if(r){setStatus(r==MMSYSERR_ALLOCATED?"PSG is busy":"Driver open failed");return 0;}
 opened=1;if(cookie!=MIDI_COOKIE){releaseDriver();setStatus("Bad driver cookie");return 0;}
 if(pitEnabled && midi(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1L,1L)) {
  releaseDriver();setStatus("PIT unavailable");return 0;
 }
 return 1;
}
static int loadFile(void) {
 HFILE f;OFSTRUCT of;LONG length;unsigned n;HCURSOR old;
 freeFile();GetWindowText(GetDlgItem(mainWindow,FILEEDIT),filename,sizeof(filename));
 f=OpenFile(filename,&of,OF_READ|OF_SHARE_DENY_WRITE);
 if(f==HFILE_ERROR){setStatus("Cannot open file");return 0;}
 length=_llseek(f,0L,2);_llseek(f,0L,0);
 if(length<14L||length>32768L){_lclose(f);setStatus("File size rejected");return 0;}
 fileSize=(unsigned)length;fileMem=GlobalAlloc(GMEM_MOVEABLE,fileSize);
 if(!fileMem){_lclose(f);setStatus("Out of memory");return 0;}
 fileBytes=(BYTE FAR *)GlobalLock(fileMem);
 if(!fileBytes){_lclose(f);freeFile();setStatus("Out of memory");return 0;}
 n=_lread(f,(LPSTR)fileBytes,fileSize);_lclose(f);
 if(n!=fileSize){freeFile();setStatus("File read failed");return 0;}
 old=SetCursor(LoadCursor(NULL,IDC_WAIT));
 n=smf_open(fileBytes,fileSize);SetCursor(old);
 if(!n){copyText(detail,(LPSTR)smf_error(),sizeof(detail));freeFile();setStatus("Invalid MIDI file");return 0;}
 duration=smf_duration_ms();position=0;
 wsprintf(detail,"%u events",smf_event_count());return 1;
}
/* A deliberately restricted PSG translation. Never pretend GM timbres exist. */
static int emit(SMF_EVENT *e) {
 unsigned s=e->status&0xf0,c=e->status&15;DWORD r;char b[80];
 if((s==0x90&&e->data2)&&((c==9&&(e->data1<35||e->data1>81))||(c!=9&&(e->data1<45||e->data1>96)))) {++skipped;return 1;}
 if(s!=0x80&&s!=0x90&&!(s==0xb0&&(e->data1==120||e->data1==123))){++skipped;return 1;}
 r=message(MODM_DATA,(DWORD)e->status|((DWORD)e->data1<<8)|((DWORD)e->data2<<16));
 if(r){stop();setStatus("MIDI send failed");return 0;}
 ++sent;
 if(automatic){wsprintf(b,"EVENT %lu %u %u %u",e->ms,(unsigned)e->status,(unsigned)e->data1,(unsigned)e->data2);logText(b);}
 return 1;
}
static void pump(DWORD now) {
 int count=0,r;DWORD elapsed,late;
 if(!playing)return;elapsed=now-startTick;position=elapsed;if(position>duration)position=duration;
 if(pending&&elapsed>=nextEvent.ms&&elapsed-nextEvent.ms>1000UL){stop();setStatus("Timing stalled");return;}
 while(pending&&elapsed>=nextEvent.ms&&count<64){
  late=elapsed-nextEvent.ms;if(late>maxLate)maxLate=late;
  if(!emit(&nextEvent))return;
  ++count;r=smf_next(&nextEvent);
  if(r<0){stop();setStatus("MIDI parse failed");return;}pending=r;
  elapsed=GetTickCount()-startTick;
 }
 if(!pending&&elapsed>=duration){playing=0;releaseDriver();freeFile();position=duration;++completions;setStatus("Finished / muted");}
 if(now-paintTick>=500UL){paintTick=now;progressPaint();}
}
static void play(void) {
 stop();sent=skipped=0;maxLate=0;duration=position=0;lstrcpy(detail,"");
 if(!loadFile()||!acquireDriver())return;
 pending=smf_next(&nextEvent);if(pending<0){stop();setStatus("MIDI parse failed");return;}
 setStatus("Playing MIDI");UpdateWindow(mainWindow);
 startTick=GetTickCount();paintTick=startTick;playing=1;pump(startTick);
}
static void word(BYTE FAR **p,unsigned v){*(*p)++=(BYTE)v;*(*p)++=(BYTE)(v>>8);}
static void dword(BYTE FAR **p,DWORD v){word(p,(unsigned)v);word(p,(unsigned)(v>>16));}
static int smallDialog(HWND w,char *title,int height,FARPROC fn) {
 HGLOBAL h;BYTE FAR *p;FARPROC proc;DWORD units=GetDialogBaseUnits();int result=IDCANCEL;
 h=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,64);if(!h)return IDCANCEL;p=(BYTE FAR *)GlobalLock(h);if(!p){GlobalFree(h);return IDCANCEL;}
 dword(&p,WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME);*p++=0;
 word(&p,0);word(&p,0);word(&p,128*4/LOWORD(units));word(&p,height*8/HIWORD(units));*p++=0;*p++=0;
 while(*title)*p++=*title++;*p++=0;GlobalUnlock(h);
 proc=MakeProcInstance(fn,instance);if(proc){result=DialogBoxIndirect(instance,h,w,(DLGPROC)proc);FreeProcInstance(proc);}GlobalFree(h);return result;
}
static int listfiles(HWND w,char *where) {
 char pattern[144],cwd[144],display[15];int n,drive;
 if(where[0]&&where[1]==':'){
  drive=where[0];if(drive>='a'&&drive<='z')drive-=32;drive-='A'-1;
  if(drive<1||drive>26)return 0;
  if(!browseDirs[(drive-1)*144]){if(!_getdcwd(drive,cwd,sizeof(cwd)))return 0;lstrcpy(browseDirs+(drive-1)*144,cwd);}
 }
 if(lstrlen(where)>135)return 0;lstrcpy(pattern,where);n=lstrlen(pattern);
 if(n&&pattern[n-1]!='\\'&&pattern[n-1]!=':')lstrcat(pattern,"\\");lstrcat(pattern,"*.MID");
 if(!DlgDirList(w,pattern,FILELIST,0,DDL_DIRECTORY|DDL_DRIVES|DDL_READONLY|DDL_ARCHIVE))return 0;
 if(getcwd(cwd,sizeof(cwd))){n=lstrlen(cwd);if(n>14){lstrcpy(display,"...");lstrcat(display,cwd+n-11);SetDlgItemText(w,PATHBOX,display);}else SetDlgItemText(w,PATHBOX,cwd);}
 SendDlgItemMessage(w,FILELIST,LB_SETCURSEL,0,0L);return 1;
}
BOOL FAR PASCAL BrowseProc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
 HWND child;int i,n,ww,hh;char item[144],cwd[144];RECT r,c;
 if(m==WM_INITDIALOG){
  GetWindowRect(w,&r);GetClientRect(w,&c);ww=128+r.right-r.left-c.right;hh=148+r.bottom-r.top-c.bottom;
  SetWindowPos(w,NULL,(GetSystemMetrics(SM_CXSCREEN)-ww)/2,(GetSystemMetrics(SM_CYSCREEN)-hh)/2,ww,hh,SWP_NOZORDER);
  CreateWindow("STATIC","",WS_CHILD|WS_VISIBLE|WS_BORDER|SS_LEFT,6,4,116,16,w,(HMENU)PATHBOX,instance,NULL);
  CreateWindow("LISTBOX","",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|WS_VSCROLL|LBS_NOTIFY,6,25,116,94,w,(HMENU)FILELIST,instance,NULL);
  CreateWindow("BUTTON","&Open",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,6,124,54,20,w,(HMENU)IDOK,instance,NULL);
  CreateWindow("BUTTON","&Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP,68,124,54,20,w,(HMENU)IDCANCEL,instance,NULL);
  for(i=0;i<4;i++){child=GetDlgItem(w,i==0?PATHBOX:i==1?FILELIST:i==2?IDOK:IDCANCEL);if(!child){EndDialog(w,IDCANCEL);return TRUE;}SendMessage(child,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);}
  if(!listfiles(w,appdir)){EndDialog(w,IDCANCEL);return TRUE;}SetFocus(GetDlgItem(w,FILELIST));
  if(automatic==5)SetTimer(w,9,600,NULL);return FALSE;
 }
 if(m==WM_TIMER&&wp==9){KillTimer(w,9);check("browse lists files",SendDlgItemMessage(w,FILELIST,LB_GETCOUNT,0,0L)>0);
  if(testStage==2){n=(int)SendDlgItemMessage(w,FILELIST,LB_FINDSTRING,(WPARAM)-1,(LPARAM)(LPSTR)"TINY.MID");check("browse finds MIDI",n!=LB_ERR);if(n!=LB_ERR){SendDlgItemMessage(w,FILELIST,LB_SETCURSEL,n,0L);SendMessage(w,WM_COMMAND,IDOK,0L);return TRUE;}}
  EndDialog(w,IDCANCEL);return TRUE;}
 if(m==WM_COMMAND&&(wp==IDOK||(wp==FILELIST&&HIWORD(lp)==LBN_DBLCLK))){
  if(SendDlgItemMessage(w,FILELIST,LB_GETCURSEL,0,0L)==LB_ERR)return TRUE;
  if(DlgDirSelect(w,item,FILELIST)){if(!listfiles(w,item))SetDlgItemText(w,PATHBOX,"Cannot list");return TRUE;}
  if(!getcwd(cwd,sizeof(cwd)))return TRUE;n=lstrlen(cwd);if(n+lstrlen(item)+2>sizeof(chosen))return TRUE;
  lstrcpy(chosen,cwd);if(n&&cwd[n-1]!='\\')lstrcat(chosen,"\\");lstrcat(chosen,item);EndDialog(w,IDOK);return TRUE;
 }
 if((m==WM_COMMAND&&wp==IDCANCEL)||m==WM_CLOSE){EndDialog(w,IDCANCEL);return TRUE;}return FALSE;
}
static void browse(void) {
 int result,i,restored=1;unsigned drive;HGLOBAL h;char cwd[144];
 stop();chosen[0]=0;if(!getcwd(browseOld,sizeof(browseOld))){setStatus("Cannot read path");return;}
 _dos_getdrive(&drive);browseDrive=(int)drive;h=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,26*144);
 if(!h){setStatus("Out of memory");return;}browseDirs=(char FAR *)GlobalLock(h);
 if(!browseDirs){GlobalFree(h);return;}lstrcpy(browseDirs+(browseDrive-1)*144,browseOld);
 result=smallDialog(mainWindow,"MIDI files",148,(FARPROC)BrowseProc);
 for(i=0;i<26;i++)if(browseDirs[i*144]){lstrcpy(cwd,browseDirs+i*144);if(chdir(cwd))restored=0;}
 if(_chdrive(browseDrive)||chdir(browseOld))restored=0;GlobalUnlock(h);GlobalFree(h);browseDirs=NULL;
 if(!restored){setStatus("Path restore failed");return;}
 if(result==IDOK){SetDlgItemText(mainWindow,FILEEDIT,chosen);setStatus("Ready to play");}
 if(automatic==5){getcwd(cwd,sizeof(cwd));check("browse restores directory",lstrcmp(cwd,browseOld)==0);if(testStage==1)check("cancel preserves filename",!chosen[0]);else check("browse selected file",chosen[0]!=0);}
}
static void drawLine(HDC dc,int y,char *s) {char b[19];unsigned i;for(i=0;i<18&&s[i];i++)b[i]=s[i];while(i<18)b[i++]=' ';b[18]=0;TextOut(dc,4,y,b,18);}
static void paint(HWND w) {
 HDC dc;PAINTSTRUCT ps;RECT r;char line[64];int width;
 dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));
 drawLine(dc,78,status);wsprintf(line,"%lu / %lu sec",position/1000UL,duration/1000UL);drawLine(dc,94,line);
 r.left=4;r.top=111;r.right=144;r.bottom=120;FrameRect(dc,&r,GetStockObject(BLACK_BRUSH));
 r.left=5;r.top=112;r.right=143;r.bottom=119;FillRect(dc,&r,GetStockObject(WHITE_BRUSH));
 width=duration?(int)((position*138UL)/duration):0;r.left=5;r.top=112;r.right=5+width;r.bottom=119;FillRect(dc,&r,GetStockObject(BLACK_BRUSH));
 wsprintf(line,"S%u Skip%u",sent,skipped);drawLine(dc,126,line);
 copyText(line,detail,19);drawLine(dc,142,line);
 if(lstrlen(detail)>18){copyText(line,detail+18,19);drawLine(dc,158,line);}else drawLine(dc,158,pitEnabled?"PIT: MIDI chan 16":"3 tones + noise");EndPaint(w,&ps);
}
static void button(HWND w,char *label,int x,int y,int width,int id) {
 HWND b=CreateWindow("BUTTON",label,WS_CHILD|WS_VISIBLE|WS_TABSTOP,x,y,width,20,w,(HMENU)id,instance,NULL);
 if(b)SendMessage(b,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);
}
static void testTick(DWORD now) {
 if(automatic==10) {
  if(testStage==0 && now-beginTick>300UL) {
   check("PIT and PSG/noise active",opened&&playing&&debug&&debug(0)==31);
   SendMessage(mainWindow,WM_KEYDOWN,VK_ESCAPE,0L);
   check("Escape clears PIT owner",!opened&&!playing&&debug&&debug(0)==0);
   play();check("PIT replay starts",opened&&playing&&debug&&debug(0)==31);
   testStage=1;testTickAt=now;
  } else if(testStage==1 && now-testTickAt>300UL) {
   SendMessage(mainWindow,WM_ACTIVATE,0,0L);
   check("focus clears PIT owner",!opened&&!playing&&debug&&debug(0)==0);
   SendMessage(mainWindow,WM_ACTIVATE,1,0L);
   check("focus never revives stale notes",!playing);
   play();testStage=2;
  } else if(testStage==2 && completions) {
   check("PIT completion clears owner",!opened&&!playing&&debug&&debug(0)==0);
   PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=3;
  }
  return;
 }
 if((automatic==1||automatic==6)&&completions){check("normal playback completed",sent>0);PostMessage(mainWindow,WM_CLOSE,0,0L);automatic=9;}
 if(automatic==6&&testStage==0&&now-beginTick>700UL){check("duplicate activates",WinExec("C:\\MINIMIDI.EXE",SW_SHOWNORMAL)>=32);check("duplicate preserves ownership",opened&&debug&&debug(1)==1);check("intruder starts",WinExec("C:\\MIDIINTR.EXE",SW_SHOWNORMAL)>=32);testStage=1;}
 if(automatic==7&&testStage==0&&now-beginTick>400UL){
  check("SOUND lease acquired",OpenSound()>=0);play();check("busy ownership refuses playback",!playing&&!opened);CloseSound();play();check("retry after owner release works",playing&&opened);testStage=1;
 }
 if(automatic==7&&testStage==1&&completions){PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=2;}
 if(automatic==8&&testStage==0&&now-beginTick>400UL){play();check("missing driver refuses playback",!playing&&!opened);PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=1;}
 if(automatic==2&&testStage==0&&now-beginTick>500UL){play();play();check("repeat play restarts",playing&&position==0);testStage=1;testTickAt=now;}
 else if(automatic==2&&testStage==1&&now-testTickAt>700UL){stop();stop();check("repeat stop silent",!playing&&!opened&&debug&&debug(0)==0);play();check("replay opens owner",playing&&opened);testStage=2;testTickAt=now;}
 else if(automatic==2&&testStage==2&&now-testTickAt>700UL){PostMessage(mainWindow,WM_CLOSE,0,0L);automatic=9;}
 if(automatic==3&&now-beginTick>700UL){check("closing while playing",playing&&opened);PostMessage(mainWindow,WM_CLOSE,0,0L);automatic=9;}
 if(automatic==4&&testStage==0&&now-beginTick>400UL){play();check("malformed file rejected before ownership",!playing&&!opened);SetDlgItemText(mainWindow,FILEEDIT,"C:\\NOFILE.MID");play();check("missing file rejected",!playing&&!opened);PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=1;}
 if(automatic==5&&testStage==0&&now-beginTick>400UL){testStage=1;browse();testStage=2;browse();play();check("selected MIDI plays",playing);testStage=3;}
 if(automatic==5&&testStage==3&&completions){PostMessage(mainWindow,WM_CLOSE,0,0L);testStage=4;}
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
 DWORD now;HWND edit;
 switch(m){
 case WM_CREATE:
  mainWindow=w;edit=CreateWindow("EDIT",filename,WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,4,4,140,18,w,(HMENU)FILEEDIT,instance,NULL);
  if(!edit)return -1;SendMessage(edit,EM_LIMITTEXT,143,0L);SendMessage(edit,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);
  button(w,"Browse",4,27,68,BROWSE);button(w,"PIT off",76,27,68,PIT);button(w,"Play",4,52,68,PLAY);button(w,"Stop",76,52,68,STOP);
  if(!GetDlgItem(w,BROWSE)||!GetDlgItem(w,PLAY)||!GetDlgItem(w,STOP))return -1;
  timerReady=SetTimer(w,PULSE,55,NULL)!=0;if(!timerReady)return -1;beginTick=GetTickCount();
  if(automatic==1||automatic==3||automatic==6||automatic==10)PostMessage(w,WM_COMMAND,PLAY,0L);return 0;
 case WM_COMMAND:
  if(wp==PIT) {
   stop();pitEnabled=!pitEnabled;
   SetWindowText(GetDlgItem(w,PIT),pitEnabled?"PIT ch16":"PIT off");
   setStatus(pitEnabled?"Channel 16 is PIT":"PIT disabled");
  }
  if(wp==PLAY)play();if(wp==STOP)stop();if(wp==BROWSE)browse();return 0;
 case WM_KEYDOWN:if(wp==VK_ESCAPE)stop();return 0;
 case WM_ACTIVATE:if(pitEnabled && LOWORD(wp)==0)stop();return 0;
 case WM_TIMER:if(wp==PULSE){now=GetTickCount();pump(now);if(automatic)testTick(now);}return 0;
 case WM_PAINT:paint(w);return 0;
 case WM_QUERYENDSESSION:return TRUE;
 case WM_ENDSESSION:if(wp)stop();return 0;
 case WM_CLOSE:stop();DestroyWindow(w);return 0;
 case WM_DESTROY:mainWindow=0;KillTimer(w,PULSE);timerReady=0;stop();PostQuitMessage(0);return 0;
 }
 return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show) {
 WNDCLASS wc;HWND w;MSG msg;OFSTRUCT of;char path[144],line[96];int i,result=0;
 if(previous){w=FindWindow("TandyMiniMIDI",NULL);if(w){ShowWindow(w,SW_RESTORE);BringWindowToTop(w);}return 1;}
 if(GetWinFlags()&WF_PMODE)return 2;instance=inst;
 if(lstrcmp(cmd,"/test")==0)automatic=1;if(lstrcmp(cmd,"/uitest")==0)automatic=2;
 if(lstrcmp(cmd,"/closetest")==0)automatic=3;if(lstrcmp(cmd,"/failtest")==0)automatic=4;if(lstrcmp(cmd,"/browsetest")==0)automatic=5;if(lstrcmp(cmd,"/ownertest")==0)automatic=6;if(lstrcmp(cmd,"/busytest")==0)automatic=7;if(lstrcmp(cmd,"/drivertest")==0)automatic=8;
 if(lstrcmp(cmd,"/pittest")==0){automatic=10;pitEnabled=1;}
 if(automatic)logFile=_lcreat("C:\\MINIMIDI.LOG",0);
 appdir[0]=0;i=GetModuleFileName(inst,appdir,sizeof(appdir));appdir[143]=0;
 if(i<=0||i>=143){if(logFile!=HFILE_ERROR)_lclose(logFile);return 3;}for(i=lstrlen(appdir)-1;i>=0;--i)if(appdir[i]=='\\')break;if(i<0||i>130){if(logFile!=HFILE_ERROR)_lclose(logFile);return 3;}appdir[i+1]=0;
 lstrcpy(filename,appdir);lstrcat(filename,automatic==4?"BAD.MID":"TINY.MID");
 if(!automatic&&cmd[0]){copyText(filename,cmd,sizeof(filename));if(filename[0]=='\"'){i=lstrlen(filename);if(i>1&&filename[i-1]=='\"'){filename[i-1]=0;for(i=0;filename[i+1];i++)filename[i]=filename[i+1];filename[i]=0;}}}
 lstrcpy(path,appdir);lstrcat(path,"MIDIMAP.DRV");module=0;
 if(OpenFile(path,&of,OF_EXIST)!=HFILE_ERROR)module=LoadLibrary(path);
 if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}
 if(!midi)lstrcpy(status,"Driver missing");
 wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;wc.cbClsExtra=0;wc.cbWndExtra=0;wc.hInstance=inst;
 wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszMenuName=NULL;wc.lpszClassName="TandyMiniMIDI";
 if(!RegisterClass(&wc)){result=4;goto cleanup;}
 w=CreateWindow("TandyMiniMIDI","Mini MIDI",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,NULL,NULL,inst,NULL);
 if(!w){result=5;goto cleanup;}ShowWindow(w,show);UpdateWindow(w);
 while(GetMessage(&msg,NULL,0,0)){
  if(msg.message==WM_KEYDOWN&&msg.wParam==VK_TAB){HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);if(next)SetFocus(next);continue;}
  if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){stop();continue;}
  TranslateMessage(&msg);DispatchMessage(&msg);
 }
cleanup:
 stop();freeFile();check("normal exit has no owner",!opened);if(module>=32)FreeLibrary(module);
 wsprintf(line,"TOTAL_FAILURES=%d SENT=%u SKIPPED=%u MAX_LATE_MS=%lu",failures,sent,skipped,maxLate);logText(line);if(logFile!=HFILE_ERROR)_lclose(logFile);
 if(automatic)ExitWindows(0L,0);return result?result:(failures?6:0);
}
