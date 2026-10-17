/* SMF foreground producer for the shared MIDICLK timestamp transport. */
#define WINVER 0x0300
#include <windows.h>
#include "SMF.H"
#include "MDAPI.H"
#include "MCIPSG.H"
typedef WORD(FAR PASCAL *OPEN)(WORD FAR *);typedef WORD(FAR PASCAL *PUSH)(WORD,DWORD,DWORD,DWORD);typedef WORD(FAR PASCAL *COMMIT)(WORD,DWORD,DWORD,WORD);typedef WORD(FAR PASCAL *ONE)(WORD);typedef WORD(FAR PASCAL *STATUS)(WORD,MDINFO FAR *,WORD);typedef WORD(FAR PASCAL *MODE)(WORD,WORD);
static HMODULE dll;static WORD handle,have,eof,ended,lastError,pumps;static DWORD committed,lastAccepted;static SMF_EVENT e;static MDINFO info;static char path[144];static OPEN op;static PUSH put;static COMMIT commit;static ONE start,pump,pauseClock,resumeClock,closeClock;static STATUS status;static MODE timing;
static WORD feed(WORD budget){int r;WORD result;DWORD through;
 while(budget--&&!eof){if(!have){r=smf_step(&e);if(r<0)return 1;if(r==2)continue;if(!r){eof=1;break;}have=1;}
  result=put(handle,info.generation,e.ms,(DWORD)e.status|((DWORD)e.data1<<8)|((DWORD)e.data2<<16));if(result==MD_FULL)break;if(result){lastError=result;return 1;}lastAccepted=e.ms;have=0;}
 if(ended)return 0;
 if(eof){result=commit(handle,info.generation,smf_duration_ms(),MD_EOF);if(result)return 1;ended=1;return 0;}
 /* A peeked event seals every earlier timestamp, but not a partially
    submitted same-time chord. No timestamp receives a guessed watermark. */
 if(!have){r=smf_next(&e);if(r<0)return 1;if(!r){eof=1;return feed(0);}have=1;}
 if(e.ms){through=e.ms-1;if(through>=committed){result=commit(handle,info.generation,through,0);if(result)return 1;committed=through;}}
 return 0;}
WORD SCStop(void){if(dll){if(handle&&closeClock(handle))return 1;handle=0;FreeLibrary(dll);dll=0;}return 0;}
WORD SCStart(void){WORD n,i,last=0;if(SCStop())return 1;n=GetModuleFileName(GetModuleHandle("MCITNDY"),path,sizeof(path));if(!n||n>=sizeof(path)-13)return 1;for(i=0;i<n;i++)if(path[i]=='\\'||path[i]=='/')last=i+1;lstrcpy(path+last,"MIDICLK.DLL");dll=LoadLibrary(path);if(dll<32){dll=0;return 1;}
 op=(OPEN)GetProcAddress(dll,(LPSTR)1L);put=(PUSH)GetProcAddress(dll,(LPSTR)2L);commit=(COMMIT)GetProcAddress(dll,(LPSTR)3L);start=(ONE)GetProcAddress(dll,(LPSTR)4L);pump=(ONE)GetProcAddress(dll,(LPSTR)5L);pauseClock=(ONE)GetProcAddress(dll,(LPSTR)6L);resumeClock=(ONE)GetProcAddress(dll,(LPSTR)7L);closeClock=(ONE)GetProcAddress(dll,(LPSTR)9L);status=(STATUS)GetProcAddress(dll,(LPSTR)10L);timing=(MODE)GetProcAddress(dll,(LPSTR)13L);
 if(!(op&&put&&commit&&start&&pump&&pauseClock&&resumeClock&&closeClock&&status&&timing)){FreeLibrary(dll);dll=0;return 1;}
 if(op(&handle)){handle=0;SCStop();return 1;}status(handle,&info,sizeof(info));timing(handle,1);have=eof=ended=lastError=pumps=0;committed=lastAccepted=0;
 if(!smf_rewind()||feed(4096)||start(handle)){SCStop();return 1;}return 0;}
WORD SCPump(void){WORD r;if(!handle)return 0;r=pump(handle);status(handle,&info,sizeof(info));pumps++;if(r||info.state==MD_FAILED)return 2;if(info.state==MD_DONE)return 0;if(feed(256))return 2;return 1;}
DWORD SCPosition(void){if(handle)status(handle,&info,sizeof(info));return info.position;}
WORD SCPause(void){return handle?pauseClock(handle):1;}
WORD SCResume(void){return handle?resumeClock(handle):1;}
WORD SCDebug(WORD q){if(handle)status(handle,&info,sizeof(info));switch(q){case 0:return info.state==MD_PAUSED;case 1:return pumps;case 2:return (WORD)info.coalesced;case 3:return lastError?lastError:info.error;case 4:return handle!=0;case 6:return (WORD)info.delivered;case 7:return info.queued;}return 0;}
