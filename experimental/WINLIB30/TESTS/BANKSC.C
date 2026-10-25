/* Actual file-producer bank persistence; fake MD service and SMF input. */
#include <stdio.h>
#include <stdlib.h>
#include "mdhost/windows.h"
#include "../SOURCE/MCIPSG.C"
static WORD active,chosen,firstBank,failBank;static unsigned cursor,checks;
static WORD fakeOpen(WORD FAR *h){active=1;chosen=0;*h=7;return 0;}
static WORD fakePut(WORD h,DWORD gen,DWORD at,DWORD msg){(void)h;(void)gen;(void)at;(void)msg;if(cursor==1)firstBank=chosen;return 0;}
static WORD fakeCommit(WORD h,DWORD gen,DWORD at,WORD f){(void)h;(void)gen;(void)at;(void)f;return 0;}
static WORD fakeOne(WORD h){(void)h;return 0;}
static WORD fakeClose(WORD h){(void)h;active=0;return 0;}
static WORD fakeStatus(WORD h,MDINFO FAR *v,WORD n){(void)h;(void)n;memset(v,0,sizeof(*v));v->generation=1;return 0;}
static WORD fakeMode(WORD h,WORD v){(void)h;(void)v;return 0;}
static WORD fakeBank(WORD h,WORD v){if(!active||h!=7||failBank)return MD_BUSY;chosen=v;return 0;}
HTASK GetCurrentTask(void){return 1;}HMODULE GetModuleHandle(const char *n){(void)n;return 33;}
WORD GetModuleFileName(HMODULE m,char *p,WORD n){(void)m;(void)n;strcpy(p,"C:\\WINAMP\\MCITNDY.DLL");return strlen(p);}
HMODULE LoadLibrary(const char *n){(void)n;return 34;}WORD FreeLibrary(HMODULE m){(void)m;return 0;}
void *GetProcAddress(HMODULE m,LPSTR p){(void)m;switch((uintptr_t)p){case 1:return fakeOpen;case 2:return fakePut;case 3:return fakeCommit;case 4:case 5:case 6:case 7:return fakeOne;case 9:return fakeClose;case 10:return fakeStatus;case 13:return fakeMode;case 19:return fakeBank;}return 0;}
int smf_rewind(void){cursor=0;return 1;}
int smf_step(SMF_EVENT *v){if(cursor==2)return 0;v->ms=cursor*110;v->status=cursor?0x80:0x90;v->data1=60;v->data2=cursor?0:127;cursor++;return 1;}
int smf_next(SMF_EVENT *v){return smf_step(v);}
SMF_ULONG smf_duration_ms(void){return 220;}
static void ck(int ok,const char *name){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",name);exit(1);}}
int main(void){ck(!SCBank(1),"offline Chamber choice");ck(!SCStart()&&chosen==1&&firstBank==1,"bank before first file event");
 ck(!SCStop()&&!active,"stop releases provider");ck(!SCStart()&&chosen==1&&firstBank==1,"replay retains Chamber");
 ck(SCBank(99)!=0&&chosen==1,"invalid choice unchanged");failBank=1;ck(SCBank(0)!=0&&instrumentBank==1,"refused backend choice unchanged");failBank=0;
 ck(!SCBank(0)&&chosen==0,"change to Default");ck(!SCStop()&&!SCStart()&&chosen==0&&firstBank==0,"Default replay");
 ck(!SCStop()&&!active,"final provider cleanup");printf("PASS actual file bank checks=%u\n",checks);return 0;}
