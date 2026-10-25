/* Foreground app binding to the shared deadline MIDI service. No direct IO. */
#include "MDAPI.H"
typedef WORD(FAR PASCAL *ACOPEN)(WORD FAR *);
typedef WORD(FAR PASCAL *ACPUSH)(WORD,DWORD,DWORD,DWORD);
typedef WORD(FAR PASCAL *ACCOMMIT)(WORD,DWORD,DWORD,WORD);
typedef WORD(FAR PASCAL *ACONE)(WORD);
typedef WORD(FAR PASCAL *ACRESET)(WORD,DWORD);
typedef WORD(FAR PASCAL *ACSTATUS)(WORD,MDINFO FAR *,WORD);
typedef DWORD(FAR PASCAL *ACVER)(void);
typedef WORD(FAR PASCAL *ACMODE)(WORD,WORD);
typedef WORD(FAR PASCAL *ACCLOCK)(WORD,DWORD FAR *);
static WORD acLastError,acLastRaw;static int acBound;static WORD acHandle,acStarted,acSealed,acBankSelected;static DWORD acGen,acWatermark,acClockValue;static MDINFO acInfo;
static ACOPEN acOpenFn;static ACPUSH acPutFn;static ACCOMMIT acCommitFn;static ACONE acStartFn,acPumpFn,acCloseFn;static ACRESET acResetFn;static ACSTATUS acStatusFn;static ACMODE acTimingFn,acVoiceFn,acDebugFn,acBankFn;static ACCLOCK acClockFn;
static int ACBind(HINSTANCE mod){ACVER v=(ACVER)GetProcAddress(mod,(LPSTR)14L);if(!v||v()!=0x00080100UL)return 0;
 acOpenFn=(ACOPEN)GetProcAddress(mod,(LPSTR)1L);acPutFn=(ACPUSH)GetProcAddress(mod,(LPSTR)2L);acCommitFn=(ACCOMMIT)GetProcAddress(mod,(LPSTR)3L);acStartFn=(ACONE)GetProcAddress(mod,(LPSTR)4L);acPumpFn=(ACONE)GetProcAddress(mod,(LPSTR)5L);acResetFn=(ACRESET)GetProcAddress(mod,(LPSTR)8L);acCloseFn=(ACONE)GetProcAddress(mod,(LPSTR)9L);acStatusFn=(ACSTATUS)GetProcAddress(mod,(LPSTR)10L);acTimingFn=(ACMODE)GetProcAddress(mod,(LPSTR)13L);acClockFn=(ACCLOCK)GetProcAddress(mod,(LPSTR)15L);acVoiceFn=(ACMODE)GetProcAddress(mod,(LPSTR)16L);acDebugFn=(ACMODE)GetProcAddress(mod,(LPSTR)17L);acBankFn=(ACMODE)GetProcAddress(mod,(LPSTR)19L);
 acBound=acOpenFn&&acPutFn&&acCommitFn&&acStartFn&&acPumpFn&&acResetFn&&acCloseFn&&acStatusFn&&acTimingFn&&acClockFn&&acVoiceFn&&acDebugFn&&acBankFn;return acBound;}
static WORD ACBank(WORD bank){if(bank>MD_BANK_CHAMBER)return 1;
 if(acHandle&&acBankFn(acHandle,bank))return 1;acBankSelected=bank;return 0;}
static WORD ACReset(void){if(!acHandle)return 0;if(acResetFn(acHandle,0)||acBankFn(acHandle,acBankSelected)||acStatusFn(acHandle,&acInfo,sizeof(acInfo)))return 1;acGen=acInfo.generation;acStarted=acSealed=0;acWatermark=0;return 0;}
static WORD ACClose(void){if(!acHandle)return 0;if(ACReset()||acCloseFn(acHandle))return 1;acHandle=0;return 0;}
static WORD ACOpen(void){if(!acBound||acHandle)return 1;if(acOpenFn(&acHandle))return 1;if(acTimingFn(acHandle,2)||acVoiceFn(acHandle,0)||acBankFn(acHandle,acBankSelected)||acStatusFn(acHandle,&acInfo,sizeof(acInfo))){ACClose();return 1;}acGen=acInfo.generation;acStarted=acSealed=0;acWatermark=0;return 0;}
static WORD ACPush(DWORD at,DWORD msg){return acPutFn(acHandle,acGen,at,msg);}
static WORD ACPublish(DWORD through,int last){if(acSealed)return 0;if((LONG)(through-acWatermark)<0)return 0;
 {WORD r=acCommitFn(acHandle,acGen,through,last?1:0);if(r){acLastError=100+r;acLastRaw=acDebugFn(acHandle,3);return 1;}}acWatermark=through;if(last)acSealed=1;
 if(!acStarted){{WORD r=acStartFn(acHandle);if(r){acLastError=200+r;acLastRaw=acDebugFn(acHandle,3);return 1;}}acStarted=1;}{WORD r=acPumpFn(acHandle);if(r){acLastError=300+r;acLastRaw=acDebugFn(acHandle,3);}return r;}}
static WORD ACClock(void){if(!acHandle||acStatusFn(acHandle,&acInfo,sizeof(acInfo))||acInfo.state==MD_FAILED||acClockFn(acHandle,&acClockValue))return 1;return 0;}
static DWORD ACPending(void){return acInfo.accepted-acInfo.delivered;}
static WORD FAR PASCAL ACDebug(WORD q){if(q==1)return acHandle!=0;if(!acHandle)return 0;return acDebugFn(acHandle,q);}
