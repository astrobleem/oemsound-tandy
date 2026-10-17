#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define FAR
#define PASCAL
#define MD_FULL 2
#define MD_PROGRAM 0xc0
typedef char *LPSTR;typedef uint16_t WORD;typedef uint8_t BYTE;typedef uint32_t DWORD;typedef int32_t LONG;
static int playing,failures,stepsPlayed,opened=1,timerReady=1,step;static WORD tempo=120,module=1;
static BYTE channels[4]={0,1,2,9},pattern[4][8]={{60,62,64,65,67,69,71,72},{0},{0},{42,42,42,42,42,42,42,42}};static char *status;
static WORD fakeMask;static WORD fakeDebug(WORD h,WORD q){return fakeMask;}static WORD(*acDebugFn)(WORD,WORD)=fakeDebug;
static WORD acHandle,acStarted,acSealed;static DWORD acGen=1,acClockValue,acWatermark,fakeNow;
static struct {DWORD generation;}acInfo;
static DWORD seedAt[16],seedMsg[16],msgAt[512],msg[512];static unsigned seeds,count;
static void redraw(void){}
static WORD ACOpen(void){acHandle=1;return 0;}static WORD ACClose(void){acHandle=0;return 0;}static WORD ACClock(void){acClockValue=fakeNow;return 0;}
static WORD resetFn(WORD h,DWORD at){count=seeds=0;acInfo.generation=++acGen;return 0;}
static WORD statFn(WORD h,void *p,WORD n){return 0;}
static WORD (*acResetFn)(WORD,DWORD)=resetFn;static WORD (*acStatusFn)(WORD,void*,WORD)=statFn;
static WORD seedFn(WORD h,DWORD g,DWORD at,DWORD v){seedAt[seeds]=at;seedMsg[seeds++]=v;return 0;}
static void *GetProcAddress(WORD m,char *n){return seedFn;}
static WORD ACPush(DWORD at,DWORD v){if(count==512)abort();msgAt[count]=at;msg[count++]=v;return 0;}
static WORD ACPublish(DWORD through,int last){acWatermark=through;return 0;}
#include "../SOURCE/BEATSEQ.H"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d\n",__LINE__);return 1;}}while(0)
int main(void){unsigned i,n;DWORD oldAt[512],oldMsg[512],gen;
 play();CHECK(playing&&count>10);CHECK(msgAt[0]==0&&msg[0]==0xc0);CHECK(msgAt[1]==0);CHECK(msgAt[3]==187);CHECK(msgAt[5]==250);
 n=count;for(i=0;i<n;i++){oldAt[i]=msgAt[i];oldMsg[i]=msg[i];}gen=acGen;fakeNow=125;pattern[0][0]=80;tempo=150;bsEdit();
 CHECK(acGen==gen&&bsEditAt>=1000&&bsDur==200);for(i=0;i<n;i++)CHECK(oldAt[i]==msgAt[i]&&oldMsg[i]==msg[i]);
 fakeNow=900;CHECK(bsPump());for(i=0;i<n;i++)CHECK(oldAt[i]==msgAt[i]&&oldMsg[i]==msg[i]);CHECK(bsMarks[bsHead].duration==250);CHECK(bsMarks[(bsHead+bsMarkCount-1)&31].duration==200);{int found=0;for(i=n;i<count;i++)if((msg[i]&255)==0x90&&((msg[i]>>8)&127)==80&&msgAt[i]>=bsEditAt)found=1;CHECK(found);}
 for(i=0;i<8;i++)pattern[0][i]=pattern[3][i]=0;fakeNow=0;count=0;play();CHECK(playing&&count==1&&msg[0]==0xc0);CHECK(bsMarkCount>0);puts("PASS actual BEATSEQ: onset/gate, immutable prepared edits, queued tempo, silent pattern");return 0;}
