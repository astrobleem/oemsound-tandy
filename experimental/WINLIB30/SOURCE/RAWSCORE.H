#ifndef RAWSCORE_H
#define RAWSCORE_H
/* Private foreground adapter; RAWCLK remains the only SOUND/PIT/PSG owner. */
#include "TIMECLK.H"
typedef WORD(FAR PASCAL *RAZERO)(void);
typedef WORD(FAR PASCAL *RAPUT)(TCFRAME FAR *);
typedef WORD(FAR PASCAL *RADEB)(WORD);
typedef DWORD(FAR PASCAL *RACLOCK)(void);
static HINSTANCE raModule;static RAZERO raOpenFn,raStartFn,raEndFn,raCloseFn,raFlushFn;
static RAPUT raPutFn;static RADEB raDebugFn;static RACLOCK raClockFn;
static int raOwned,raMode,raFailures,raChecks,raCompleted,raStopped,raClosed;
static WORD raPrepared,raDelivered,raServiceError;static char raLog[32];
static DWORD raLastAt;
static DWORD RACycles(DWORD ms){DWORD sec=ms/1000UL;return sec*1193181UL+(sec*2UL)/3UL+((ms%1000UL)*3579545UL+((sec*2UL)%3UL)*1000UL)/3000UL;}
static DWORD RAMilliseconds(void){DWORD x;if(!raOwned)return 0;x=raClockFn();return (x/1193181UL)*1000UL+((x%1193181UL)*1000UL)/1193181UL;}
static void RACheck(int okay){raChecks++;if(!okay)raFailures++;}
static int RAInit(HINSTANCE inst,LPSTR cmd,char *name){char path[144];int j;
 raMode=!lstrcmp(cmd,"/test")?1:!lstrcmp(cmd,"/stoptest")?2:!lstrcmp(cmd,"/closetest")?3:!lstrcmp(cmd,"/busytest")?4:0;
 lstrcpy(raLog,"C:\\");lstrcat(raLog,name);lstrcat(raLog,".LOG");
 GetModuleFileName(inst,path,sizeof(path));for(j=lstrlen(path)-1;j>=0&&path[j]!='\\';j--);lstrcpy(path+j+1,"RAWCLK.DLL");
 raModule=LoadLibrary(path);if(raModule<32)return 0;
 raOpenFn=(RAZERO)GetProcAddress(raModule,(LPSTR)13L);raPutFn=(RAPUT)GetProcAddress(raModule,(LPSTR)2L);
 raStartFn=(RAZERO)GetProcAddress(raModule,(LPSTR)3L);raEndFn=(RAZERO)GetProcAddress(raModule,(LPSTR)4L);raCloseFn=(RAZERO)GetProcAddress(raModule,(LPSTR)5L);
 raDebugFn=(RADEB)GetProcAddress(raModule,(LPSTR)6L);raClockFn=(RACLOCK)GetProcAddress(raModule,(LPSTR)12L);raFlushFn=(RAZERO)GetProcAddress(raModule,(LPSTR)16L);
 return raOpenFn&&raPutFn&&raStartFn&&raEndFn&&raCloseFn&&raDebugFn&&raClockFn&&raFlushFn&&GetProcAddress(raModule,(LPSTR)18L);
}
static int RAOpen(void){WORD r;if(raOwned)return 0;r=raOpenFn();if(r)return 0;raOwned=1;raLastAt=0;return 1;}
static int RAFrame(DWORD ms,WORD p0,WORD p1,WORD p2,WORD volume,WORD noise,WORD mask){TCFRAME f;WORD r;
 if(!raOwned)return 0;f.at=RACycles(ms);f.p[0]=p0;f.p[1]=p1;f.p[2]=p2;f.volume=volume;f.noise=noise;f.mask=mask;
 if(raDebugFn(0)&&(LONG)(ms-raLastAt)<8L){raFailures++;return 0;}
 r=raPutFn(&f);if(r){raFailures++;return 0;}raLastAt=ms;raPrepared++;return 1;}
static int RAStart(DWORD end){DWORD held;if(!RAFrame(end,1,1,1,65535U,0,15)||raEndFn()||raStartFn()){raFailures++;return 0;}if(raMode==1){held=GetTickCount();while(GetTickCount()-held<900UL);}return 1;}
static int RAService(void){if(!raOwned)return 0;raServiceError|=raDebugFn(6);if(raServiceError){raFailures++;return -1;}if(raDebugFn(7))return 1;return 0;}
static int RAClose(void){WORD sent;if(!raOwned)return 1;sent=raDebugFn(2);raServiceError|=raDebugFn(6);
 if(raDebugFn(7))raCompleted++;else raStopped++;
 if(raFlushFn()||raCloseFn()){raFailures++;return 0;}raDelivered+=sent;raOwned=0;raClosed++;return 1;}
static void RAReport(void){HFILE f;char line[180];if(raOwned){raFailures++;return;}if(raModule>=32){FreeLibrary(raModule);raModule=0;}
 if(!raMode)return;RACheck(raServiceError==0);RACheck(!raOwned);RACheck(GetModuleHandle("RAWCLK")==0);
 if(raMode==1)RACheck(raCompleted>0&&raPrepared==raDelivered);
 if(raMode==2||raMode==3)RACheck(raStopped>0);
 f=_lcreat(raLog,0);if(f>=0){wsprintf(line,"METRIC prepared=%u delivered=%u completed=%d stopped=%d closed=%d raw_error=%u\r\nFINAL failures=%d checks=%d\r\n",raPrepared,raDelivered,raCompleted,raStopped,raClosed,raServiceError,raFailures,raChecks);_lwrite(f,line,lstrlen(line));_lclose(f);}}

#endif
