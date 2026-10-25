#define WINVER 0x0300
#include <windows.h>
#include "LTWUI.H"
/* Synchronous caller-owned batches only. Never span a return to USER. */
void LtwListBegin(LTWLISTPAINT *state,HWND list) {
 if(!state->depth++){
  state->list=list;
  if(list)SendMessage(list,WM_SETREDRAW,FALSE,0L);
 }
}
void LtwListEnd(LTWLISTPAINT *state,HWND currentList) {
 HWND list;
 if(!state->depth||--state->depth)return;
 list=state->list;state->list=NULL;
 if(list&&list==currentList&&IsWindow(list)){
  SendMessage(list,WM_SETREDRAW,TRUE,0L);
  InvalidateRect(list,NULL,TRUE);
 }
}
/* Query USER each time: no retained text/background cache can become stale. */
BOOL LtwSetText(HWND control,LPCSTR text) {
 char current[144];int n;
 if(!control)return FALSE;
 n=GetWindowTextLength(control);
 if(n>=0&&n<sizeof(current)){
  GetWindowText(control,current,sizeof(current));
  if(!lstrcmp(current,text))return FALSE;
 }
 SetWindowText(control,text);return TRUE;
}
