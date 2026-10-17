#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "MIDISYN.H"
static unsigned checks;static int failures;static void ck(int ok,const char *what){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",what);failures++;}}
static DWORD msg(unsigned s,unsigned n,unsigned v){return s|((DWORD)n<<8)|((DWORD)v<<16);}
int main(void){MSYN s,copy;unsigned i;DWORD ms;MSReset(&s);for(i=0;i<4;i++)ck(s.volume[i]==15,"reset mute");s.flags=0;ck(!MSMessage(&s,0,msg(0x90,60,127)),"noteon");ck(s.note[0]==60&&s.volume[0]==0&&s.p[0]>0,"tone state");MSRender(&s,100000);ck(s.note[0]==60&&s.volume[0]==0,"flat sustains");ck(!MSMessage(&s,100000,msg(0x80,60,0))&&s.flags==0&&s.volume[0]==15,"flat preserved off");ck(!MSMessage(&s,100100,msg(0x90,60,127))&&!MSMessage(&s,100100,msg(0x91,60,127)),"channels");ck(s.note[0]==60&&s.note[1]==60,"channel separation");ck(!MSMessage(&s,100200,msg(0xb0,120,0))&&s.note[0]==0&&s.note[1]==60&&s.flags==0,"CC scoped and flat preserved");copy=s;ck(MSMessage(&s,100200,0x01003c90)==1&&!memcmp(&s,&copy,sizeof(s)),"invalid no mutation");
 MSReset(&s);s.flags=0;for(i=0;i<4;i++)MSMessage(&s,0,msg(0x90,45+i*3,127));ck(s.note[0]==54&&s.note[1]==48&&s.note[2]==51,"oldest steal");s.counter=65535;MSMessage(&s,10,msg(0x90,60,127));ck(s.counter==4&&s.note[1]==60,"age wrap ranking");
 MSReset(&s);MSMessage(&s,0,msg(0x99,42,127));ck(s.noiseNote==42&&s.noise==4&&s.volume[3]==2,"hat");MSRender(&s,165);ck(s.noiseNote==0&&s.volume[3]==15,"hat decay");
 MSReset(&s);MSMessage(&s,0,msg(0xc0,16,0));MSMessage(&s,(DWORD)-100,msg(0x90,60,127));MSRender(&s,0);ck(s.note[0]==60&&s.volume[0]==0,"historical seed organ");
 MSReset(&s);MSMessage(&s,(DWORD)-100,msg(0x90,60,127));MSRender(&s,0);ck(s.volume[0]==1,"historical seed keys32bit age");
 for(i=0;i<10000;i++){ms=(DWORD)(((uint64_t)i*3600000)/9999);ck(MSCycles(ms)==(DWORD)(((uint64_t)ms*3579545)/3000),"rational cycles");}
 {DWORD edges[]={0,1,999,1000,1999,2000,2999,3000,3600000,0x7fffffff,0xffffffff};for(i=0;i<sizeof(edges)/sizeof(*edges);i++)ck(MSCycles(edges[i])==(DWORD)(((uint64_t)edges[i]*3579545)/3000),"cycle edge");}
 printf("%s pure renderer checks=%u failures=%d\n",failures?"FAIL":"PASS",checks,failures);return failures?1:0;}
