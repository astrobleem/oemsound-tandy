#define WINVER 0x0300
#include <windows.h>
#include "RAWAPP.H"
#include "MCIAPI.H"
extern void CoreTests(void);
typedef DWORD (FAR PASCAL *CMD)(WORD,WORD,DWORD,DWORD);
static CMD command;static HMODULE module;
static MCOPEN op;static MCGEN gp;
static WORD checks,failures,phase,dev,i,pumpResult;static DWORD since,freeBefore,lowFree;
static HFILE log;static char text[160];static HWND window;
static void emit(void){_lwrite(log,text,lstrlen(text));_lclose(log);log=_lopen("C:\\RAWQA.LOG",OF_WRITE);_llseek(log,0L,2);}
void Check(char *name,int yes){checks++;if(!yes)failures++;wsprintf(text,"CHECK %s got=%u want=1\r\n",(LPSTR)name,yes!=0);emit();}
static void midiClose(void){if(dev){command(dev,MC_STOP,0L,0L);Check("MIDI_close",!command(dev,MC_CLOSE,0L,0L));dev=0;}}
static void midiPlay(void){DWORD r;op.callback=0;op.type=(LPCSTR)523UL;op.element="C:\\MMLAB\\TYPE0.MID";op.alias=0;
 r=command(0,MC_OPEN,MC_ELEMENT|MC_TYPE|MC_TYPEID,(DWORD)(LPVOID)&op);Check("MIDI_open",!r);
 if(!r){dev=op.device;gp.callback=0;Check("MIDI_play",!command(dev,MC_PLAY,0L,(DWORD)(LPVOID)&gp));}}
LONG FAR PASCAL WndProc(HWND w,WORD m,WORD a,LONG b)
{
 if(m==WM_TIMER){
  if(phase==1){pumpResult=RawPump();if(GetTickCount()-since<1000UL)return 0;
   Check("TND_playing",pumpResult==1);Check("TND_owner",RawDebug(0)==1);Check("TND_PSGlatch_writes",RawDebug(1)>20);
   Check("TND_serviced",RawDebug(3)>5);Check("TND_gap_bounded",RawDebug(2)<=500);
   wsprintf(text,"TIMING TND max_gap_ms=%u pumps=%u ticks=%u\r\n",RawDebug(2),RawDebug(3),RawDebug(6));emit();
   Check("TND_Stop",!RawClose());Check("release_OpenSound",OpenSound()==3);CloseSound();
   Check("AGI_switch",!RawOpen("C:\\MMLAB\\CHIME.SND",2));since=GetTickCount();phase=2;return 0;
  }
  if(phase==2){pumpResult=RawPump();if(pumpResult==1 && GetTickCount()-since<2500UL)return 0;
   Check("AGI_EOF",pumpResult==0);Check("AGI_no_error",!RawDebug(5));Check("AGI_close",!RawClose());
   midiPlay();Check("raw_refuses_MIDI_owner",RawOpen("C:\\MMLAB\\PLUCK.TND",1)==7);midiClose();
   Check("raw_after_MIDI",!RawOpen("C:\\MMLAB\\FX.TND",1));Check("raw_Stop_switch",!RawClose());
   for(i=0;i<12;i++){Check("restart_open",!RawOpen(i&1?"C:\\MMLAB\\CHIME.SND":"C:\\MMLAB\\PLUCK.TND",i&1?2:1));Check("restart_close",!RawClose());}
   Check("no_live_owner",!RawDebug(0));Check("idle_pump_refused",RawPump()==2);
   lowFree=GetFreeSpace(0);wsprintf(text,"MEMORY free_before=%lu free_after=%lu\r\n",freeBefore,lowFree);emit();
   Check("no_large_heap_loss",lowFree+4096UL>=freeBefore);
   Check("sound_available_final",OpenSound()==3);CloseSound();PostMessage(w,WM_CLOSE,0,0L);return 0;
  }
 }
 if(m==WM_CLOSE){KillTimer(w,1);RawClose();midiClose();DestroyWindow(w);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProc(w,m,a,b);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
 WNDCLASS wc;MSG msg;log=_lcreat("C:\\RAWQA.LOG",0);Check("realmode_8088",!(GetWinFlags()&WF_PMODE));CoreTests();
 module=LoadLibrary("C:\\MMLAB\\MCITNDY.DLL");Check("load_MIDI",module>=32);
 if(module>=32)command=(CMD)GetProcAddress(module,(LPSTR)701L);Check("MCI_export",command!=0);
 if(!command)goto finish;
 Check("malformed_no_owner",RawOpen("C:\\MMLAB\\BAD.TND",1)!=0&&!RawDebug(0));
 freeBefore=GetFreeSpace(0);Check("TND_open",!RawOpen("C:\\MMLAB\\PLUCK.TND",1));
 Check("TND_initial_period",RawDebug(7)==426);Check("TND_initial_volume",RawDebug(8)==2);
 wc.style=0;wc.lpfnWndProc=WndProc;wc.cbClsExtra=wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=0;wc.hCursor=0;wc.hbrBackground=0;wc.lpszMenuName=0;wc.lpszClassName="RawQA";
 RegisterClass(&wc);window=CreateWindow("RawQA","",0,0,0,0,0,0,0,inst,0L);Check("window",window!=0);
 if(!window)goto finish;phase=1;since=GetTickCount();SetTimer(window,1,55,0);
 while(GetMessage(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);}
finish:
 RawClose();if(module>=32)FreeLibrary(module);
 wsprintf(text,"FINAL failures=%u checks=%u\r\n",failures,checks);emit();_lclose(log);ExitWindows(0L,0);return failures;
}
