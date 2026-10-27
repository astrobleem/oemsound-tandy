/* Actual compiler/export adapter vs actual libtandy and raw PSG shadow. */
#include "FXCORE.H"
#include "lt_audio.h"
#include "FXPLAY.H"
#include <string.h>
#include <stdlib.h>
typedef struct {unsigned reg[8],latch,noiseWrites;} SHADOW;
static unsigned checks,states,pitValue;
static FXCLIP c;static FXP_ROW rows[FX_TICKS+1];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void emit(void *ctx,lt_u8 b){SHADOW *s=(SHADOW *)ctx;unsigned r;
 if(b&128){r=(b>>4)&7;s->latch=r;if(r==6)s->noiseWrites++;
  if(r==0||r==2||r==4)s->reg[r]=(s->reg[r]&1008)|(b&15);else s->reg[r]=b&15;}
 else {r=s->latch;if(r==0||r==2||r==4)s->reg[r]=(s->reg[r]&15)|((b&63)<<4);}}
static int pit(void *ctx,unsigned divisor){(void)ctx;pitValue=divisor;return 0;}
static void trajectory(FXPATCH *p){unsigned i,j,n,cursor=0;unsigned char b[16];SHADOW raw,lib;FXP_SOUND s;lt_chip chip;lta_handle audio;lta_request q;
 CHECK(FXCompile(p,&c));CHECK(c.count>=2&&c.count<=FX_TICKS+1);memset(&raw,0,sizeof(raw));memset(&lib,0,sizeof(lib));
 for(i=0;i<4;i++)raw.reg[i*2+1]=lib.reg[i*2+1]=15;
 lt_init(&chip,emit,&lib);CHECK(lta_init(&audio,&chip,1)==LTA_OK);lta_request_init(&q);
 if(!c.target){q.op=LTA_OP_ACQUIRE;q.mask=(lt_u8)c.mask;CHECK(lta_command(&audio,&q)==LTA_OK);}
 for(i=0;i<c.count;i++){rows[i].period=c.row[i].period;rows[i].level=c.row[i].level;rows[i].noise=c.row[i].noise;rows[i].pit=c.row[i].pit;}
 s.count=c.count;s.target=c.target;s.mask=c.mask;s.rows=rows;
 for(i=0;i<c.count;i++){
  n=FXBytes(&c,i,b);CHECK(n<=16);for(j=0;j<n;j++)emit(&raw,b[j]);
  CHECK(FXPTick(&s,&cursor,&audio,pit,0)==(i+1<c.count?1:0));
  if(c.target){CHECK(pitValue==c.row[i].pit);CHECK(!pitValue||pitValue>=151);}
  else {for(j=0;j<8;j++)if((j==0&&c.mask==1)||(j==4&&c.mask==12)||(j==6&&c.mask!=1)||(j%2&&c.mask&(1U<<(j/2))))CHECK(raw.reg[j]==lib.reg[j]);
   CHECK(raw.noiseWrites==lib.noiseWrites);CHECK(c.row[i].period>=14&&c.row[i].period<=1023);CHECK(c.row[i].level<=15);}
  states++;
 }
 CHECK(c.row[c.count-1].level==15&&c.row[c.count-1].pit==0);
 CHECK(FXPStop(&s,&audio,pit,0)==0);
 if(c.target)CHECK(pitValue==0);else{q.op=LTA_OP_RELEASE;CHECK(lta_command(&audio,&q)==LTA_OK);}
}
int main(int argc,char **argv){FXPATCH p,a,b,saved;unsigned i,j,n;char text[FX_TEXT],path[240];FILE *f;
 CHECK(argc==2);CHECK(sizeof(FXPATCH)<64&&sizeof(FXCLIP)<5000);
 for(i=0;i<5;i++)for(j=0;j<2;j++){FXPreset(&p,i);p.v[FX_TARGET]=(int)j;trajectory(&p);}
 FXPreset(&p,1);p.v[FX_HOLD]=80;p.v[FX_RELEASE]=30;CHECK(FXCompile(&p,&c));CHECK(c.row[1].period==51&&c.row[38].period==1017);
 for(i=0;i<9;i++){FXPreset(&p,2);p.v[FX_NOISE]=(int)i;p.v[FX_ATTACK]=99;p.v[FX_HOLD]=100;p.v[FX_RELEASE]=99;trajectory(&p);}
 FXPreset(&p,0);p.v[FX_TARGET]=1;p.v[FX_PITCH]=19;trajectory(&p);CHECK(c.row[0].pit==62799);
 p.v[FX_PITCH]=7900;trajectory(&p);CHECK(c.row[0].pit==151);
 FXPreset(&a,0);b=a;
 for(i=0;i<500;i++){a.v[FX_TARGET]=b.v[FX_TARGET]=(int)(i&1);FXRandom(&a,i&1);FXRandom(&b,i&1);CHECK(!memcmp(&a,&b,sizeof(a)));CHECK(FXValid(&a));trajectory(&a);
  n=FXEncode(&a,text,sizeof(text));CHECK(n&&FXDecode(&p,text,n));CHECK(!memcmp(&p,&a,sizeof(p)));}
 saved=p;strcpy(text,"TFX1\r\n65536\r\n440 0 0 0 0 10 10 0 0 100 0 0\n");CHECK(!FXDecode(&p,text,(unsigned)strlen(text)));CHECK(!memcmp(&p,&saved,sizeof(p)));
 strcpy(text,"TFX1\r\n1\r\n-65000 0 0 0 0 10 10 0 0 100 0 0\n");CHECK(!FXDecode(&p,text,(unsigned)strlen(text)));
 CHECK(FXName("MYFX.FXP")&&!FXName("CON.FXP")&&!FXName("../A.FXP"));
 sprintf(path,"%s/TEST.FXP",argv[1]);FXPreset(&p,0);CHECK(FXSave(&p,path,0)==0);CHECK(FXSave(&p,path,0)==3);p.seed=65535;CHECK(FXSave(&p,path,1)==0);CHECK(FXLoad(&a,path)==0&&!memcmp(&a,&p,sizeof(p)));
 for(i=0;i<5;i++){FXPreset(&p,i);CHECK(FXCompile(&p,&c));sprintf(path,"%s/P%u.FXP",argv[1],i);CHECK(FXSave(&p,path,0)==0);}
 FXPreset(&p,2);CHECK(FXCompile(&p,&c));sprintf(path,"%s/GENERATED.H",argv[1]);f=fopen(path,"wb");CHECK(f&&FXHeader(&c,f));CHECK(fclose(f)==0);
 printf("PASS checks=%u trajectory_states=%u patch_bytes=%u clip_bytes=%u\n",checks,states,(unsigned)sizeof(p),(unsigned)sizeof(c));return 0;}
