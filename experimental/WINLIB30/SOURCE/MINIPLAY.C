/* Original small Windows3.0 MIDI UI using the established app-local MCI API.
   This fallback is separate from original Media Player compatibility proof. */
#define WINVER 0x0300
#include <windows.h>
#include "LWAUDIO.H"
#include "FMTAPP.H"
#include "PLLIST.H"
#include "LTWUI.H"
#include "CDLG.H"
static WORD rawKind,formatKind,formatPaused;
#define PAUSE 109
static void caption(char *);
static WORD closeFile(void);
static void serviceForeground(void);
static void playlistFeedback(char *);
#define PLAY 101
#define STOP 102
#define FILEBOX 103
#define LABEL 104
#define CLOCKBOX 105
#define PLAYLIST 106
#define ADD 107
#define REMOVE 110
#define PLAYLIST_STATUS 111
#define BANK_BUTTON 112
typedef DWORD (FAR PASCAL *CMD)(WORD,WORD,DWORD,DWORD);
typedef WORD (FAR PASCAL *ONE)(WORD);
typedef BOOL (FAR PASCAL *BROWSEOPEN)(LPOPENFILENAME);
typedef DWORD (FAR PASCAL *BROWSEERROR)(void);
#ifdef PAINT_TRACE
static void trace(char *s){HFILE f=_lopen("C:\\PAINT.LOG",OF_WRITE);if(f==HFILE_ERROR)f=_lcreat("C:\\PAINT.LOG",0);_llseek(f,0L,2);_lwrite(f,s,lstrlen(s));_lwrite(f,"\r\n",2);_lclose(f);}
#else
#define trace(s)
#endif
typedef WORD(FAR PASCAL *BANKSET)(WORD,WORD);
static CMD command;static ONE debug;static BANKSET setInstrumentBank;
static WORD selectedBank;static char bankIni[144];static HWND bankButton;
typedef WORD (FAR PASCAL *PUMP)(void);static PUMP foregroundPump;
static DWORD lastService;static WORD foregroundCalls,clockRedraws,lastAck;
static HMODULE module,browseModule;static HWND mainWindow,fileBox,label,clockBox,listBox,trackBox,playlistStatus;
static HBRUSH panelBrush,displayBrush,selectBrush;static HPEN lightPen,darkPen;
static HFONT compactFont;static COLORREF ink,panelColor,lightColor;static WORD charWidth=8,charHeight=8;
static WORD lastSecond=0xffff,compactMode;
static WORD uiDown,uiUp,uiCancel,uiPressed,uiAfterRelease,uiAdd,uiBrowseStep;
static DWORD uiBrowseError;
static PLLModel playlist;
static char activeName[144],stateText[144]="Ready";
static RECT displayRect;
static WORD device,playing,automatic,phase,checks,failures,successes,aborts;
static WORD opening,openCancelled,closeAfterOpen;static DWORD openTick;
static DWORD phaseAt;static HFILE log=HFILE_ERROR;
static char directory[144],filename[144],text[180];
static MCOPEN op;static MCGEN gp;static MCSTATUS st;
static BROWSEOPEN browseOpen;static BROWSEERROR browseError;
static char browsePath[128],browseDirectory[144],browseFileTitle[128],browseDll[144];
static char browseFilter[]="Supported sound files\0*.MID;*.VGM;*.VGZ;*.MOD;*.TND;*.SND\0MIDI files (*.MID)\0*.MID\0VGM files (*.VGM)\0*.VGM\0VGZ files (*.VGZ)\0*.VGZ\0MOD files (*.MOD)\0*.MOD\0Tandy files (*.TND)\0*.TND\0Raw sound files (*.SND)\0*.SND\0All files (*.*)\0*.*\0\0";
static OPENFILENAME browseInfo;static OFSTRUCT browseRead;
static void serviceForeground(void)
{
 DWORD now=GetTickCount();
 if(playing && now-lastService>=55UL){
  lastService=now;
  if(formatKind){WORD r=FmtPump();if(r!=1){++successes;if(!closeFile())caption(r==0?"Done":FmtReason());}}
  else if(rawKind){WORD r=RawPump();if(r!=1){++successes;if(!closeFile())caption(r==0?"Done":"Clock limit");}}
  else if(foregroundPump)foregroundPump();++foregroundCalls;
 }
}
static void freeSkin(void)
{
 if(selectBrush)DeleteObject(selectBrush);if(panelBrush)DeleteObject(panelBrush);if(displayBrush)DeleteObject(displayBrush);
 if(lightPen)DeleteObject(lightPen);if(darkPen)DeleteObject(darkPen);
}
static void logText(void)
{
 if(log!=HFILE_ERROR){_lwrite(log,text,lstrlen(text));_lclose(log);log=_lopen("C:\\MINIPLAY.LOG",OF_WRITE);_llseek(log,0L,2);}
}
static void check(char *name,WORD yes)
{
 if(log==HFILE_ERROR)return;++checks;if(!yes)++failures;
 wsprintf(text,"CHECK %s got=%u want=1\r\n",(LPSTR)name,yes);logText();
}
static const char *baseName(const char *name)
{
 WORD i,last=0;for(i=0;name[i];++i)if(name[i]=='\\' || name[i]=='/')last=i+1;
 return name+last;
}
static void caption(char *s)
{
 if(lstrcmp(stateText,s)){lstrcpy(stateText,s);LtwSetText(label,s);}
}
/* The bank is a foreground MIDI preference, not another sound provider. */
static void bankCaption(void)
{
 LtwSetText(bankButton,compactMode?(selectedBank?"Chamber":"Default"):
     (selectedBank?"MIDI: Chamber":"MIDI: Default"));
}
static void chooseBank(void)
{
 WORD wanted=selectedBank^1;
 if(device&&setInstrumentBank(device,wanted)){caption("Bank unavailable");return;}
 selectedBank=wanted;bankCaption();
 if(!WritePrivateProfileString("FauxBlaster","Bank",wanted?"1":"0",bankIni))
  caption("Cannot save bank");
}
/* Decode/preflight run in the foreground. Keep painting and cancellation
   responsive while deferring destructive commands until the open unwinds. */
