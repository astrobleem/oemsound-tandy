/* Original small Windows3.0 MIDI UI using the established app-local MCI API.
   This fallback is separate from original Media Player compatibility proof. */
#define WINVER 0x0300
#include <windows.h>
#include "LWAUDIO.H"
static WORD rawKind;
static void caption(char *);
static WORD closeFile(void);
#define PLAY 101
#define STOP 102
#define FILEBOX 103
#define LABEL 104
#define CLOCKBOX 105
#define PLAYLIST 106
#define ADD 107
typedef DWORD (FAR PASCAL *CMD)(WORD,WORD,DWORD,DWORD);
typedef WORD (FAR PASCAL *ONE)(WORD);
#ifdef PAINT_TRACE
static void trace(char *s){HFILE f=_lopen("C:\\PAINT.LOG",OF_WRITE);if(f==HFILE_ERROR)f=_lcreat("C:\\PAINT.LOG",0);_llseek(f,0L,2);_lwrite(f,s,lstrlen(s));_lwrite(f,"\r\n",2);_lclose(f);}
#else
#define trace(s)
#endif
static CMD command;static ONE debug;
typedef WORD (FAR PASCAL *PUMP)(void);static PUMP foregroundPump;
static DWORD lastService;static WORD foregroundCalls,clockRedraws,lastAck;
static HMODULE module;static HWND mainWindow,fileBox,label,clockBox,listBox,trackBox;
static HBRUSH panelBrush,displayBrush,selectBrush;static HPEN lightPen,darkPen;
static HFONT compactFont;static COLORREF ink,panelColor,lightColor;static WORD charWidth=8,charHeight=8;
static WORD playlistCount,lastSecond=0xffff,compactMode;
static char playlist[4][144],activeName[144],stateText[144]="Ready";
static RECT displayRect;
static WORD device,playing,automatic,phase,checks,failures,successes,aborts;
static DWORD phaseAt;static HFILE log=HFILE_ERROR;
static char directory[144],filename[144],text[180];
static MCOPEN op;static MCGEN gp;static MCSTATUS st;
static void serviceForeground(void)
{
 DWORD now=GetTickCount();
 if(playing && now-lastService>=55UL){
  lastService=now;
  if(rawKind){WORD r=RawPump();if(r!=1){++successes;if(!closeFile())caption(r==0?"Done":"Clock limit");}}
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
static char *baseName(char *name)
{
 WORD i,last=0;for(i=0;name[i];++i)if(name[i]=='\\' || name[i]=='/')last=i+1;
 return name+last;
}
static void caption(char *s)
{
 if(lstrcmp(stateText,s)){lstrcpy(stateText,s);SetWindowText(label,s);}
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
 static RECT r;int width,height,bw,listY=112;GetClientRect(w,&r);width=r.right;height=r.bottom;
 compactMode=width<220;
 displayRect.left=4;displayRect.top=16;displayRect.right=width-4;displayRect.bottom=54;
 MoveWindow(trackBox,8,20,width-16,12,TRUE);
 MoveWindow(clockBox,8,36,52,12,TRUE);MoveWindow(label,64,36,width-72,12,TRUE);
 MoveWindow(fileBox,4,58,width-8,14,TRUE);
 bw=(width-16)/3;
 MoveWindow(GetDlgItem(w,PLAY),4,76,bw,22,TRUE);
 MoveWindow(GetDlgItem(w,STOP),8+bw,76,bw,22,TRUE);
 MoveWindow(GetDlgItem(w,ADD),12+2*bw,76,width-16-2*bw,22,TRUE);
 MoveWindow(listBox,4,listY,width-8,height-listY-4,TRUE);
 trackName();InvalidateRect(w,0,FALSE);
}
static void addFile(void)
{
 WORD i;static char b[144];GetWindowText(fileBox,b,sizeof(b));if(!b[0])return;
 for(i=0;i<playlistCount;++i)if(!lstrcmpi(b,playlist[i])){
  SendMessage(listBox,LB_SETCURSEL,i,0L);return;
 }
 if(playlistCount==4){caption("Full");return;}
 lstrcpy(playlist[playlistCount],b);
 SendMessage(listBox,LB_ADDSTRING,0,(LONG)(LPSTR)baseName(playlist[playlistCount]));
 SendMessage(listBox,LB_SETCURSEL,playlistCount,0L);++playlistCount;
 if(!playing){lstrcpy(activeName,b);trackName();}
}
static WORD closeFile(void)
{
 DWORD r;playing=0;
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
 DWORD r;WORD i,found=0xffff;if(closeFile())return;GetWindowText(fileBox,filename,sizeof(filename));
 for(i=0;i<playlistCount;++i)if(!lstrcmpi(filename,playlist[i]))found=i;
 SendMessage(listBox,LB_SETCURSEL,found,0L);
 lstrcpy(activeName,filename);trackName();lastSecond=0xffff;SetWindowText(clockBox,"00:00");
 caption("Opening");UpdateWindow(mainWindow);UpdateWindow(label);UpdateWindow(trackBox);
 i=lstrlen(filename);rawKind=0;
 if(i>=4){if(!lstrcmpi(filename+i-4,".TND"))rawKind=1;else if(!lstrcmpi(filename+i-4,".SND"))rawKind=2;}
 if(rawKind){r=RawOpen(filename,rawKind);if(r){if(RawClose()){caption("Retry");return;}rawKind=0;wsprintf(text,"Raw E%lu",r);caption(text);return;}
  playing=1;lastService=GetTickCount();SetTimer(mainWindow,1,55,0);caption("Queued");return;}
 SetTimer(mainWindow,1,250,0);
 op.callback=0;op.type=(LPCSTR)523UL;op.element=filename;op.alias=0;
 r=command(0,MC_OPEN,MC_ELEMENT|MC_TYPE|MC_TYPEID,(DWORD)(LPVOID)&op);
 if(r){wsprintf(text,"E%lu",r);caption(text);return;}
 device=op.device;gp.callback=(DWORD)mainWindow;
 r=command(device,MC_PLAY,MC_NOTIFY,(DWORD)(LPVOID)&gp);
 if(r){if(!closeFile()){wsprintf(text,"E%lu",r);caption(text);}return;}
 playing=1;lastService=GetTickCount();caption("Playing");


}
static char listNames[4][20];static WORD listCount,listSelected=0xffff,listFirst;
static void listKeepVisible(HWND w)
{
 static RECT r;WORD rows;GetClientRect(w,&r);rows=(r.bottom-4)/charHeight;if(!rows)rows=1;
 if(listSelected<listCount){if(listSelected<listFirst)listFirst=listSelected;else if(listSelected>=listFirst+rows)listFirst=listSelected-rows+1;}
}
LONG FAR PASCAL ControlProc(HWND w,WORD m,WORD a,LONG b)
{
 WORD id=GetDlgCtrlID(w),i,n,row,index;static RECT r,line;static PAINTSTRUCT ps;HDC dc;HFONT old;static char buf[144];LONG result;
 if(m==WM_ERASEBKGND)return 1L;
 if(m==WM_SETTEXT){result=DefWindowProc(w,m,a,b);InvalidateRect(w,0,FALSE);return result;}
 if(id==PLAYLIST){
  if(m==LB_ADDSTRING){if(listCount==4)return LB_ERRSPACE;for(i=0;i<19 && ((LPSTR)b)[i];++i)listNames[listCount][i]=((LPSTR)b)[i];listNames[listCount][i]=0;InvalidateRect(w,0,FALSE);return listCount++;}
  if(m==LB_GETCOUNT)return listCount;
  if(m==LB_GETCURSEL)return listSelected==0xffff?LB_ERR:listSelected;
  if(m==LB_SETCURSEL){listSelected=a<listCount?a:0xffff;listKeepVisible(w);InvalidateRect(w,0,FALSE);return listSelected==0xffff?LB_ERR:listSelected;}
  if(m==WM_SIZE){listKeepVisible(w);InvalidateRect(w,0,FALSE);return 0L;}
  if(m==WM_LBUTTONDOWN){SetFocus(w);row=((int)HIWORD(b)-2)/charHeight;index=listFirst+row;if(index<listCount){listSelected=index;InvalidateRect(w,0,FALSE);PostMessage(mainWindow,WM_COMMAND,PLAYLIST,MAKELONG(w,LBN_SELCHANGE));}return 0L;}
  if(m==WM_KEYDOWN && (a==VK_UP || a==VK_DOWN || a==VK_HOME || a==VK_END)){
   if(!listCount)return 0L;if(listSelected>=listCount)listSelected=0;
   else if(a==VK_UP && listSelected) --listSelected;else if(a==VK_DOWN && listSelected+1<listCount)++listSelected;else if(a==VK_HOME)listSelected=0;else if(a==VK_END)listSelected=listCount-1;
   listKeepVisible(w);InvalidateRect(w,0,FALSE);PostMessage(mainWindow,WM_COMMAND,PLAYLIST,MAKELONG(w,LBN_SELCHANGE));return 0L;
  }
 } else if(id==PLAY || id==STOP || id==ADD){
  if(m==WM_LBUTTONDOWN){SetFocus(w);SetCapture(w);SetWindowWord(w,0,1);InvalidateRect(w,0,FALSE);return 0L;}
  if(m==WM_LBUTTONUP){ReleaseCapture();GetClientRect(w,&r);i=GetWindowWord(w,0);SetWindowWord(w,0,0);InvalidateRect(w,0,FALSE);
   if(i && (int)LOWORD(b)>=0 && (int)LOWORD(b)<r.right && (int)HIWORD(b)>=0 && (int)HIWORD(b)<r.bottom)SendMessage(mainWindow,WM_COMMAND,id,MAKELONG(w,BN_CLICKED));return 0L;}
  if(m==WM_KEYDOWN && (a==VK_SPACE || a==VK_RETURN) && !(b&0x40000000L)){SendMessage(mainWindow,WM_COMMAND,id,MAKELONG(w,BN_CLICKED));return 0L;}
 }
 if(m==WM_CANCELMODE){SetWindowWord(w,0,0);ReleaseCapture();InvalidateRect(w,0,FALSE);return 0L;}
 if(m==WM_SETFOCUS || m==WM_KILLFOCUS){InvalidateRect(w,0,FALSE);return 0L;}
 if(m==WM_PAINT){
  trace("child paint begin");dc=BeginPaint(w,&ps);GetClientRect(w,&r);dc=beginSurface(dc,&r);old=SelectObject(dc,compactFont);trace("child fill");
  FillRect(dc,&r,(id==PLAYLIST || id==104 || id==105 || id==108)?displayBrush:panelBrush);
  trace("child fill done");SetTextColor(dc,ink);SetBkColor(dc,RGB(0,0,0));
  if(id==PLAYLIST){
   bevel(dc,&r,1);row=0;
   while(listFirst+row<listCount && 2+(row+1)*charHeight<=r.bottom-2){
    index=listFirst+row;line.left=2;line.top=2+row*charHeight;line.right=r.right-2;line.bottom=line.top+charHeight;
    if(index==listSelected){FillRect(dc,&line,selectBrush);SetTextColor(dc,RGB(0,0,0));SetBkColor(dc,ink);}else{SetTextColor(dc,ink);SetBkColor(dc,RGB(0,0,0));}
    n=lstrlen(listNames[index]);i=(r.right-8)/charWidth;if(n>i)n=i;if(n)TextOut(dc,4,line.top,listNames[index],n);++row;
   }
  } else {
   n=GetWindowText(w,buf,sizeof(buf));
   if(id==PLAY || id==STOP || id==ADD){bevel(dc,&r,GetWindowWord(w,0));SetTextColor(dc,lightColor);SetBkColor(dc,panelColor);
    if(n)TextOut(dc,(r.right-n*charWidth)/2,(r.bottom-charHeight)/2,buf,n);
    if(GetFocus()==w){MoveTo(dc,4,r.bottom-4);LineTo(dc,r.right-4,r.bottom-4);}
   } else if(n){i=r.right/charWidth;if(n>i)n=i;if(n)TextOut(dc,0,0,buf,n);}
  }
  trace("child end");SelectObject(dc,old);endSurface(&r);EndPaint(w,&ps);return 0L;
 }
 return DefWindowProc(w,m,a,b);
}
LONG FAR PASCAL WndProc(HWND w,WORD message,WORD a,LONG b)
{
 DWORD r;
 if(message==WM_USER+49)return RawDebug(a==0?1:a);
 if(message==WM_USER+47)return a?device:playing;
 if(message==WM_USER+45){lastAck=a;return 0L;}
 if(message==WM_USER+46)return lastAck;
 if(message==WM_USER+42)return a==2?lastSecond:(a?clockRedraws:foregroundCalls);
 if(message==WM_CREATE){
  HINSTANCE inst=((LPCREATESTRUCT)b)->hInstance;HWND buttons[3];WORD i;
  mainWindow=w;trace("WM_CREATE");
  fileBox=CreateWindow("EDIT",filename,WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP,4,58,140,14,w,FILEBOX,inst,0L);
  buttons[0]=CreateWindow("ClassicButton","Play",WS_CHILD|WS_VISIBLE|WS_TABSTOP,4,76,44,22,w,PLAY,inst,0L);
  buttons[1]=CreateWindow("ClassicButton","Stop",WS_CHILD|WS_VISIBLE|WS_TABSTOP,52,76,44,22,w,STOP,inst,0L);
  buttons[2]=CreateWindow("ClassicButton","Add",WS_CHILD|WS_VISIBLE|WS_TABSTOP,100,76,44,22,w,ADD,inst,0L);
  trackBox=CreateWindow("ClassicLCD",baseName(filename),WS_CHILD|WS_VISIBLE|SS_LEFT,8,20,132,12,w,108,inst,0L);
  clockBox=CreateWindow("ClassicLCD","00:00",WS_CHILD|WS_VISIBLE|SS_LEFT,8,36,52,12,w,CLOCKBOX,inst,0L);
  label=CreateWindow("ClassicLCD","Ready",WS_CHILD|WS_VISIBLE|SS_LEFT,64,36,76,12,w,LABEL,inst,0L);
  listBox=CreateWindow("ClassicList","",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,4,112,140,44,w,PLAYLIST,inst,0L);
  SendMessage(fileBox,WM_SETFONT,compactFont,0L);SendMessage(trackBox,WM_SETFONT,compactFont,0L);
  SendMessage(clockBox,WM_SETFONT,compactFont,0L);SendMessage(label,WM_SETFONT,compactFont,0L);SendMessage(listBox,WM_SETFONT,compactFont,0L);
  for(i=0;i<3;++i)SendMessage(buttons[i],WM_SETFONT,compactFont,0L);
  lstrcpy(activeName,filename);addFile();
  lstrcpy(playlist[1],directory);lstrcat(playlist[1],"PLUCK.TND");SendMessage(listBox,LB_ADDSTRING,0,(LONG)(LPSTR)"PLUCK.TND");
  lstrcpy(playlist[2],directory);lstrcat(playlist[2],"CHIME.SND");SendMessage(listBox,LB_ADDSTRING,0,(LONG)(LPSTR)"CHIME.SND");playlistCount=3;
  trace("children created");layout(w);trace("layout done");SetTimer(w,1,250,0);return 0L;
 }
 if(message==WM_GETMINMAXINFO){
  MINMAXINFO FAR *mm=(MINMAXINFO FAR *)b;mm->ptMinTrackSize.x=140;mm->ptMinTrackSize.y=168;return 0L;
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
  trace("parent text");TextOut(dc,6,4,"TANDY QUEUED",12);trace("title done");TextOut(dc,6,102,"PLAYLIST",8);trace("playlist title done");SelectObject(dc,old);endSurface(&r);EndPaint(w,&ps);return 0L;
 }
 if(message==WM_COMMAND){
  if(a==PLAY)play();
  else if(a==STOP){if(!closeFile())caption("Stopped");}
  else if(a==ADD)addFile();
  else if(a==PLAYLIST && HIWORD(b)==LBN_SELCHANGE){
   WORD n=(WORD)SendMessage(listBox,LB_GETCURSEL,0,0L);if(n<playlistCount){SetWindowText(fileBox,playlist[n]);if(!playing){lstrcpy(activeName,playlist[n]);trackName();}}
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
 n=GetModuleFileName(inst,directory,sizeof(directory));if(!n || n>=sizeof(directory)-13)return 1;for(i=0;i<n;++i)if(directory[i]=='\\' || directory[i]=='/')last=i+1;directory[last]=0;RawInit(directory);
 lstrcpy(dll,directory);lstrcat(dll,"MCITNDY.DLL");wsprintf(text,"DLL %s\r\n",(LPSTR)dll);logText();module=LoadLibrary(dll);
 wsprintf(text,"LOAD_RESULT %u\r\n",module);logText();
 if(module<32){MessageBox(0,"Keep MCITNDY.DLL with this player.","MIDI Player",MB_OK);return 1;}
 foregroundPump=(PUMP)GetProcAddress(module,(LPSTR)900L);
 command=(CMD)GetProcAddress(module,(LPSTR)701L);debug=(ONE)GetProcAddress(module,(LPSTR)904L);
 check("command_export",command!=0);if(!command){FreeLibrary(module);return 1;}
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
 wc.lpszClassName="ClassicLCD";RegisterClass(&wc);wc.lpszClassName="ClassicButton";RegisterClass(&wc);wc.lpszClassName="ClassicList";RegisterClass(&wc);
 wc.lpfnWndProc=WndProc;wc.cbWndExtra=0;wc.hbrBackground=panelBrush;wc.lpszClassName="MiniMCIPlayer";RegisterClass(&wc);
 trace("skin objects/classes ready");width=GetSystemMetrics(SM_CXSCREEN)-4;if(width>292)width=292;height=GetSystemMetrics(SM_CYSCREEN)-4;if(height>192)height=192;
 mainWindow=CreateWindow("MiniMCIPlayer","Tandy Queued",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_CLIPCHILDREN,2,2,width,height,0,0,inst,0L);
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
