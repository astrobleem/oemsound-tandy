/* Host-only mocks: no target build and no physical I/O. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t DWORD;
typedef uint16_t WORD, WPARAM;
typedef int32_t LONG, LPARAM;
typedef unsigned UINT;
typedef void *HWND;
typedef void *HINSTANCE;
typedef char *LPSTR;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; } MSG;
typedef struct {
 UINT style; LONG (*lpfnWndProc)(HWND,UINT,WPARAM,LPARAM);
 int cbClsExtra,cbWndExtra; HINSTANCE hInstance;
 void *hIcon,*hCursor,*hbrBackground;
 const char *lpszMenuName,*lpszClassName;
} WNDCLASS;
#define FAR
#define PASCAL
#define TRUE 1
#define WF_PMODE 1
#define WS_POPUP 0x80000000UL
#define WM_CREATE 1
#define WM_DESTROY 2
#define WM_CLOSE 0x10
#define WM_QUERYENDSESSION 0x11
#define WM_ENDSESSION 0x16
#define WM_TIMER 0x113
#define WIN ((HWND)(uintptr_t)1)
static DWORD now_tick;
static unsigned win_flags;
static unsigned char hardware_signature;
static int timer_ok,class_ok,window_ok,window_alive;
static int timer_calls,kill_calls,destroy_calls,quit_calls;
static int flags_calls,hardware_reads,register_calls,create_calls;
static int translate_calls,dispatch_calls,getmessage_calls;
static int port_count,check_count,live_timer;
static unsigned char port_bytes[8192];
static DWORD port_ticks[8192];
static MSG queue_messages[64];
static DWORD queue_ticks[64];
static int queue_length,queue_cursor,quit_pending;
static WNDCLASS registered_class;
static DWORD GetTickCount(void) { return now_tick; }
static unsigned GetWinFlags(void) { ++flags_calls;return win_flags; }
static unsigned char read_signature(void) { ++hardware_reads;return hardware_signature; }
static void record_port(unsigned char v) {
 if(port_count>=8192) { fprintf(stderr,"Port trace overflow\n");exit(90); }
 port_bytes[port_count]=v;port_ticks[port_count++]=now_tick;
}
static int SetTimer(HWND w,int id,int ms,void *callback) {
 (void)w;(void)id;(void)ms;(void)callback;
 ++timer_calls;if(timer_ok)live_timer=1;return timer_ok;
}
static int KillTimer(HWND w,int id) {
 (void)w;(void)id;++kill_calls;live_timer=0;return 1;
}
static int RegisterClass(WNDCLASS *c) {
 ++register_calls;registered_class=*c;return class_ok;
}
static int DestroyWindow(HWND w) {
 ++destroy_calls;
 if(window_alive) {
  window_alive=0;registered_class.lpfnWndProc(w,WM_DESTROY,0,0);
 }
 return 1;
}
static HWND CreateWindow(const char *cls,const char *name,DWORD style,
 int x,int y,int width,int height,HWND parent,void *menu,HINSTANCE inst,
 void *param) {
 LONG result;
 (void)cls;(void)name;(void)style;(void)x;(void)y;(void)width;
 (void)height;(void)parent;(void)menu;(void)inst;(void)param;
 ++create_calls;if(!window_ok)return NULL;
 window_alive=1;result=registered_class.lpfnWndProc(WIN,WM_CREATE,0,0);
 if(result==-1) { DestroyWindow(WIN);return NULL; }
 return WIN;
}
static void PostQuitMessage(int result) { (void)result;++quit_calls;quit_pending=1; }
static int GetMessage(MSG *msg,HWND w,unsigned lo,unsigned hi) {
 (void)w;(void)lo;(void)hi;++getmessage_calls;
 if(quit_pending) { memset(msg,0,sizeof(*msg));return 0; }
 if(queue_cursor>=queue_length) {
  fprintf(stderr,"Unexpected empty mock message queue\n");exit(91);
 }
 *msg=queue_messages[queue_cursor];now_tick=queue_ticks[queue_cursor++];return 1;
}
static int TranslateMessage(const MSG *msg) { (void)msg;++translate_calls;return 1; }
static LONG DispatchMessage(const MSG *msg) {
 ++dispatch_calls;
 return registered_class.lpfnWndProc(msg->hwnd,msg->message,msg->wParam,msg->lParam);
}
static LONG DefWindowProc(HWND w,UINT m,WPARAM p,LPARAM l) {
 (void)w;(void)m;(void)p;(void)l;return 0;
}
typedef int BOOL,HFILE;
typedef void *HMODULE;
typedef void *FARPROC;
#define HFILE_ERROR (-1)
#define MB_OKCANCEL 1
#define MB_DEFBUTTON2 0x100
#define MB_OK 0
#define IDOK 1
static int confirm_result,box_calls,exit_calls,exit_result,exit_clean,busy_kind,driver_loaded;
static char test_log[4096];
static HWND FindWindow(const char *name,void *title){
 (void)title;
 if((busy_kind==1&&!strcmp(name,"TandyPSGTest")) ||
    (busy_kind==2&&!strcmp(name,"TandyStartupChime")) ||
    (busy_kind==3&&!strcmp(name,"TandyXPStartupChime")) ||
    (busy_kind==4&&!strcmp(name,"TandyAutomaticMouth")))return WIN;
 return NULL;
}
static HMODULE GetModuleHandle(const char *s){(void)s;return driver_loaded?WIN:NULL;}
static FARPROC GetProcAddress(HMODULE h,const char *s){(void)h;(void)s;return driver_loaded?WIN:NULL;}
static int MessageBox(HWND w,const char *s,const char *t,unsigned flags){
 (void)w;(void)s;(void)t;++box_calls;return flags&MB_OKCANCEL?confirm_result:IDOK;
}
static int ExitWindows(DWORD code,unsigned reserved){
 (void)code;(void)reserved;++exit_calls;
 exit_clean=!live_timer&&!window_alive&&port_count>=4&&
  port_bytes[port_count-4]==0x9f&&port_bytes[port_count-3]==0xbf&&
  port_bytes[port_count-2]==0xdf&&port_bytes[port_count-1]==0xff;
 return exit_result;
}
static int PostMessage(HWND w,UINT m,WPARAM wp,LPARAM lp){
 MSG *q=&queue_messages[queue_length];q->hwnd=w;q->message=m;q->wParam=wp;q->lParam=lp;
 queue_ticks[queue_length++]=now_tick;return 1;
}
static HFILE _lopen(const char *p,int m){(void)p;(void)m;return 1;}
static HFILE _lcreat(const char *p,int m){(void)p;(void)m;return 1;}
static long _llseek(HFILE f,long p,int m){(void)f;(void)p;(void)m;return 0;}
static unsigned _lwrite(HFILE f,const char *s,unsigned n){(void)f;if(strlen(test_log)+n<4096)strncat(test_log,s,n);return n;}
static int _lclose(HFILE f){(void)f;return 0;}
static unsigned lstrlen(const char *s){return (unsigned)strlen(s);}
