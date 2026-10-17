#define WINVER 0x0300
#include <windows.h>
static HFILE log;static WORD checks,failures,token;static char b[200],name[144];
static void check(char *n,DWORD got,DWORD want){checks++;if(got!=want)failures++;wsprintf(b,"CHECK %s got=%lu want=%lu\r\n",(LPSTR)n,got,want);_lwrite(log,b,lstrlen(b));_lclose(log);log=_lopen("C:\\RAWGUI.LOG",OF_WRITE);_llseek(log,0L,2);}
static void waitms(WORD ms){DWORD t=GetTickCount();MSG m;while(GetTickCount()-t<ms){while(PeekMessage(&m,0,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessage(&m);}Yield();}}
static void ack(HWND w){DWORD t=GetTickCount();++token;PostMessage(w,WM_USER+45,token,0L);while((WORD)SendMessage(w,WM_USER+46,0,0L)!=token&&GetTickCount()-t<10000UL)waitms(55);check("queued_input_ack",SendMessage(w,WM_USER+46,0,0L),token);}
static void button(HWND w,WORD id){HWND b=GetDlgItem(w,id);PostMessage(b,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(8,8));PostMessage(b,WM_LBUTTONUP,0,MAKELONG(8,8));ack(w);}
static void path(HWND w,LPSTR p){PostMessage(GetDlgItem(w,103),WM_SETTEXT,0,(LONG)p);ack(w);}
static void state(HWND w,LPSTR expected){GetWindowText(GetDlgItem(w,104),name,sizeof(name));check("visible_state",!lstrcmp(name,expected),1);}
int PASCAL WinMain(HINSTANCE i,HINSTANCE p,LPSTR c,int show)
{
 HWND w;WORD child,k;DWORD before,freeBefore,freeAfter;
 log=_lcreat("C:\\RAWGUI.LOG",0);child=WinExec("C:\\MMLAB\\MINIPLAY.EXE",SW_SHOWNORMAL);wsprintf(b,"LAUNCH_RESULT %u\r\n",child);_lwrite(log,b,lstrlen(b));check("launch",child>=32,1);waitms(1000);w=FindWindow("MiniMCIPlayer",0);check("visible_window",w&&IsWindowVisible(w),1);if(!w)goto done;
 freeBefore=GetFreeSpace(0);button(w,101);waitms(1200);state(w,"Playing");check("MIDI_baseline_active",SendMessage(w,WM_USER+47,0,0L),1);
 path(w,"C:\\MMLAB\\STRESS.TND");button(w,101);waitms(1200);state(w,"Queued");check("TND_active",SendMessage(w,WM_USER+47,0,0L),1);
 check("TND_exclusive_other_task",OpenSound()<0,1);CloseSound();
 before=SendMessage(w,WM_USER+49,0,0L);check("TND_PSG_writes",before>20,1);
 MoveWindow(w,0,0,156,192,TRUE);waitms(500);ShowWindow(w,SW_HIDE);waitms(250);ShowWindow(w,SW_SHOWNORMAL);waitms(400);state(w,"Queued");check("redraw_kept_playing",SendMessage(w,WM_USER+47,0,0L),1);
 check("redraw_no_service_error",SendMessage(w,WM_USER+49,5,0L),0);
 wsprintf(b,"TIMING UI max_raw_gap_units64=%u ticks=%u writes=%u\r\n",(WORD)SendMessage(w,WM_USER+49,2,0L),(WORD)SendMessage(w,WM_USER+49,6,0L),(WORD)SendMessage(w,WM_USER+49,1,0L));_lwrite(log,b,lstrlen(b));
 button(w,102);state(w,"Stopped");check("TND_Stop_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\CHIME.SND");button(w,101);waitms(1600);state(w,"Done");check("AGI_EOF_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\FX.TND");button(w,101);waitms(700);state(w,"Queued");
 path(w,"C:\\MMLAB\\TYPE0.MID");button(w,101);waitms(1600);state(w,"Done");check("switch_back_MIDI_EOF",SendMessage(w,WM_USER+47,0,0L),0);
 path(w,"C:\\MMLAB\\BAD.TND");button(w,101);check("malformed_inactive",SendMessage(w,WM_USER+47,0,0L),0);check("malformed_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\FAST.TND");button(w,101);state(w,"Raw E2");check("fast_tempo_inactive",SendMessage(w,WM_USER+47,0,0L),0);check("fast_tempo_sound_free",OpenSound(),3);CloseSound();
 for(k=0;k<3;k++){path(w,"C:\\MMLAB\\STRESS.TND");button(w,101);waitms(200);check("restart_active",SendMessage(w,WM_USER+47,0,0L),1);button(w,102);check("restart_Stop_inactive",SendMessage(w,WM_USER+47,0,0L),0);}
 freeAfter=GetFreeSpace(0);wsprintf(b,"MEMORY free_before=%lu free_after=%lu\r\n",freeBefore,freeAfter);_lwrite(log,b,lstrlen(b));check("no_large_heap_loss",freeAfter+4096UL>=freeBefore,1);
 path(w,"C:\\MMLAB\\STRESS.TND");button(w,101);waitms(300);PostMessage(w,WM_CLOSE,0,0L);waitms(800);check("Close_destroyed",IsWindow(w),0);check("Close_sound_free",OpenSound(),3);CloseSound();check("Windows_still_running",1,1);
done:wsprintf(b,"FINAL failures=%u checks=%u\r\n",failures,checks);_lwrite(log,b,lstrlen(b));_lclose(log);ExitWindows(0L,0);return failures;
}
