/* Shared app-local MCI MIDI service, foreground SMF renderer + RAWCLK. */
#define WINVER 0x0300
#define MCI_IMPL
#include <windows.h>
#include "MCIAPI.H"
#include "SMF.H"
#include "MCIPSG.H"
#include "MIDISKIP.H"
static HTASK owner,opening;static HWND service,notify;static WORD device,serial=1,playing,paused,inPump,count,fileBytes,openStage,refillCalls,maxPumpMs;
static HFILE hfile=-1;static DWORD length,position;static HMODULE pin;
static int valid(WORD id){return device&&id==device&&owner==GetCurrentTask();}
static void signal(WORD reason){HWND w=notify;notify=0;if(w&&IsWindow(w))PostMessage(w,MC_NOTIFYMSG,reason,(LONG)device);}
WORD MCEXPORT MciDebug(WORD q){switch(q){case 1:return count;case 2:return SCDebug(6);case 3:return refillCalls;case 4:return maxPumpMs;case 5:return 0;case 6:return fileBytes;case 7:return 1024;case 8:return SCDebug(3);case 9:return SCDebug(2);case 10:return paused;}return openStage;}
WORD MCEXPORT MciPump(void){WORD r;DWORD then,span;if(!valid(device)||!playing||inPump)return 0;inPump=1;then=GetTickCount();r=SCPump();position=SCPosition();refillCalls++;
 if(r!=1){if(!SCStop()){playing=paused=0;signal(r?MC_FAILURE:MC_SUCCESS);}}
 span=GetTickCount()-then;if(span>maxPumpMs)maxPumpMs=span>65535UL?65535U:(WORD)span;inPump=0;return 0;}
LONG MCEXPORT MciWnd(HWND w,WORD m,WORD a,LONG b){if(m==WM_TIMER){MciPump();return 0L;}if((m==WM_CLOSE||(m==WM_ENDSESSION&&a))&&valid(device))MciCommand(device,MC_CLOSE,0L,0L);return DefWindowProc(w,m,a,b);}
static void freeFile(void){smf_close();if(hfile>=0)_lclose(hfile);hfile=-1;count=0;}
/* The song stays on disk: the parser pulls small per-track windows. */
static unsigned readAt(DWORD off,unsigned char *dst,unsigned n){
    if(hfile<0||_llseek(hfile,(LONG)off,0)<0)return 0;
    return _lread(hfile,dst,n)==n?n:0;}
static DWORD readFile(LPCSTR name)
{
    LONG size;int r;static SMF_EVENT e;WORD group=0;DWORD msg;DWORD groupAt=0xffffffffUL;
    hfile=_lopen(name,OF_READ|OF_SHARE_DENY_WRITE);if(hfile<0)return MC_BADFILE;
    size=_llseek(hfile,0L,2);
    if(size<=0L)return MC_BADFILE;
    if(!smf_open_stream(readAt,(DWORD)size))return MC_BADFILE;
    length=smf_duration_ms();
    if(!smf_event_count())return MC_BADFILE;
    count=smf_event_count();fileBytes=size>65535L?65535U:(WORD)size;
    while((r=smf_next(&e))==1){
        if(e.ms!=groupAt){groupAt=e.ms;group=0;}
        msg=(DWORD)e.status|((DWORD)e.data1<<8)|((DWORD)e.data2<<16);
        if(MSSkipMessage(msg))continue;
        if(++group>1024)return MC_BADFILE;
        if(!MSValidMessage(msg,1))
            return MC_UNSUPPORTED;
    }
    return r<0?MC_BADFILE:0L;
}

