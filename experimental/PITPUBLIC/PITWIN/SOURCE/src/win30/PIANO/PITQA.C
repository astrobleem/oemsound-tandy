/* Native DOS mocked client checks; no port or installed-driver I/O. */
#include <stdio.h>
#include <string.h>
#define FAR _far
#define PASCAL _pascal
typedef unsigned HMODULE;
typedef int (FAR PASCAL *TESTPROC)(unsigned,unsigned,unsigned);
static int present=1,answer;
static unsigned calls,version,hz,level,discoveries;
static int FAR PASCAL fake(unsigned v,unsigned h,unsigned l)
{
    calls++;version=v;hz=h;level=l;return answer;
}
static HMODULE GetModuleHandle(char *name)
{
    return !strcmp(name,"SOUND")?1:0;
}
static TESTPROC GetProcAddress(HMODULE module,char *name)
{
    discoveries++;
    return module && present && !strcmp(name,"TANDYPITVOICE")?fake:0;
}
#include "PITCLNT.H"
#define CHECK(x) do {tests++;if(!(x)){printf("FAIL line %u\n",    (unsigned)__LINE__);return 1;}} while(0)
int main(void)
{
    PIT_CLIENT p;unsigned tests=0,i,before;
    p.proc=0;p.note=0;present=0;
    CHECK(PitClientDiscover(&p)==PITCLIENT_UNAVAILABLE&&!p.proc&&!calls);
    present=1;
    CHECK(PitClientDiscover(&p)==0&&p.proc&&!calls);
    p.note=60;
    CHECK(PitClientNote(&p,0,64)==PITCLIENT_NOT_OWNED&&!p.note&&!calls);
    CHECK(PitClientNote(&p,1,45)==0&&p.note==45&&calls==1
        &&version==1&&hz==110&&level==1);
    before=calls;
    CHECK(PitClientNote(&p,1,45)==0&&calls==before);
    before=discoveries;
    CHECK(PitClientDiscover(&p)==PITCLIENT_NOT_OWNED
        &&discoveries==before&&p.note==45);
    CHECK(PitClientNote(&p,1,96)==0&&p.note==96&&hz==2093);
    before=calls;
    CHECK(PitClientNote(&p,1,44)==PITCLIENT_BAD_NOTE
        &&!p.note&&calls==before);
    CHECK(PitClientOff(&p,1)==0&&!p.note&&hz==0&&level==0);
    before=calls;
    CHECK(PitClientNote(&p,1,97)==PITCLIENT_BAD_NOTE&&calls==before);
    answer=-3;
    CHECK(PitClientNote(&p,1,60)==-3&&!p.note);
    answer=0;
    CHECK(PitClientOff(&p,1)==0&&!p.note);
    CHECK(PitClientNote(&p,1,60)==0&&p.note==60);
    before=calls;
    CHECK(PitClientOff(&p,0)==PITCLIENT_NOT_OWNED
        &&!p.note&&calls==before);
    CHECK(PitClientNote(&p,1,60)==0&&calls==before+1);
    before=calls;PitClientForget(&p);
    CHECK(!p.note&&calls==before);
    CHECK(PitClientNote(&p,1,60)==0&&calls==before+1);
    CHECK(PitClientOff(&p,1)==0&&!p.note);
    p.proc=0;
    CHECK(PitClientNote(&p,1,60)==PITCLIENT_UNAVAILABLE&&!p.note);
    CHECK(PitClientOff(&p,1)==PITCLIENT_UNAVAILABLE&&!p.note);
    for(i=1;i<52;i++)CHECK(pitClientHz[i]>pitClientHz[i-1]);
    printf("PASS %u client discovery/cache/lease/bounds checks\n",tests);
    return 0;
}
