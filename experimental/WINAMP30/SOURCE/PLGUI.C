/* Asynchronous native playlist qualification.  The player and LWFILE.DLL are
   production inputs; this driver never calls playlist internals. */
#define WINVER 0x0300
#include <windows.h>

#define ID_PLAY 101
#define ID_STOP 102
#define ID_FILE 103
#define ID_STATE 104
#define ID_CLOCK 105
#define ID_LIST 106
#define ID_ADD 107
#define ID_TRACK 108
#define ID_REMOVE 110
#define ID_STATUS 111
#define BROWSE_FILE 100
#define BROWSE_LIST 101
#define BROWSE_FILTER 102
#define BROWSE_DIR 103
#define ID_OPEN 1
#define ID_CANCEL 2

#ifndef PLGUI_CASE
#define PLGUI_CASE 1
#endif

#define P_START 0
#define P_WAIT_PLAYER 1
#define P_GEST_OUT 2
#define P_GEST_OUT_CHECK 3
#define P_GEST_CANCEL 4
#define P_GEST_CANCEL_CHECK 5
#define P_GEST_DUP 6
#define P_GEST_DUP_CHECK 7
#define P_WAIT_ADD 10
#define P_BROWSER_CANCEL 11
#define P_WAIT_CANCEL 12
#define P_ADD_SELECT 13
#define P_ADD_OPEN 14
#define P_WAIT_ADD_DONE 15
#define P_MISSING_TYPE 16
#define P_MISSING_OPEN 17
#define P_WAIT_NOTICE 18
#define P_NOTICE_DISMISS 19
#define P_WAIT_NOTICE_GONE 20
#define P_MISSING_PLAY 22
#define P_MISSING_PLAY_CHECK 23
#define P_REMOVE_NONE 24
#define P_REMOVE_NONE_CHECK 25
#define P_C1_NEXT_COPPER 26
#define P_C1_NEXT_DUP 27
#define P_C1_NEXT_MISSING 28
#define P_MISSING_PLAY_VERIFY 68
#define P_C2_NEXT_ADD 30
#define P_C2_KEYS_END 31
#define P_C2_KEYS_END_CHECK 32
#define P_C2_KEYS_HOME 33
#define P_C2_KEYS_HOME_CHECK 34
#define P_C2_KEYS_NEXT 35
#define P_C2_KEYS_NEXT_CHECK 36
#define P_C2_KEYS_PRIOR 37
#define P_C2_KEYS_PRIOR_CHECK 38
#define P_C2_NARROW 39
#define P_C2_NARROW_END 40
#define P_C2_NARROW_CHECK 41
#define P_C2_RESTORE 42
#define P_C3_HOME_FIRST 48
#define P_C3_HOME_FIRST_CHECK 49
#define P_C3_ROW 50
#define P_C3_ROW_CHECK 51
#define P_C3_PLAY 52
#define P_C3_PLAY_CHECK 53
#define P_C3_START_BROWSER 54
#define P_C3_HOLD_BROWSER 55
#define P_C3_WAIT_ACTIVE_CANCEL 56
#define P_C3_ROW_ACTIVE 57
#define P_C3_ROW_ACTIVE_CHECK 58
#define P_C3_REMOVE_ACTIVE 59
#define P_C3_REMOVE_ACTIVE_CHECK 60
#define P_C3_HOME 61
#define P_C3_HOME_CHECK 62
#define P_C3_REMOVE_ORDINARY 63
#define P_C3_REMOVE_ORDINARY_CHECK 64
#define P_C3_STOP 65
#define P_C3_STOP_CHECK 66
#define P_C3_NEXT_ADD 67
#define P_CLOSE_PLAYER 90
#define P_FAIL_CLOSE 91

#define BROWSE_CANCEL 1
#define BROWSE_ADD 2
#define BROWSE_MISSING 3
#define BROWSE_ACTIVE 4

static HWND rootWindow,player,listBox,browser,notice,lastPopup;
static FARPROC timerThunk;
static WORD timerId,phase,busy,done,checks,failures,queueOK;
static WORD browserPurpose,cancelContinuation,addContinuation,addExpectedCount,addExpectedSelection;
static WORD c2AddIndex,addCountBefore,gestureAddBefore,gestureUpBefore,modalSampled;
static WORD lastPopupVisible,lastPopupOwner;
static DWORD phaseAt,suiteAt,failureAt,freeCold,freeOpen,freeWarm,freeAfter,bytesBefore,foregroundBefore;
static int addFileIndex;
static char line[384],title[80],className[32],value[160],expected[160];
static char missingPath[]="C:\\MMLAB\\MISSING.VGM";
static char addLeaf[32];
static char clockBefore[32],clockAfter[32],popupTitle[64];
static RECT rowRect;
static OFSTRUCT fileInfo;
static MSG message;