static WORD formatProgress(WORD phase,DWORD done,DWORD total)
{
 MSG msg;DWORD now;WORD count=0,percent;char status[40];
 if(openCancelled)return 0;now=GetTickCount();
 if(now-openTick<110UL)return 1;openTick=now;
 if(phase==FMT_INFLATE){
  percent=total?(WORD)(done>=total?100:(done*100UL)/total):0;
  wsprintf(status,"Opening %u%%",percent);caption(status);
 }else caption("Checking file");
 UpdateWindow(label);
 while(count++<16&&PeekMessage(&msg,0,0,0,PM_REMOVE)){
  if(msg.message==WM_QUIT){openCancelled=closeAfterOpen=1;
   PostQuitMessage(msg.wParam);break;}
  if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){openCancelled=1;continue;}
  if(msg.message==WM_KEYDOWN&&msg.wParam==VK_TAB&&
      (msg.hwnd==mainWindow||IsChild(mainWindow,msg.hwnd)))
   SetFocus(GetNextDlgTabItem(mainWindow,GetFocus(),GetKeyState(VK_SHIFT)<0));
  else {TranslateMessage(&msg);DispatchMessage(&msg);}
 }
 return !openCancelled;
}
static void trackName(void)
{
 static char b[144];WORD limit,n;static RECT r;GetClientRect(trackBox,&r);
 limit=(r.right-r.left)/(charWidth?charWidth:8);if(limit>143)limit=143;
 lstrcpy(b,baseName(activeName));n=lstrlen(b);
 if(n>limit && limit>3){b[limit-3]='.';b[limit-2]='.';b[limit-1]='.';b[limit]=0;}
 SetWindowText(trackBox,b);
}
/* Compose one control in a small color DDB, then copy its rectangle once.
   This bounds expensive screen flushes in the accepted software driver. */
