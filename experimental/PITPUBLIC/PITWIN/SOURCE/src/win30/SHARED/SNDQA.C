/* Production shared owner with mocked Windows/SOUND; no hardware writes. */
#include <stdio.h>
#include <string.h>
#define FAR
#define PASCAL
typedef unsigned HMODULE;
typedef int HFILE;
typedef char *LPSTR;
#define HFILE_ERROR (-1)
#define wsprintf sprintf
#define lstrcmp strcmp
#define lstrlen strlen
static HFILE _lcreat(char *p,int n){return -1;}
static int _lwrite(int h,char *p,int n){return n;}
static int _lclose(int h){return 0;}
static int lease,capable=1,answer;
static unsigned opens,closes,stops,calls,discovers,lastHz,lastLevel;
static int OpenSound(void){opens++;if(lease)return -1;lease=1;return 3;}
static void CloseSound(void){closes++;if(lease==1)lease=0;}
static int StopSound(void){stops++;return lease==1?0:-1;}
typedef int (*TESTPROC)(unsigned,unsigned,unsigned);
static int fake(unsigned v,unsigned h,unsigned l)
{calls++;lastHz=h;lastLevel=l;return answer;}
static HMODULE GetModuleHandle(char *s){return 1;}
static TESTPROC GetProcAddress(HMODULE h,char *s)
{discovers++;return capable?fake:0;}
#include "SNDOWN.H"
#define CHECK(x) do {tests++;if(!(x)){printf("FAIL %u\n",__LINE__);return 1;}} while(0)
int main(void)
{
 unsigned tests=0,b,c;
 lease=17;
 CHECK(!SoundAcquire()&&!soundOwned&&lease==17&&!closes&&!calls);
 SoundRelease();CHECK(lease==17&&!closes&&!stops&&!calls);
 lease=0;
 CHECK(SoundAcquire()&&soundOwned&&lease==1&&!discovers);
 b=opens;CHECK(SoundAcquire()&&opens==b);
 CHECK(SoundPitNote(60)&&!calls);
 SoundRelease();CHECK(!soundOwned&&!lease&&closes==1&&stops==1&&!calls);
 SoundRelease();CHECK(closes==1&&stops==1);
 pitOption=1;capable=0;
 CHECK(!SoundAcquire()&&!soundOwned&&!lease&&closes==2&&!calls);
 capable=1;
 CHECK(SoundAcquire()&&soundOwned&&lease==1);
 CHECK(SoundPitNote(60)&&soundPit.note==60&&lastHz==262&&lastLevel==1);
 b=calls;CHECK(SoundPitNote(60)&&calls==b);
 CHECK(!SoundPitNote(44)&&!soundPit.note&&calls==b);
 SoundRelease();CHECK(!soundOwned&&!soundPit.note&&!lease&&lastHz==0&&lastLevel==0);
 CHECK(SoundAcquire());answer=-3;
 CHECK(!SoundPitNote(64)&&!soundPit.note);
 b=closes;c=stops;SoundRelease();
 CHECK(!soundOwned&&!lease&&closes==b+1&&stops==c+1);
 answer=0;CHECK(SoundAcquire()&&SoundPitNote(96)&&lastHz==2093);
 SoundRelease();CHECK(!soundOwned&&!lease&&!soundPit.note);
 b=closes;c=calls;lease=17;
 CHECK(!SoundAcquire()&&closes==b&&calls==c&&lease==17);
 SoundRelease();CHECK(lease==17&&closes==b&&calls==c);
 printf("PASS %u owner/refusal/capability/cleanup checks\n",tests);return 0;
}