DWORD MCEXPORT MciCommand(WORD id,WORD message,DWORD flags,DWORD params){MCOPEN FAR *op;MCSTATUS FAR *st;MCSET FAR *set;MCGEN FAR *gp;DWORD result;WORD r;HMODULE h;static WNDCLASS cls;static char name[144];
 if(message==MC_OPEN){if(device||opening)return MC_BUSY;if(id)return MC_BADID;if(flags!=(MC_ELEMENT|MC_TYPE|MC_TYPEID))return MC_FLAGS;if(!params)return MC_PARAM;
  op=(MCOPEN FAR *)params;if(op->type!=(LPCSTR)523UL||!op->element)return MC_UNSUPPORTED;opening=GetCurrentTask();openStage=1;result=readFile(op->element);if(result){freeFile();opening=0;return result;}
  h=GetModuleHandle("MCITNDY");if(!GetModuleFileName(h,name,sizeof(name))){freeFile();opening=0;return MC_BUSY;}pin=LoadLibrary(name);if(pin<32){pin=0;freeFile();opening=0;return MC_BUSY;}
  cls.style=0;cls.lpfnWndProc=MciWnd;cls.cbClsExtra=cls.cbWndExtra=0;cls.hInstance=h;cls.hIcon=0;cls.hCursor=0;cls.hbrBackground=0;cls.lpszMenuName=0;cls.lpszClassName="MCITNDYService";RegisterClass(&cls);
  service=CreateWindow("MCITNDYService","",0,0,0,1,1,0,0,h,0L);if(!service||!SetTimer(service,1,55,0)){if(service)DestroyWindow(service);service=0;UnregisterClass("MCITNDYService",h);h=pin;pin=0;freeFile();opening=0;FreeLibrary(h);return MC_BUSY;}
  owner=GetCurrentTask();device=serial++;if(!serial)serial=1;position=0;playing=paused=notify=0;op->device=device;opening=openStage=0;return 0;}
 if(!valid(id))return MC_BADID;
 if(message==MC_STATUS){if(flags!=MC_ITEM)return MC_FLAGS;if(!params)return MC_PARAM;st=(MCSTATUS FAR *)params;MciPump();switch(st->item){case MC_LENGTH:st->result=length;break;case MC_POSITION:st->result=position;break;case MC_MODE:st->result=paused?MC_PAUSED:playing?MC_PLAYING:MC_STOPPED;break;case MC_FORMAT:st->result=0;break;case MC_READY:st->result=1;break;default:return MC_UNSUPPORTED;}return 0;}
 if(message==MC_SET){if(flags!=MC_TIME)return MC_FLAGS;if(!params)return MC_PARAM;set=(MCSET FAR *)params;return set->format?MC_UNSUPPORTED:0;}
 if(message==MC_PLAY){if(flags&~MC_NOTIFY)return MC_FLAGS;if((flags&MC_NOTIFY)&&!params)return MC_PARAM;gp=(MCGEN FAR *)params;
  if((flags&MC_NOTIFY)&&(!IsWindow((HWND)gp->callback)||GetWindowTask((HWND)gp->callback)!=owner))return MC_PARAM;
  if(playing)return MC_BUSY;if(SCStart())return MC_BUSY;playing=1;paused=0;position=0;refillCalls=maxPumpMs=0;notify=(flags&MC_NOTIFY)?(HWND)gp->callback:0;return 0;}
 if(message==MC_PAUSE||message==MC_RESUME){if(flags)return MC_FLAGS;if(!playing)return MC_BUSY;r=message==MC_PAUSE?SCPause():SCResume();if(r)return MC_BUSY;paused=message==MC_PAUSE;position=SCPosition();return 0;}
 if(message==MC_STOP||message==MC_CLOSE){if(flags)return MC_FLAGS;if(playing)position=SCPosition();if(SCStop())return MC_BUSY;playing=paused=0;signal(MC_ABORTED);
  if(message==MC_CLOSE){KillTimer(service,1);DestroyWindow(service);service=0;UnregisterClass("MCITNDYService",GetModuleHandle("MCITNDY"));freeFile();device=0;owner=0;h=pin;pin=0;if(h)FreeLibrary(h);}return 0;}
 return MC_UNSUPPORTED;}
DWORD MCEXPORT MciString(LPSTR cmd,LPSTR out,WORD size,HWND w){return MC_UNSUPPORTED;}
WORD MCEXPORT MciError(DWORD error,LPSTR out,WORD size){static char text[]="Bounded private MCI: operation refused";WORD i;if(!out||!size)return 0;for(i=0;i+1<size&&text[i];i++)out[i]=text[i];out[i]=0;return 1;}
int MCEXPORT WEP(int reason){return 1;}