static void record(char *s)
{
 HFILE f=_lopen("C:\\PLGUI.LOG",OF_WRITE);
 if(f!=HFILE_ERROR){_llseek(f,0L,2);_lwrite(f,s,lstrlen(s));_lwrite(f,"\r\n",2);_lclose(f);}
}
static void check(char *name,DWORD got,DWORD want)
{
 ++checks;if(got!=want)++failures;
 wsprintf(line,"CHECK %s got=%lu want=%lu",(LPSTR)name,got,want);record(line);
}
static void phaseSet(WORD next)
{
 phase=next;phaseAt=GetTickCount();wsprintf(line,"PHASE %u case=%u task=%u",phase,PLGUI_CASE,(WORD)GetCurrentTask());record(line);
}
static DWORD query(WORD q){return (DWORD)SendMessage(player,WM_USER+51,q,0L);}
static WORD debug(WORD q){return (WORD)SendMessage(player,WM_USER+50,q,0L);}
static WORD hasPrefix(char *s,char *p);
static void caption(char *want)
{
 GetWindowText(player,value,sizeof(value));wsprintf(line,"CAPTION actual=[%s] expected=[%s]",(LPSTR)value,(LPSTR)want);record(line);
 check("main_caption_Tandy_Winamp",!lstrcmp(value,want),1);
}
static void listCount(WORD want,char *tag)
{
 WORD actual=(WORD)SendMessage(listBox,LB_GETCOUNT,0,0L),model=(WORD)query(0);
 wsprintf(line,"COUNT %s native=%u model=%u expected=%u",(LPSTR)tag,actual,model,want);record(line);
 check("native_model_count_agree",actual,model);check(tag,actual,want);
 wsprintf(expected,"Playlist %u/32",want);GetWindowText(GetDlgItem(player,ID_STATUS),value,sizeof(value));
 check("playlist_header_count",hasPrefix(value,expected),1);
}
static WORD selectedIndex(void){return (WORD)SendMessage(listBox,LB_GETCURSEL,0,0L);}
static void selected(WORD want,char *tag)
{
 WORD native=selectedIndex(),model=(WORD)query(1);wsprintf(line,"SELECT %s native=%u model=%u",(LPSTR)tag,native,model);record(line);
 check("selection_native_model_agree",native,model);check(tag,native,want);
}
static void rowText(WORD row,char *want,char *tag)
{
 value[0]=0;check("native_row_read",SendMessage(listBox,LB_GETTEXT,row,(LPARAM)(LPSTR)value)!=LB_ERR,1);
 check(tag,!lstrcmpi(value,want),1);
}
static WORD containsText(char *s,char *part)
{
 WORD i,j,n=lstrlen(s),m=lstrlen(part);if(m>n)return 0;
 for(i=0;i<=n-m;++i){for(j=0;j<m;++j)if((s[i+j]>='a'&&s[i+j]<='z'?s[i+j]-32:s[i+j])!=(part[j]>='a'&&part[j]<='z'?part[j]-32:part[j]))break;if(j==m)return 1;}
 return 0;
}
static WORD hasPrefix(char *s,char *part)
{
 WORD i;for(i=0;part[i];++i)if(s[i]!=part[i])return 0;return 1;
}
static WORD postButton(HWND window,WORD id,short x,short y)
{
 HWND c=GetDlgItem(window,id);WORD down,up;
 if(!c){check("button_exists",0,1);return 0;}
 down=(WORD)(PostMessage(c,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(x,y))!=0);
 up=(WORD)(PostMessage(c,WM_LBUTTONUP,0,MAKELONG(x,y))!=0);
 if(!down||!up){check("button_down_up_posted",(DWORD)(down&&up),1);return 0;}
 return 1;
}
static WORD postKey(HWND window,WORD key)
{
 SetFocus(window);
 return (WORD)(PostMessage(window,WM_KEYDOWN,key,0L)!=0&&PostMessage(window,WM_KEYUP,key,0L)!=0);
}
static WORD visibleWindow(HWND w){return (WORD)(w&&IsWindow(w)&&IsWindowVisible(w));}
static HWND findPopup(HWND owner,char *wantedTitle)
{
 HWND w=GetWindow(GetDesktopWindow(),GW_CHILD),found=0;
 WORD walked=0,vis,own;char cls[32],txt[64];
 if(!owner||!IsWindow(owner))return 0;
 while(w&&walked++<256){
  if(GetWindow(w,GW_OWNER)==owner){
   vis=(WORD)(IsWindowVisible(w)!=0);own=(WORD)GetWindow(w,GW_OWNER);cls[0]=0;txt[0]=0;
   if(vis){GetClassName(w,cls,sizeof(cls));GetWindowText(w,txt,sizeof(txt));}
   if(!lastPopup||lastPopup!=w||lastPopupVisible!=vis||lastPopupOwner!=own||lstrcmp(popupTitle,txt)){
    wsprintf(line,"POPUP hwnd=%04X owner=%04X visible=%u class=[%s] title=[%s]",(WORD)w,own,vis,(LPSTR)cls,(LPSTR)txt);record(line);
    lastPopup=w;lastPopupVisible=vis;lastPopupOwner=own;lstrcpy(popupTitle,txt);
   }
   if(vis&&!lstrcmp(cls,"#32770")&&!lstrcmp(txt,wantedTitle))found=w;
  }
  w=GetWindow(w,GW_HWNDNEXT);
 }
 return found;
}
static void finish(void)
{
 if(done)return;done=1;wsprintf(line,"FINAL failures=%u checks=%u",failures,checks);record(line);PostQuitMessage(0);
}
static void failAndClose(char *why)
{
 HWND owned;
 check(why,0,1);failureAt=GetTickCount();
 owned=browser&&IsWindow(browser)?findPopup(browser,"File selection"):0;
 if(owned&&IsWindowVisible(owned))PostMessage(owned,WM_CLOSE,0,0L);
 owned=player&&IsWindow(player)?findPopup(player,"Add song"):0;
 if(owned&&IsWindowVisible(owned))PostMessage(owned,WM_CLOSE,0,0L);
 if(player&&IsWindow(player)){phaseSet(P_FAIL_CLOSE);}else finish();
}
static void closePlayer(void){if(player&&IsWindow(player))PostMessage(player,WM_CLOSE,0,0L);phaseSet(P_CLOSE_PLAYER);}
static void startBrowser(WORD purpose)
{
 browserPurpose=purpose;browser=0;lastPopup=0;popupTitle[0]=0;modalSampled=0;phaseAt=GetTickCount();
 addCountBefore=(WORD)query(0);record("DRIVER post real Add button down/up pair");
 if(postButton(player,ID_ADD,8,8))phaseSet(P_WAIT_ADD);else failAndClose("Add_button_pair");
}
static void inspectBrowser(void)
{
 HWND dir,filter;
 GetWindowText(browser,title,sizeof(title));check("browser_visible",IsWindowVisible(browser)!=0,1);
 check("browser_Add_song_title",!lstrcmp(title,"Add song"),1);GetClassName(browser,className,sizeof(className));check("browser_dialog_class",!lstrcmp(className,"#32770"),1);
 check("browser_owner_player",(WORD)GetWindow(browser,GW_OWNER),(WORD)player);check("browser_owner_disabled",IsWindowEnabled(player)==FALSE,1);
 check("browser_file_edit",GetDlgItem(browser,BROWSE_FILE)!=0,1);check("browser_file_list",GetDlgItem(browser,BROWSE_LIST)!=0,1);
 filter=GetDlgItem(browser,BROWSE_FILTER);check("browser_filter_combo",filter!=0,1);dir=GetDlgItem(browser,BROWSE_DIR);check("browser_directory_label",dir!=0,1);
 if(filter)check("browser_sound_filter_selected",SendMessage(filter,CB_GETCURSEL,0,0L),0);
 if(dir){GetWindowText(dir,value,sizeof(value));check("browser_initial_MMLAB",lstrcmpi(value,"C:\\MMLAB")==0||lstrcmpi(value,"MMLAB")==0||containsText(value,"MMLAB"),1);}
}
static void setAdd(char *leaf,WORD expectedCount,WORD expectedSelection,WORD continuation)
{
 WORD i;for(i=0;i<sizeof(addLeaf)-1&&leaf[i];++i)addLeaf[i]=leaf[i];addLeaf[i]=0;
 addExpectedCount=expectedCount;addExpectedSelection=expectedSelection;addContinuation=continuation;
 startBrowser(BROWSE_ADD);
}
static void prepareAddFromBrowser(void)
{
 HWND l=GetDlgItem(browser,BROWSE_LIST);
 if(!l){failAndClose("browser_real_file_list_missing");return;}
 addFileIndex=(int)SendMessage(l,LB_FINDSTRING,(WPARAM)0xffff,(LPARAM)(LPSTR)addLeaf);
 wsprintf(line,"BROWSER native index leaf=[%s] index=%d",(LPSTR)addLeaf,addFileIndex);record(line);
 check("browser_real_file_row_found",addFileIndex>=0,1);
 if(addFileIndex<0){failAndClose("requested_browser_file_missing");return;}
 SendMessage(l,LB_SETCURSEL,(WPARAM)addFileIndex,0L);
 SendMessage(browser,WM_COMMAND,BROWSE_LIST,MAKELONG(l,LBN_SELCHANGE));
 phaseSet(P_ADD_OPEN);
}
static void finishAdd(void)
{
 WORD step=(WORD)SendMessage(player,WM_USER+52,6,0L);
 check("successful_add_browser_step",step,6);check("successful_add_error_low",SendMessage(player,WM_USER+52,7,0L),0);check("successful_add_error_high",SendMessage(player,WM_USER+52,8,0L),0);listCount(addExpectedCount,"successful_add_count");
 selected(addExpectedSelection,"successful_add_selection");rowText(addExpectedSelection,addLeaf,"successful_add_row");caption("Tandy Winamp");
#if PLGUI_CASE == 1
 if(addContinuation==1){phaseSet(P_C1_NEXT_DUP);return;}
 if(addContinuation==2){phaseSet(P_C1_NEXT_MISSING);return;}
 closePlayer();return;
#elif PLGUI_CASE == 2
 ++c2AddIndex;
 if(c2AddIndex<5){phaseSet(P_C2_NEXT_ADD);return;}
 phaseSet(P_C2_KEYS_END);return;
#else
 phaseSet(P_C3_HOME_FIRST);return;
#endif
}
static void completeCancel(void)
{
 check("browser_cancel_return_step",SendMessage(player,WM_USER+52,6,0L),5);check("browser_cancel_error_low",SendMessage(player,WM_USER+52,7,0L),0);check("browser_cancel_error_high",SendMessage(player,WM_USER+52,8,0L),0);
 listCount(addCountBefore,"cancel_preserves_playlist");caption("Tandy Winamp");
#if PLGUI_CASE == 1
 if(cancelContinuation==1){freeAfter=GetFreeSpace(0);phaseSet(P_C1_NEXT_COPPER);return;}
 if(cancelContinuation==2){phaseSet(P_MISSING_PLAY);return;}
#elif PLGUI_CASE == 3
 check("active_browser_keeps_playing",SendMessage(player,WM_USER+47,0,0L),1);
 check("active_browser_error_zero",debug(6),0);check("active_browser_bytes_advance",debug(31)>bytesBefore,1);
 GetWindowText(GetDlgItem(player,ID_CLOCK),clockAfter,sizeof(clockAfter));check("active_browser_clock_advances",lstrcmp(clockAfter,clockBefore)!=0,1);
 phaseSet(P_C3_ROW_ACTIVE);return;
#endif
 closePlayer();
}
static void initPlayer(void)
{
 check("player_visible",player&&IsWindowVisible(player),1);caption("Tandy Winamp");
 listBox=(HWND)(WORD)query(2);check("query_native_list",listBox!=0,1);
 check("native_list_control_id",(WORD)GetDlgCtrlID(listBox),ID_LIST);GetClassName(listBox,value,sizeof(value));check("native_list_class",!lstrcmpi(value,"LISTBOX"),1);
 listCount(3,"initial_default_three");selected(0,"initial_selection_first");
 rowText(0,"STREAM.MID","default_stream_row");rowText(1,"PLUCK.TND","default_pluck_row");rowText(2,"CHIME.SND","default_chime_row");
#if PLGUI_CASE == 1
 addCountBefore=(WORD)query(0);gestureAddBefore=(WORD)SendMessage(player,WM_USER+52,5,0L);freeCold=GetFreeSpace(0);phaseSet(P_GEST_OUT);
#elif PLGUI_CASE == 2
 c2AddIndex=0;phaseSet(P_C2_NEXT_ADD);
#else
 phaseSet(P_C3_NEXT_ADD);
#endif
}
static void runTick(void)
{
 DWORD now=GetTickCount(),foregroundNow,bytesNow,modalElapsed;HWND d;WORD n,measureVisible,measureError,measureClock;
 if(done||busy)return;busy=1;
 if(phase&&phase!=P_FAIL_CLOSE&&phase!=P_CLOSE_PLAYER&&now-suiteAt>260000UL){failAndClose("suite_260_second_deadline");busy=0;return;}
 switch(phase){
 case P_START:
  suiteAt=now;check("owner_queue_exactly_64",queueOK,1);
  check("player_launch",WinExec("C:\\MMLAB\\MINIPLAY.EXE",SW_SHOWNORMAL)>=32,1);phaseSet(P_WAIT_PLAYER);break;
 case P_WAIT_PLAYER:
  player=FindWindow("MiniMCIPlayer",NULL);
  if(player&&IsWindowVisible(player))initPlayer();
  else if(now-phaseAt>30000UL)failAndClose("player_launch_deadline");
  break;
#if PLGUI_CASE == 1
 case P_GEST_OUT:
  {HWND c=GetDlgItem(player,ID_ADD);WORD a,b;if(!c){check("Add_button_exists",0,1);failAndClose("Add_button_missing");break;}
   gestureUpBefore=(WORD)SendMessage(player,WM_USER+52,1,0L);
   a=(WORD)(PostMessage(c,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(8,8))!=0);b=(WORD)(PostMessage(c,WM_LBUTTONUP,0,MAKELONG(0xffff,0xffff))!=0);check("outside_gesture_posted",a&&b,1);phaseSet(P_GEST_OUT_CHECK);}
  break;
 case P_GEST_OUT_CHECK:
  if((WORD)SendMessage(player,WM_USER+52,1,0L)==gestureUpBefore){if(now-phaseAt>5000UL)failAndClose("outside_UP_delivery_deadline");break;}
  check("outside_release_no_Add",SendMessage(player,WM_USER+52,5,0L),gestureAddBefore);phaseSet(P_GEST_CANCEL);break;
 case P_GEST_CANCEL:
  {HWND c=GetDlgItem(player,ID_ADD);WORD a,b,cancel;if(!c){check("Add_button_exists",0,1);failAndClose("Add_button_missing");break;}
   gestureUpBefore=(WORD)SendMessage(player,WM_USER+52,1,0L);
   a=(WORD)(PostMessage(c,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(8,8))!=0);cancel=(WORD)(PostMessage(c,WM_CANCELMODE,0,0L)!=0);b=(WORD)(PostMessage(c,WM_LBUTTONUP,0,MAKELONG(8,8))!=0);check("cancelmode_gesture_posted",a&&b&&cancel,1);phaseSet(P_GEST_CANCEL_CHECK);}
  break;
 case P_GEST_CANCEL_CHECK:
  if((WORD)SendMessage(player,WM_USER+52,1,0L)==gestureUpBefore){if(now-phaseAt>5000UL)failAndClose("cancel_UP_delivery_deadline");break;}
  check("cancel_before_release_no_Add",SendMessage(player,WM_USER+52,5,0L),gestureAddBefore);phaseSet(P_GEST_DUP);break;
 case P_GEST_DUP:
  {HWND c=GetDlgItem(player,ID_ADD);WORD a,b;if(!c){check("Add_button_exists",0,1);failAndClose("Add_button_missing");break;}
   gestureUpBefore=(WORD)SendMessage(player,WM_USER+52,1,0L);
   a=(WORD)(PostMessage(c,WM_LBUTTONUP,0,MAKELONG(8,8))!=0);b=(WORD)(GetWindowWord(c,0)==0);check("duplicate_release_without_press",a&&b,1);phaseSet(P_GEST_DUP_CHECK);}
  break;
 case P_GEST_DUP_CHECK:
  if((WORD)SendMessage(player,WM_USER+52,1,0L)==gestureUpBefore){if(now-phaseAt>5000UL)failAndClose("stray_UP_delivery_deadline");break;}
  check("duplicate_release_no_Add",SendMessage(player,WM_USER+52,5,0L),gestureAddBefore);cancelContinuation=1;startBrowser(BROWSE_CANCEL);break;
 case P_C1_NEXT_COPPER:
  setAdd("COPPER.VGM",4,3,1);break;
 case P_C1_NEXT_DUP:
  setAdd("COPPER.VGM",4,3,2);break;
 case P_C1_NEXT_MISSING:
  cancelContinuation=2;startBrowser(BROWSE_MISSING);break;
#endif
 case P_WAIT_ADD:
  d=findPopup(player,"Add song");
  if(d&&IsWindowVisible(d)){browser=d;inspectBrowser();
   wsprintf(line,"BUTTON_FLOW down=%lu up=%lu cancel=%lu pressed=%lu after=%lu Add=%lu step=%lu errlo=%lu errhi=%lu",
    SendMessage(player,WM_USER+52,0,0L),SendMessage(player,WM_USER+52,1,0L),SendMessage(player,WM_USER+52,2,0L),SendMessage(player,WM_USER+52,3,0L),SendMessage(player,WM_USER+52,4,0L),SendMessage(player,WM_USER+52,5,0L),SendMessage(player,WM_USER+52,6,0L),SendMessage(player,WM_USER+52,7,0L),SendMessage(player,WM_USER+52,8,0L));record(line);
#if PLGUI_CASE == 1
   if(browserPurpose==BROWSE_CANCEL)freeOpen=GetFreeSpace(0);
#endif
   if(browserPurpose==BROWSE_CANCEL)phaseSet(P_BROWSER_CANCEL);
   else if(browserPurpose==BROWSE_ADD)phaseSet(P_ADD_SELECT);
   else if(browserPurpose==BROWSE_MISSING)phaseSet(P_MISSING_TYPE);
   else if(browserPurpose==BROWSE_ACTIVE)phaseSet(P_C3_HOLD_BROWSER);
  }else if(now-phaseAt>20000UL)failAndClose("visible_Add_song_deadline");
  break;
 case P_BROWSER_CANCEL:
  if(!visibleWindow(browser)){failAndClose("browser_not_visible_before_cancel");break;}
  check("browser_Cancel_posted",postButton(browser,ID_CANCEL,8,8),1);phaseSet(P_WAIT_CANCEL);break;
 case P_WAIT_CANCEL:
  if(!IsWindow(browser)&& (WORD)SendMessage(player,WM_USER+52,6,0L)==5)completeCancel();
  else if(now-phaseAt>20000UL)failAndClose("cancel_browser_return_deadline");
  break;
 case P_ADD_SELECT:
  if(!visibleWindow(browser)){failAndClose("browser_not_visible_before_file_select");break;}
  prepareAddFromBrowser();break;
 case P_ADD_OPEN:
  if(!visibleWindow(browser)){failAndClose("browser_not_visible_before_Open");break;}
  check("browser_Open_posted",postButton(browser,ID_OPEN,8,8),1);phaseSet(P_WAIT_ADD_DONE);break;
 case P_WAIT_ADD_DONE:
  if(!IsWindow(browser)&&(WORD)SendMessage(player,WM_USER+52,6,0L)==6)finishAdd();
  else if(now-phaseAt>20000UL)failAndClose("successful_add_return_deadline");
  break;
 case P_MISSING_TYPE:
  if(!visibleWindow(browser)){failAndClose("browser_not_visible_before_missing_type");break;}
  d=GetDlgItem(browser,BROWSE_FILE);check("missing_filename_edit",d!=0,1);if(!d){failAndClose("missing_filename_edit_missing");break;}SendMessage(d,WM_SETTEXT,0,(LPARAM)(LPSTR)missingPath);phaseSet(P_MISSING_OPEN);break;
 case P_MISSING_OPEN:
  if(!visibleWindow(browser)){failAndClose("browser_not_visible_before_missing_Open");break;}
  check("missing_Open_posted",postButton(browser,ID_OPEN,8,8),1);phaseSet(P_WAIT_NOTICE);break;
 case P_WAIT_NOTICE:
  d=findPopup(browser,"File selection");
  if(d&&IsWindowVisible(d)){notice=d;check("missing_notice_visible",1,1);caption("Tandy Winamp");phaseSet(P_NOTICE_DISMISS);}
  else if(now-phaseAt>20000UL)failAndClose("missing_notice_deadline");
  break;
 case P_NOTICE_DISMISS:
  if(!visibleWindow(notice)){failAndClose("missing_notice_not_visible_before_dismiss");break;}
  check("missing_notice_dismiss_posted",postButton(notice,ID_OPEN,8,8),1);phaseSet(P_WAIT_NOTICE_GONE);break;
 case P_WAIT_NOTICE_GONE:
  if(!IsWindow(notice)&&IsWindow(browser))phaseSet(P_BROWSER_CANCEL);
  else if(now-phaseAt>20000UL)failAndClose("missing_notice_dismiss_deadline");
  break;
#if PLGUI_CASE == 1
 case P_MISSING_PLAY:
  SendMessage(GetDlgItem(player,ID_FILE),WM_SETTEXT,0,(LPARAM)(LPSTR)missingPath);phaseSet(P_MISSING_PLAY_CHECK);break;
 case P_MISSING_PLAY_CHECK:
  check("missing_Play_posted",postButton(player,ID_PLAY,8,8),1);phaseSet(P_MISSING_PLAY_VERIFY);break;
 case P_MISSING_PLAY_VERIFY:
  GetWindowText(GetDlgItem(player,ID_STATE),value,sizeof(value));
  if(lstrcmp(value,"Cannot open file")){if(now-phaseAt>20000UL)failAndClose("missing_Play_refusal_deadline");break;}
  check("missing_Play_refused",SendMessage(player,WM_USER+47,0,0L),0);caption("Tandy Winamp");selected(0xffff,"missing_Play_clears_selection");phaseSet(P_REMOVE_NONE);break;
 case P_REMOVE_NONE:
  check("no_selection_native_list",selectedIndex()==0xffff,1);check("no_selection_Remove_posted",postButton(player,ID_REMOVE,8,8),1);phaseSet(P_REMOVE_NONE_CHECK);break;
 case P_REMOVE_NONE_CHECK:
  listCount(4,"no_selection_Remove_unchanged");caption("Tandy Winamp");freeWarm=GetFreeSpace(0);wsprintf(line,"MEMORY cold=%lu dialog=%lu released=%lu final=%lu",freeCold,freeOpen,freeAfter,freeWarm);record(line);check("browser_heap_loss_bounded",freeWarm+4096UL>=freeAfter,1);closePlayer();break;
#endif
#if PLGUI_CASE == 2
 case P_C2_NEXT_ADD:
  {static char *names[]={"COPPER.VGM","F02.VGM","F03.VGM","F04.VGM","F05.VGM"};
   setAdd(names[c2AddIndex],(WORD)(4+c2AddIndex),(WORD)(3+c2AddIndex),6);}
  break;
 case P_C2_KEYS_END:
  check("End_key_posted",postKey(listBox,VK_END),1);phaseSet(P_C2_KEYS_END_CHECK);break;
 case P_C2_KEYS_END_CHECK:
  selected(7,"keyboard_End");phaseSet(P_C2_KEYS_HOME);break;
 case P_C2_KEYS_HOME:
  check("Home_key_posted",postKey(listBox,VK_HOME),1);phaseSet(P_C2_KEYS_HOME_CHECK);break;
 case P_C2_KEYS_HOME_CHECK:
  selected(0,"keyboard_Home");phaseSet(P_C2_KEYS_NEXT);break;
 case P_C2_KEYS_NEXT:
  check("PageDown_key_posted",postKey(listBox,VK_NEXT),1);phaseSet(P_C2_KEYS_NEXT_CHECK);break;
 case P_C2_KEYS_NEXT_CHECK:
  n=(WORD)query(1);check("keyboard_PageDown_moves",n>0&&n<8,1);selected(n,"keyboard_PageDown_sync");addExpectedSelection=n;phaseSet(P_C2_KEYS_PRIOR);break;
 case P_C2_KEYS_PRIOR:
  check("PageUp_key_posted",postKey(listBox,VK_PRIOR),1);phaseSet(P_C2_KEYS_PRIOR_CHECK);break;
 case P_C2_KEYS_PRIOR_CHECK:
  n=(WORD)query(1);check("keyboard_PageUp_moves_back",n<addExpectedSelection,1);selected(n,"keyboard_PageUp_sync");phaseSet(P_C2_NARROW);break;
 case P_C2_NARROW:
  MoveWindow(player,0,0,156,192,TRUE);phaseSet(P_C2_NARROW_END);break;
 case P_C2_NARROW_END:
  check("narrow_End_key_posted",postKey(listBox,VK_END),1);phaseSet(P_C2_NARROW_CHECK);break;
 case P_C2_NARROW_CHECK:
  selected(7,"narrow_End");check("narrow_list_scrollbar_moves",GetScrollPos(listBox,SB_VERT)>0,1);phaseSet(P_C2_RESTORE);break;
 case P_C2_RESTORE:
  MoveWindow(player,0,0,316,192,TRUE);selected(7,"restored_selection");caption("Tandy Winamp");closePlayer();break;
#endif
#if PLGUI_CASE == 3
 case P_C3_HOME_FIRST:
  check("Home_before_mouse_posted",postKey(listBox,VK_HOME),1);phaseSet(P_C3_HOME_FIRST_CHECK);break;
 case P_C3_HOME_FIRST_CHECK:
  selected(0,"mouse_selection_starts_at_zero");phaseSet(P_C3_ROW);break;
 case P_C3_NEXT_ADD:
  setAdd("COPPER.VGM",4,3,5);break;
 case P_C3_ROW:
  if(SendMessage(listBox,LB_GETITEMRECT,3,(LPARAM)(LPSTR)&rowRect)==LB_ERR){check("Copper_row_rect",0,1);failAndClose("Copper_row_missing");break;}
  SetFocus(listBox);check("Copper_row_click_pair",PostMessage(listBox,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG((rowRect.left+rowRect.right)/2,(rowRect.top+rowRect.bottom)/2))&&PostMessage(listBox,WM_LBUTTONUP,0,MAKELONG((rowRect.left+rowRect.right)/2,(rowRect.top+rowRect.bottom)/2)),1);phaseSet(P_C3_ROW_CHECK);break;
 case P_C3_ROW_CHECK:
  selected(3,"select_Copper_for_play");phaseSet(P_C3_PLAY);break;
 case P_C3_PLAY:
  check("Play_button_posted",postButton(player,ID_PLAY,8,8),1);phaseSet(P_C3_PLAY_CHECK);break;
 case P_C3_PLAY_CHECK:
  if(now-phaseAt<600UL)break;
  check("copper_playing",SendMessage(player,WM_USER+47,0,0L),1);check("copper_play_error",debug(6),0);check("copper_psg_bytes_nonzero",debug(31)>0,1);caption("Tandy Winamp");
  GetWindowText(GetDlgItem(player,ID_CLOCK),clockBefore,sizeof(clockBefore));bytesBefore=debug(31);foregroundBefore=SendMessage(player,WM_USER+42,0,0L);phaseSet(P_C3_START_BROWSER);break;
 case P_C3_START_BROWSER:
  startBrowser(BROWSE_ACTIVE);break;
 case P_C3_HOLD_BROWSER:
  if(!visibleWindow(browser)){failAndClose("active_browser_not_visible_before_Cancel");break;}
  if(!modalSampled){
   modalSampled=1;bytesBefore=debug(31);foregroundBefore=SendMessage(player,WM_USER+42,0,0L);
   GetWindowText(GetDlgItem(player,ID_CLOCK),clockBefore,sizeof(clockBefore));phaseAt=GetTickCount();record("MODAL baseline sampled with browser visible");break;
  }
  if(now-phaseAt>=2000UL){
   foregroundNow=SendMessage(player,WM_USER+42,0,0L);bytesNow=debug(31);
   measureVisible=visibleWindow(browser);measureError=debug(6);GetWindowText(GetDlgItem(player,ID_CLOCK),clockAfter,sizeof(clockAfter));measureClock=(WORD)(lstrcmp(clockAfter,clockBefore)!=0);modalElapsed=now-phaseAt;
   check("active_browser_Cancel_posted",postButton(browser,ID_CANCEL,8,8),1);phaseSet(P_C3_WAIT_ACTIVE_CANCEL);
   wsprintf(line,"MODAL elapsed=%lu foreground_before=%lu after=%lu bytes_before=%lu after=%lu",modalElapsed,foregroundBefore,foregroundNow,bytesBefore,bytesNow);record(line);
   check("modal_visible_during_progress",measureVisible,1);
   check("modal_foreground_pump_progress",foregroundNow>foregroundBefore,1);check("modal_PSG_bytes_progress",bytesNow>bytesBefore,1);check("modal_error_zero",measureError,0);check("modal_clock_progress",measureClock,1);
  }break;
 case P_C3_WAIT_ACTIVE_CANCEL:
  if(!IsWindow(browser)&&(WORD)SendMessage(player,WM_USER+52,6,0L)==5){
   check("active_Cancel_error_low",SendMessage(player,WM_USER+52,7,0L),0);check("active_Cancel_error_high",SendMessage(player,WM_USER+52,8,0L),0);listCount(addCountBefore,"active_Cancel_preserves_playlist");
   check("active_browser_keeps_playing",SendMessage(player,WM_USER+47,0,0L),1);check("active_browser_error_zero",debug(6),0);
   caption("Tandy Winamp");phaseSet(P_C3_ROW_ACTIVE);
  }else if(now-phaseAt>20000UL)failAndClose("active_browser_cancel_deadline");
  break;
 case P_C3_ROW_ACTIVE:
  if(SendMessage(listBox,LB_GETITEMRECT,3,(LPARAM)(LPSTR)&rowRect)==LB_ERR){check("playing_Copper_row_rect",0,1);failAndClose("playing_Copper_row_missing");break;}
  SetFocus(listBox);check("playing_Copper_row_click_pair",PostMessage(listBox,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG((rowRect.left+rowRect.right)/2,(rowRect.top+rowRect.bottom)/2))&&PostMessage(listBox,WM_LBUTTONUP,0,MAKELONG((rowRect.left+rowRect.right)/2,(rowRect.top+rowRect.bottom)/2)),1);phaseSet(P_C3_ROW_ACTIVE_CHECK);break;
 case P_C3_ROW_ACTIVE_CHECK:
  selected(3,"playing_Copper_selected");phaseSet(P_C3_REMOVE_ACTIVE);break;
 case P_C3_REMOVE_ACTIVE:
  check("Remove_active_button_posted",postButton(player,ID_REMOVE,8,8),1);phaseSet(P_C3_REMOVE_ACTIVE_CHECK);break;
 case P_C3_REMOVE_ACTIVE_CHECK:
  listCount(3,"remove_playing_row_decrements_list");check("remove_active_keeps_playing",SendMessage(player,WM_USER+47,0,0L),1);GetWindowText(GetDlgItem(player,ID_TRACK),value,sizeof(value));check("remove_active_keeps_track",!lstrcmpi(value,"COPPER.VGM"),1);caption("Tandy Winamp");
  {HFILE h=OpenFile("C:\\MMLAB\\COPPER.VGM",&fileInfo,OF_READ|OF_SHARE_DENY_WRITE);check("removed_song_file_still_exists",h!=HFILE_ERROR,1);if(h!=HFILE_ERROR)_lclose(h);}phaseSet(P_C3_HOME);break;
 case P_C3_HOME:
  check("Home_key_posted",postKey(listBox,VK_HOME),1);phaseSet(P_C3_HOME_CHECK);break;
 case P_C3_HOME_CHECK:
  selected(0,"ordinary_row_selected");phaseSet(P_C3_REMOVE_ORDINARY);break;
 case P_C3_REMOVE_ORDINARY:
  check("Remove_ordinary_button_posted",postButton(player,ID_REMOVE,8,8),1);phaseSet(P_C3_REMOVE_ORDINARY_CHECK);break;
 case P_C3_REMOVE_ORDINARY_CHECK:
  listCount(2,"remove_ordinary_row_decrements_list");check("ordinary_remove_keeps_playing",SendMessage(player,WM_USER+47,0,0L),1);caption("Tandy Winamp");phaseSet(P_C3_STOP);break;
 case P_C3_STOP:
  check("Stop_button_posted",postButton(player,ID_STOP,8,8),1);phaseSet(P_C3_STOP_CHECK);break;
 case P_C3_STOP_CHECK:
  if(now-phaseAt<400UL)break;
  check("stop_inactive",SendMessage(player,WM_USER+47,0,0L),0);check("stop_sound_released",OpenSound(),3);CloseSound();closePlayer();break;
#endif
 case P_FAIL_CLOSE:
  if(now-failureAt<2000UL){
   d=browser&&IsWindow(browser)?findPopup(browser,"File selection"):0;if(d)PostMessage(d,WM_CLOSE,0,0L);
   d=player&&IsWindow(player)?findPopup(player,"Add song"):0;if(d)PostMessage(d,WM_CLOSE,0,0L);break;
  }
  if(player&&IsWindow(player))PostMessage(player,WM_CLOSE,0,0L);
  phaseSet(P_CLOSE_PLAYER);
  break;
 case P_CLOSE_PLAYER:
  if(!player||!IsWindow(player)){check("player_closed",1,1);check("close_sound_released",OpenSound(),3);CloseSound();finish();}
  else if(now-phaseAt>15000UL){check("player_close_deadline",0,1);finish();}
  break;
 default:break;
 }
 busy=0;
}
void FAR PASCAL ClockProc(HWND w,UINT m,UINT id,DWORD tick)
{
 (void)w;(void)m;(void)id;(void)tick;runTick();
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp)
{
 if(m==WM_CLOSE){PostQuitMessage(0);return 0L;}
 return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
 WNDCLASS wc;int i;HFILE f;MSG msg;
 (void)prev;(void)cmd;(void)show;
 f=_lcreat("C:\\PLGUI.LOG",0);if(f==HFILE_ERROR)return 2;_lclose(f);
 queueOK=(WORD)(SetMessageQueue(64)!=0);check("SetMessageQueue_64",queueOK,1);if(!queueOK){finish();ExitWindows(0L,0);return 3;}
 for(i=0;i<sizeof(wc);++i)((BYTE FAR *)&wc)[i]=0;
 wc.lpfnWndProc=WndProc;wc.hInstance=inst;wc.lpszClassName="PLASYNCOWNER";wc.hbrBackground=GetStockObject(WHITE_BRUSH);
 if(!RegisterClass(&wc)){check("RegisterClass",0,1);finish();ExitWindows(0L,0);return 4;}
 rootWindow=CreateWindow("PLASYNCOWNER","Playlist async QA",WS_OVERLAPPEDWINDOW,480,0,150,60,NULL,NULL,inst,NULL);
 if(!rootWindow){check("CreateWindow",0,1);finish();ExitWindows(0L,0);return 4;}
 ShowWindow(rootWindow,SW_SHOW);UpdateWindow(rootWindow);
 timerThunk=MakeProcInstance((FARPROC)ClockProc,inst);
 if(!timerThunk){check("MakeProcInstance",0,1);finish();ExitWindows(0L,0);return 5;}
 timerId=SetTimer(NULL,0,200,(TIMERPROC)timerThunk);
 if(!timerId){check("SetTimer",0,1);FreeProcInstance(timerThunk);finish();ExitWindows(0L,0);return 5;}
 phaseSet(P_START);
 while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
 KillTimer(NULL,timerId);FreeProcInstance(timerThunk);
 ExitWindows(0L,0);return failures?1:0;
}