static HDC screenDC,bufferDC;static HBITMAP bufferBitmap,previousBitmap;
static HDC beginSurface(HDC dc,RECT *r)
{
 screenDC=dc;bufferDC=CreateCompatibleDC(dc);bufferBitmap=0;previousBitmap=0;
 if(bufferDC){bufferBitmap=CreateCompatibleBitmap(dc,r->right,r->bottom);
  if(bufferBitmap){previousBitmap=SelectObject(bufferDC,bufferBitmap);return bufferDC;}
  DeleteDC(bufferDC);bufferDC=0;
 }return dc;
}
static void endSurface(RECT *r)
{
 if(bufferDC){BitBlt(screenDC,0,0,r->right,r->bottom,bufferDC,0,0,SRCCOPY);
  SelectObject(bufferDC,previousBitmap);DeleteObject(bufferBitmap);DeleteDC(bufferDC);bufferDC=0;
 }
}
static void bevel(HDC dc,RECT *r,WORD sunken)
{
 HPEN old;old=SelectObject(dc,sunken?darkPen:lightPen);
 MoveTo(dc,r->left,r->bottom-1);LineTo(dc,r->left,r->top);LineTo(dc,r->right-1,r->top);
 SelectObject(dc,sunken?lightPen:darkPen);LineTo(dc,r->right-1,r->bottom-1);LineTo(dc,r->left,r->bottom-1);SelectObject(dc,old);
}
static void layout(HWND w)
{
 static RECT r;int width,height,bw,bw2,headerY,listY;GetClientRect(w,&r);width=r.right;height=r.bottom;
 compactMode=width<300;
 displayRect.left=4;displayRect.top=16;displayRect.right=width-4;displayRect.bottom=54;
 MoveWindow(trackBox,8,20,width-16,12,TRUE);
 MoveWindow(clockBox,8,36,52,12,TRUE);MoveWindow(label,64,36,width-72,12,TRUE);
 MoveWindow(fileBox,4,58,width-8,14,TRUE);
 if(!compactMode){
  bw=(width-24)/5;
  MoveWindow(GetDlgItem(w,PLAY),4,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,STOP),8+bw,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,PAUSE),12+2*bw,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,ADD),16+3*bw,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,REMOVE),20+4*bw,76,bw,22,TRUE);
  headerY=104;listY=118;
 }else{
  bw=(width-16)/3;bw2=(width-12)/2;
  MoveWindow(GetDlgItem(w,PLAY),4,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,STOP),8+bw,76,bw,22,TRUE);
  MoveWindow(GetDlgItem(w,PAUSE),12+2*bw,76,width-16-2*bw,22,TRUE);
  MoveWindow(GetDlgItem(w,ADD),4,101,bw2,22,TRUE);
  MoveWindow(GetDlgItem(w,REMOVE),8+bw2,101,width-12-bw2,22,TRUE);
  headerY=128;listY=142;
 }
 bw=compactMode?72:116;
 MoveWindow(playlistStatus,4,headerY,width-12-bw,12,TRUE);
 MoveWindow(bankButton,width-4-bw,headerY,bw,14,TRUE);bankCaption();playlistFeedback(0);
 MoveWindow(listBox,4,listY,width-8,height-listY-4,TRUE);
 trackName();InvalidateRect(w,0,FALSE);
}
static WORD listRows(void)
{
 static RECT r;WORD rows;GetClientRect(listBox,&r);rows=charHeight?(WORD)(r.bottom/charHeight):1;if(!rows)rows=1;return rows;
}
static void playlistFeedback(char *note)
{
 if(compactMode){wsprintf(text,"%u/32",playlist.count);if(note&&note[0])caption(note);}
 else if(note&&note[0])wsprintf(text,"Playlist %u/32 %s",playlist.count,(LPSTR)note);
 else wsprintf(text,"Playlist %u/32",playlist.count);
 LtwSetText(playlistStatus,text);
}
static void selectedPathChanged(void)
{
 const char *p=PLLGet(&playlist,playlist.selected);
 if(p){LtwSetText(fileBox,p);if(!playing){lstrcpy(activeName,(LPSTR)p);trackName();}}
 else {LtwSetText(fileBox,"");if(!playing){activeName[0]=0;trackName();}}
}
static WORD findPath(const char *path)
{
 WORD i;const char *p;for(i=0;i<playlist.count;++i){p=PLLGet(&playlist,i);if(p&&!lstrcmpi((LPSTR)p,(LPSTR)path))return i;}return PLL_NONE;
}
static void rebuildList(void)
{
 WORD i;LONG result;LTWLISTPAINT batch;const char *p;batch.list=0;batch.depth=0;LtwListBegin(&batch,listBox);
 SendMessage(listBox,LB_RESETCONTENT,0,0L);
 for(i=0;i<playlist.count;++i){p=PLLGet(&playlist,i);if(p){
  result=SendMessage(listBox,LB_ADDSTRING,0,(LONG)(LPSTR)baseName(p));
  if(result==LB_ERR||result==LB_ERRSPACE){while(playlist.count>i)PLLRemove(&playlist,playlist.count-1,1);break;}
 }}
 if(playlist.selected<playlist.count)SendMessage(listBox,LB_SETCURSEL,playlist.selected,0L);
 LtwListEnd(&batch,listBox);playlistFeedback(0);
}
static void addPath(char *path)
{
 PLLAddStatus status;WORD i;LONG result;LTWLISTPAINT batch;const char *p;
 status=PLLAdd(&playlist,path);
 if(status==PLL_ADD_DUPLICATE){i=findPath(path);if(i<playlist.count){PLLSelect(&playlist,i,listRows());SendMessage(listBox,LB_SETCURSEL,i,0L);selectedPathChanged();}playlistFeedback("Already listed");return;}
 if(status==PLL_ADD_FULL){playlistFeedback("Full");return;}
 if(status!=PLL_ADD_ADDED){playlistFeedback("Invalid path");return;}
 i=(WORD)(playlist.count-1);p=PLLGet(&playlist,i);batch.list=0;batch.depth=0;LtwListBegin(&batch,listBox);
 result=SendMessage(listBox,LB_ADDSTRING,0,(LONG)(LPSTR)baseName(p));
 if(result==LB_ERR || result==LB_ERRSPACE){PLLRemove(&playlist,i,listRows());LtwListEnd(&batch,listBox);playlistFeedback("List unavailable");return;}
 PLLSelect(&playlist,i,listRows());SendMessage(listBox,LB_SETCURSEL,i,0L);
 LtwListEnd(&batch,listBox);selectedPathChanged();playlistFeedback(0);
}
static WORD openBrowser(void)
{
 WORD i,n,last,dlg;DWORD err;HFILE h;
 uiBrowseStep=1;uiBrowseError=0;
 lstrcpy(browseDll,directory);lstrcat(browseDll,"LWFILE.DLL");
 browseModule=LoadLibrary(browseDll);if(browseModule<32){uiBrowseError=browseModule;playlistFeedback("Browser unavailable");return 0;}
 uiBrowseStep=2;
 browseOpen=(BROWSEOPEN)GetProcAddress(browseModule,(LPSTR)1L);
 browseError=(BROWSEERROR)GetProcAddress(browseModule,(LPSTR)26L);
 if(!browseOpen||!browseError){uiBrowseError=0xffffUL;FreeLibrary(browseModule);browseModule=0;playlistFeedback("Browser unavailable");return 0;}
 uiBrowseStep=3;
 browsePath[0]=0;browseFileTitle[0]=0;browseDirectory[0]=0;
 n=GetDlgItemText(mainWindow,FILEBOX,browseFileTitle,sizeof(browseFileTitle));last=0;
 for(i=0;i<n;++i)if(browseFileTitle[i]=='\\'||browseFileTitle[i]=='/')last=(WORD)(i+1);
 if(last){for(i=0;i<last;++i)browseDirectory[i]=browseFileTitle[i];browseDirectory[last]=0;}
 else lstrcpy(browseDirectory,directory);
 n=lstrlen(browseDirectory);if(n>3&&(browseDirectory[n-1]=='\\'||browseDirectory[n-1]=='/'))browseDirectory[n-1]=0;
 for(i=0;i<sizeof(browseInfo);++i)((char *)&browseInfo)[i]=0;
 browseInfo.lStructSize=(DWORD)sizeof(browseInfo);browseInfo.hwndOwner=mainWindow;
 browseInfo.lpstrFilter=browseFilter;browseInfo.nFilterIndex=1;
 browseInfo.lpstrFile=browsePath;browseInfo.nMaxFile=sizeof(browsePath);
 browseInfo.lpstrFileTitle=browseFileTitle;browseInfo.nMaxFileTitle=sizeof(browseFileTitle);
 browseInfo.lpstrInitialDir=browseDirectory;browseInfo.lpstrTitle="Add song";
 browseInfo.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_HIDEREADONLY;
 uiBrowseStep=4;dlg=browseOpen(&browseInfo);err=dlg?0:browseError();uiBrowseError=err;uiBrowseStep=5;
 FreeLibrary(browseModule);browseModule=0;browseOpen=0;browseError=0;
 if(!dlg){if(err)playlistFeedback("Browser error");return 0;}
 h=OpenFile(browsePath,&browseRead,OF_READ|OF_SHARE_DENY_WRITE);
 if(h==HFILE_ERROR){playlistFeedback("File unavailable");return 0;}
 n=lstrlen((LPSTR)browseRead.szPathName);
 if(!n||n>PLL_PATH_MAX){_lclose(h);playlistFeedback("Path too long");return 0;}
 lstrcpy(browsePath,(LPSTR)browseRead.szPathName);
 _lclose(h);addPath(browsePath);uiBrowseStep=6;return 1;
}
static void removeSelected(void)
{
 WORD i;LTWLISTPAINT batch;
 i=(WORD)SendMessage(listBox,LB_GETCURSEL,0,0L);if(i>=playlist.count){playlistFeedback("Select a song");return;}
 if(PLLRemove(&playlist,i,listRows())!=PLL_OK){playlistFeedback("Remove failed");return;}
 batch.list=0;batch.depth=0;LtwListBegin(&batch,listBox);
 SendMessage(listBox,LB_DELETESTRING,i,0L);
 if(playlist.selected<playlist.count)SendMessage(listBox,LB_SETCURSEL,playlist.selected,0L);
 LtwListEnd(&batch,listBox);selectedPathChanged();playlistFeedback(0);
}
static WORD closeFile(void)
{
 DWORD r;playing=0;
 if(formatKind){if(FmtClose()){caption(FmtReason());return 1;}formatKind=formatPaused=0;SetWindowText(GetDlgItem(mainWindow,PAUSE),"Pause");}
 if(rawKind){if(RawClose()){caption("Retry");return 1;}rawKind=0;}
 if(device){
  command(device,MC_STOP,0L,0L);
  r=command(device,MC_CLOSE,0L,0L);
  if(r){caption("Retry");return 1;}
  device=0;
 }


 return 0;
}
static void play(void)
{
 DWORD r;WORD i,found;if(closeFile())return;
 /* Read the edit value rather than its cached window caption on Windows3.0. */
 filename[0]=0;i=GetDlgItemText(mainWindow,FILEBOX,filename,sizeof(filename));
 if(!i){caption("Enter a file name");return;}
 if(i>=sizeof(filename)-1){caption("File name is too long");return;}
 found=findPath(filename);
 if(found<playlist.count){PLLSelect(&playlist,found,listRows());SendMessage(listBox,LB_SETCURSEL,found,0L);}
 else {playlist.selected=PLL_NONE;SendMessage(listBox,LB_SETCURSEL,0xffff,0L);}
 lstrcpy(activeName,filename);trackName();lastSecond=0xffff;SetWindowText(clockBox,"00:00");
 caption("Opening");UpdateWindow(mainWindow);UpdateWindow(label);UpdateWindow(trackBox);
 i=lstrlen(filename);rawKind=formatKind=0;
 if(i>=4){if(!lstrcmpi(filename+i-4,".VGM")||!lstrcmpi(filename+i-4,".VGZ"))formatKind=3;else if(!lstrcmpi(filename+i-4,".MOD"))formatKind=4;}
 if(formatKind){
  opening=1;openCancelled=closeAfterOpen=0;openTick=GetTickCount()-110UL;
  SetWindowText(GetDlgItem(mainWindow,STOP),"Cancel");
  r=FmtOpen(filename,formatKind);opening=0;
  SetWindowText(GetDlgItem(mainWindow,STOP),"Stop");
  if(r){if(FmtClose()){caption(FmtReason());return;}
   formatKind=0;caption(FmtReason());
   if(closeAfterOpen)PostMessage(mainWindow,WM_CLOSE,0,0L);return;}
  if(closeAfterOpen){if(!closeFile())PostMessage(mainWindow,WM_CLOSE,0,0L);return;}
  playing=1;formatPaused=0;lastService=GetTickCount();SetTimer(mainWindow,1,55,0);
  caption(FmtReason());return;
 }
 if(i>=4){if(!lstrcmpi(filename+i-4,".TND"))rawKind=1;else if(!lstrcmpi(filename+i-4,".SND"))rawKind=2;}
 if(rawKind){r=RawOpen(filename,rawKind);if(r){if(RawClose()){caption("Retry");return;}rawKind=0;wsprintf(text,"Raw E%lu",r);caption(text);return;}
  playing=1;lastService=GetTickCount();SetTimer(mainWindow,1,55,0);caption("Queued");return;}
 SetTimer(mainWindow,1,250,0);
 op.callback=0;op.type=(LPCSTR)523UL;op.element=filename;op.alias=0;
 r=command(0,MC_OPEN,MC_ELEMENT|MC_TYPE|MC_TYPEID,(DWORD)(LPVOID)&op);
 if(r){wsprintf(text,"E%lu",r);caption(text);return;}
 device=op.device;
 if(setInstrumentBank(device,selectedBank)){closeFile();caption("Bank unavailable");return;}
 gp.callback=(DWORD)mainWindow;
 r=command(device,MC_PLAY,MC_NOTIFY,(DWORD)(LPVOID)&gp);
 if(r){if(!closeFile()){wsprintf(text,"E%lu",r);caption(text);}return;}
 playing=1;lastService=GetTickCount();caption("Playing");


}
LONG FAR PASCAL ControlProc(HWND w,WORD m,WORD a,LONG b)
{
 WORD id=GetDlgCtrlID(w),i,n,fire;static RECT r;static PAINTSTRUCT ps;HDC dc;HFONT old;static char buf[144];LONG result;
 if(m==WM_ERASEBKGND)return 1L;
 if(m==WM_SETTEXT){result=DefWindowProc(w,m,a,b);InvalidateRect(w,0,FALSE);return result;}
 if(id==PLAY || id==STOP || id==ADD || id==REMOVE || id==PAUSE || id==BANK_BUTTON){
  if(m==WM_LBUTTONDOWN){++uiDown;SetFocus(w);SetCapture(w);SetWindowWord(w,0,1);InvalidateRect(w,0,FALSE);return 0L;}
  if(m==WM_LBUTTONUP){
   ++uiUp;i=GetWindowWord(w,0);uiPressed=i;GetClientRect(w,&r);
   fire=i&&(int)LOWORD(b)>=0&&(int)LOWORD(b)<r.right&&(int)HIWORD(b)>=0&&(int)HIWORD(b)<r.bottom;
   SetWindowWord(w,0,0);if(GetCapture()==w)ReleaseCapture();
   uiAfterRelease=IsWindow(w)?GetWindowWord(w,0):0;
   if(IsWindow(w)){InvalidateRect(w,0,FALSE);if(fire)SendMessage(mainWindow,WM_COMMAND,id,MAKELONG(w,BN_CLICKED));}return 0L;
  }
  if(m==WM_KEYDOWN && (a==VK_SPACE || a==VK_RETURN) && !(b&0x40000000L)){SendMessage(mainWindow,WM_COMMAND,id,MAKELONG(w,BN_CLICKED));return 0L;}
 }
 if(m==WM_CANCELMODE){++uiCancel;SetWindowWord(w,0,0);if(GetCapture()==w)ReleaseCapture();InvalidateRect(w,0,FALSE);return 0L;}
 if(m==WM_SETFOCUS || m==WM_KILLFOCUS){InvalidateRect(w,0,FALSE);return 0L;}
 if(m==WM_PAINT){
  trace("child paint begin");dc=BeginPaint(w,&ps);GetClientRect(w,&r);dc=beginSurface(dc,&r);old=SelectObject(dc,compactFont);trace("child fill");
  FillRect(dc,&r,(id==LABEL || id==CLOCKBOX || id==108 || id==PLAYLIST_STATUS)?displayBrush:panelBrush);
  trace("child fill done");SetTextColor(dc,ink);SetBkColor(dc,RGB(0,0,0));
  {
   n=GetWindowText(w,buf,sizeof(buf));
   if(id==PLAY || id==STOP || id==ADD || id==REMOVE || id==PAUSE || id==BANK_BUTTON){bevel(dc,&r,GetWindowWord(w,0));SetTextColor(dc,lightColor);SetBkColor(dc,panelColor);
    if(n)TextOut(dc,(r.right-n*charWidth)/2,(r.bottom-charHeight)/2,buf,n);
    if(GetFocus()==w){MoveTo(dc,4,id==BANK_BUTTON?r.bottom-2:r.bottom-4);LineTo(dc,r.right-4,id==BANK_BUTTON?r.bottom-2:r.bottom-4);}
   } else if(n){i=r.right/charWidth;if(n>i)n=i;if(n)TextOut(dc,0,0,buf,n);}
  }
  trace("child end");SelectObject(dc,old);endSurface(&r);EndPaint(w,&ps);return 0L;
 }
 return DefWindowProc(w,m,a,b);
}
LONG FAR PASCAL WndProc(HWND w,WORD message,WORD a,LONG b)
{
 DWORD r;
 if(opening){
  if(message==WM_COMMAND){if(a==STOP)openCancelled=1;return 0L;}
  if(message==WM_CLOSE||(message==WM_ENDSESSION&&a)){
   openCancelled=closeAfterOpen=1;return 0L;}
  if(message==WM_QUERYENDSESSION){openCancelled=1;return 0L;}
  if(message==WM_TIMER)return 0L;
 }
 if(message==WM_USER+50)return FmtDebug(a);
 if(message==WM_USER+49)return RawDebug(a==0?1:a);
 if(message==WM_USER+47)return a?device:playing;
 if(message==WM_USER+52){switch(a){case 0:return uiDown;case 1:return uiUp;case 2:return uiCancel;case 3:return uiPressed;case 4:return uiAfterRelease;case 5:return uiAdd;case 6:return uiBrowseStep;case 7:return (WORD)uiBrowseError;case 8:return (WORD)(uiBrowseError>>16);}return 0L;}
 if(message==WM_USER+51){if(a==0)return playlist.count;if(a==1)return playlist.selected;if(a==2)return (LONG)listBox;return 0L;}
 if(message==WM_USER+45){lastAck=a;return 0L;}
 if(message==WM_USER+46)return lastAck;
 if(message==WM_USER+42)return a==2?lastSecond:(a?clockRedraws:foregroundCalls);
 if(message==WM_CREATE){
  HINSTANCE inst=((LPCREATESTRUCT)b)->hInstance;HWND buttons[5];WORD i;
  mainWindow=w;trace("WM_CREATE");
  fileBox=CreateWindow("EDIT",filename,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,4,58,140,14,w,FILEBOX,inst,0L);
  buttons[0]=CreateWindow("ClassicButton","Play",WS_CHILD|WS_VISIBLE|WS_TABSTOP,4,76,44,22,w,PLAY,inst,0L);
  buttons[1]=CreateWindow("ClassicButton","Stop",WS_CHILD|WS_VISIBLE|WS_TABSTOP,52,76,44,22,w,STOP,inst,0L);
  buttons[2]=CreateWindow("ClassicButton","Pause",WS_CHILD|WS_VISIBLE|WS_TABSTOP,100,76,44,22,w,PAUSE,inst,0L);
  buttons[3]=CreateWindow("ClassicButton","Add...",WS_CHILD|WS_VISIBLE|WS_TABSTOP,100,76,44,22,w,ADD,inst,0L);
  buttons[4]=CreateWindow("ClassicButton","Remove",WS_CHILD|WS_VISIBLE|WS_TABSTOP,100,76,44,22,w,REMOVE,inst,0L);
  trackBox=CreateWindow("ClassicLCD",baseName(filename),WS_CHILD|WS_VISIBLE|SS_LEFT,8,20,132,12,w,108,inst,0L);
  clockBox=CreateWindow("ClassicLCD","00:00",WS_CHILD|WS_VISIBLE|SS_LEFT,8,36,52,12,w,CLOCKBOX,inst,0L);
  label=CreateWindow("ClassicLCD","Ready",WS_CHILD|WS_VISIBLE|SS_LEFT,64,36,76,12,w,LABEL,inst,0L);
  playlistStatus=CreateWindow("ClassicLCD","Playlist 0/32",WS_CHILD|WS_VISIBLE|SS_LEFT,4,104,132,12,w,PLAYLIST_STATUS,inst,0L);
  listBox=CreateWindow("LISTBOX","",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|WS_TABSTOP|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,4,118,140,44,w,PLAYLIST,inst,0L);
  SendMessage(fileBox,WM_SETFONT,compactFont,0L);SendMessage(trackBox,WM_SETFONT,compactFont,0L);
  SendMessage(clockBox,WM_SETFONT,compactFont,0L);SendMessage(label,WM_SETFONT,compactFont,0L);
  bankButton=CreateWindow("ClassicButton","MIDI: Default",WS_CHILD|WS_VISIBLE|WS_TABSTOP,148,104,116,12,w,BANK_BUTTON,inst,0L);
  SendMessage(bankButton,WM_SETFONT,compactFont,0L);
  SendMessage(playlistStatus,WM_SETFONT,compactFont,0L);SendMessage(listBox,WM_SETFONT,compactFont,0L);
  for(i=0;i<5;++i)SendMessage(buttons[i],WM_SETFONT,compactFont,0L);
  PLLInit(&playlist);if(filename[0])PLLAdd(&playlist,filename);
  lstrcpy(text,directory);lstrcat(text,"PLUCK.TND");PLLAdd(&playlist,text);
  lstrcpy(text,directory);lstrcat(text,"CHIME.SND");PLLAdd(&playlist,text);
  rebuildList();selectedPathChanged();
  trace("children created");layout(w);trace("layout done");SetTimer(w,1,250,0);return 0L;
 }
 if(message==WM_GETMINMAXINFO){
  MINMAXINFO FAR *mm=(MINMAXINFO FAR *)b;mm->ptMinTrackSize.x=156;mm->ptMinTrackSize.y=192;return 0L;
 }
 if(message==WM_SIZE){if(a!=SIZE_MINIMIZED && fileBox)layout(w);return 0L;}
 if(message==WM_ERASEBKGND)return 1L;
 if(message==WM_CTLCOLOR){
  HDC dc=(HDC)a;SetTextColor(dc,ink);SetBkColor(dc,RGB(0,0,0));return (LONG)displayBrush;
 }
 if(message==WM_PAINT){
  static PAINTSTRUCT ps;HDC dc;static RECT r;HFONT old;trace("parent paint begin");dc=BeginPaint(w,&ps);GetClientRect(w,&r);dc=beginSurface(dc,&r);trace("parent fill");
  FillRect(dc,&r,panelBrush);trace("parent fill done");GetClientRect(w,&r);bevel(dc,&r,0);trace("parent bevel done");FillRect(dc,&displayRect,displayBrush);trace("LCD fill done");bevel(dc,&displayRect,1);trace("LCD bevel done");
  SetTextColor(dc,lightColor);SetBkColor(dc,panelColor);old=SelectObject(dc,compactFont);
  trace("parent text");TextOut(dc,6,4,"TANDY WINAMP",12);if(r.right>=240)TextOut(dc,116,4,"FauxBlaster",11);trace("title done");SelectObject(dc,old);endSurface(&r);EndPaint(w,&ps);return 0L;
 }
 if(message==WM_COMMAND){
  if(a==PLAY)play();
  else if(a==STOP){if(!closeFile())caption("Stopped");}
  else if(a==PAUSE){if(formatKind&&playing){if(!FmtPause()){formatPaused=!formatPaused;SetWindowText(GetDlgItem(w,PAUSE),formatPaused?"Resume":"Pause");caption(formatPaused?"Paused":FmtReason());}else caption("Pause unavailable");}}
  else if(a==ADD){++uiAdd;openBrowser();}
  else if(a==REMOVE)removeSelected();
  else if(a==BANK_BUTTON)chooseBank();
  else if(a==PLAYLIST && HIWORD(b)==LBN_SELCHANGE){
   WORD n=(WORD)SendMessage(listBox,LB_GETCURSEL,0,0L);if(n<playlist.count&&PLLSelect(&playlist,n,listRows())==PLL_OK)selectedPathChanged();
  }return 0L;
 }
 if(message==MC_NOTIFYMSG){
  if(device && (WORD)b==device){
   if(a==MC_SUCCESS){++successes;if(!closeFile())caption("Done");}
   else if(a==MC_ABORTED){++aborts;if(!closeFile())caption("Stopped");}
   else {if(!closeFile())caption("Failed");}
  }return 0L;
 }
 if(message==WM_TIMER){
  serviceForeground();
  if(playing && formatKind){r=FmtPosition();if((WORD)(r/1000UL)!=lastSecond){lastSecond=(WORD)(r/1000UL);wsprintf(text,"%02u:%02u",lastSecond/60,lastSecond%60);SetWindowText(clockBox,text);}}
  if(playing && rawKind){r=RawPosition();if((WORD)(r/1000UL)!=lastSecond){lastSecond=(WORD)(r/1000UL);wsprintf(text,"%02u:%02u",lastSecond/60,lastSecond%60);SetWindowText(clockBox,text);}}
  if(playing && device){st.item=MC_POSITION;r=command(device,MC_STATUS,MC_ITEM,(DWORD)(LPVOID)&st);if(!r && (WORD)(st.result/1000UL)!=lastSecond){lastSecond=(WORD)(st.result/1000UL);wsprintf(text,"%02u:%02u",lastSecond/60,lastSecond%60);SetWindowText(clockBox,text);++clockRedraws;}}
  if(automatic){
   if(!phase){check("visible_window",IsWindowVisible(w)!=0);play();check("GUI_play_started",playing);phase=1;phaseAt=GetTickCount();}
   else if(phase==1 && GetTickCount()-phaseAt>=5500UL){SendMessage(w,WM_COMMAND,STOP,0L);check("GUI_stop_closed_file",!device&&!playing);check("GUI_stop_sound_free",OpenSound()==3);CloseSound();phase=2;phaseAt=GetTickCount();}
   else if(phase==2 && GetTickCount()-phaseAt>=250UL){SetWindowText(fileBox,"C:\\MMLAB\\TYPE0.MID");play();check("GUI_replay_started",playing);phase=3;phaseAt=GetTickCount();}
   else if(phase==3 && successes){check("GUI_EOF_once",successes==1);check("GUI_EOF_closed_file",!device&&!playing);check("GUI_EOF_sound_free",OpenSound()==3);CloseSound();phase=4;phaseAt=GetTickCount();}
   else if(phase==4 && GetTickCount()-phaseAt>=3500UL){check("no_stale_callback",successes==1 && aborts==0);SendMessage(w,WM_CLOSE,0,0L);}
   else if(phase==3 && GetTickCount()-phaseAt>4000UL){check("GUI_EOF_timeout",0);SendMessage(w,WM_CLOSE,0,0L);}
  }return 0L;
 }
 if(message==WM_QUERYENDSESSION){if(closeFile()){PostMessage(w,WM_COMMAND,STOP,0L);return 0L;}return 1L;}
 if(message==WM_ENDSESSION && a){closeFile();return 0L;}
 if(message==WM_CLOSE){if(closeFile())return 0L;KillTimer(w,1);DestroyWindow(w);return 0L;}
 if(message==WM_DESTROY){PostQuitMessage(0);return 0L;}
 return DefWindowProc(w,message,a,b);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
 WNDCLASS wc;MSG message;WORD n,i,last=0;char dll[144];HDC dc;TEXTMETRIC tm;HFONT old;int width,height;
 if(prev){HWND existing=FindWindow("MiniMCIPlayer",0);if(existing){ShowWindow(existing,SW_SHOWNORMAL);SetActiveWindow(existing);}return 0;}
 while(*cmd==' ')++cmd;automatic=lstrcmpi(cmd,"/TEST")==0;
 if(automatic)log=_lcreat("C:\\MINIPLAY.LOG",0);
 check("entered_WinMain",1);
 wsprintf(text,"CMD %s\r\n",cmd);logText();
 n=GetModuleFileName(inst,directory,sizeof(directory));if(!n || n>=sizeof(directory)-13)return 1;for(i=0;i<n;++i)if(directory[i]=='\\' || directory[i]=='/')last=i+1;directory[last]=0;RawInit(directory);FmtInit(directory);FmtSetProgress(formatProgress);
 lstrcpy(bankIni,directory);lstrcat(bankIni,"WINAMP.INI");
 selectedBank=GetPrivateProfileInt("FauxBlaster","Bank",0,bankIni);
 if(selectedBank>1)selectedBank=0;
 lstrcpy(dll,directory);lstrcat(dll,"MCITNDY.DLL");wsprintf(text,"DLL %s\r\n",(LPSTR)dll);logText();module=LoadLibrary(dll);
 wsprintf(text,"LOAD_RESULT %u\r\n",module);logText();
 if(module<32){MessageBox(0,"Keep MCITNDY.DLL with this player.","Tandy Winamp",MB_OK);return 1;}
 foregroundPump=(PUMP)GetProcAddress(module,(LPSTR)900L);
 command=(CMD)GetProcAddress(module,(LPSTR)701L);debug=(ONE)GetProcAddress(module,(LPSTR)904L);
 setInstrumentBank=(BANKSET)GetProcAddress(module,(LPSTR)905L);
 check("command_export",command!=0);if(!command||!setInstrumentBank){
  MessageBox(0,"Run Winamp Setup Repair to restore its sound files.","Tandy Winamp",MB_OK);FreeLibrary(module);return 1;}
 lstrcpy(filename,directory);lstrcat(filename,"STREAM.MID");
 while(*cmd==' ')++cmd;automatic=lstrcmpi(cmd,"/TEST")==0;
 if(automatic)check("automatic_mode",1);
 else if(*cmd){if(lstrlen(cmd)>=sizeof(filename)){FreeLibrary(module);return 1;}lstrcpy(filename,cmd);}
 dc=GetDC(0);compactFont=GetStockObject(SYSTEM_FONT);old=SelectObject(dc,compactFont);GetTextMetrics(dc,&tm);
 charWidth=tm.tmAveCharWidth>0?tm.tmAveCharWidth:8;charHeight=tm.tmHeight>0?tm.tmHeight:8;
 panelColor=GetNearestColor(dc,RGB(64,64,64));lightColor=GetNearestColor(dc,RGB(192,192,192));
 ink=GetDeviceCaps(dc,NUMCOLORS)>2?GetNearestColor(dc,RGB(0,255,0)):RGB(255,255,255);
 SelectObject(dc,old);ReleaseDC(0,dc);
 panelBrush=CreateSolidBrush(panelColor);displayBrush=CreateSolidBrush(RGB(0,0,0));selectBrush=CreateSolidBrush(ink);
 lightPen=CreatePen(PS_SOLID,1,lightColor);darkPen=CreatePen(PS_SOLID,1,RGB(0,0,0));
 wc.style=0;wc.lpfnWndProc=WndProc;wc.cbClsExtra=wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=0;wc.hCursor=LoadCursor(0,IDC_ARROW);wc.hbrBackground=panelBrush;wc.lpszMenuName=0;wc.lpfnWndProc=ControlProc;wc.cbWndExtra=2;wc.hbrBackground=0;
 wc.lpszClassName="ClassicLCD";RegisterClass(&wc);wc.lpszClassName="ClassicButton";RegisterClass(&wc);
 wc.lpfnWndProc=WndProc;wc.cbWndExtra=0;wc.hbrBackground=panelBrush;wc.lpszClassName="MiniMCIPlayer";RegisterClass(&wc);
 trace("skin objects/classes ready");width=GetSystemMetrics(SM_CXSCREEN)-4;if(width>316)width=316;height=GetSystemMetrics(SM_CYSCREEN)-4;if(height>192)height=192;
 mainWindow=CreateWindow("MiniMCIPlayer","Tandy Winamp",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN,2,2,width,height,0,0,inst,0L);
 check("CreateWindow_result",mainWindow!=0);if(!mainWindow){FreeLibrary(module);freeSkin();return 1;}trace("window created");ShowWindow(mainWindow,show);trace("window shown");UpdateWindow(mainWindow);trace("window updated");
 while(GetMessage(&message,0,0,0)){
  if(message.message==WM_KEYDOWN && message.wParam==VK_TAB && (message.hwnd==mainWindow || IsChild(mainWindow,message.hwnd)))
   SetFocus(GetNextDlgTabItem(mainWindow,GetFocus(),GetKeyState(VK_SHIFT)<0));
  else {TranslateMessage(&message);DispatchMessage(&message);}
  serviceForeground();
 }
 FreeLibrary(module);freeSkin();
 if(log!=HFILE_ERROR){wsprintf(text,"FINAL failures=%u checks=%u\r\n",failures,checks);_lwrite(log,text,lstrlen(text));_lclose(log);ExitWindows(0L,0);}
 return failures;
}
