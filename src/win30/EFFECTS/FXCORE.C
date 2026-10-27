/* SPDX-License-Identifier: MIT. Bounded C89 trajectory compiler, no hardware. */
#include "FXCORE.H"
#include <string.h>
#include <ctype.h>
const char *FXNames[FX_FIELDS]={"Pitch Hz","Slide Hz/s","Vib Hz","Vib rate","Attack tk","Hold tk","Release tk","Atten 2dB","Noise","Gate on tk","Gate off tk","Target"};
const char *FXNoiseNames[9]={"Tone","White hi","White mid","White low","White pitch","Period hi","Period mid","Period low","Period pitch"};
static const int lo[FX_FIELDS]={19,-7800,0,0,0,1,0,0,0,1,0,0};
static const int hi[FX_FIELDS]={7900,7800,2000,20,99,100,99,15,8,100,100,1};
static const int step[FX_FIELDS]={10,100,10,1,1,1,1,1,1,1,1,1};
static unsigned next(unsigned *s){*s=(unsigned)((*s*25173UL+13849UL)&65535UL);return *s;}
int FXValid(const FXPATCH *p){unsigned i;if(!p||p->seed>65535U)return 0;
 for(i=0;i<FX_FIELDS;i++)if(p->v[i]<lo[i]||p->v[i]>hi[i])return 0;return 1;}
int FXEnabled(const FXPATCH *p,unsigned f){if(f>=FX_FIELDS)return 0;
 if(p->v[FX_TARGET]&&(f==FX_ATTACK||f==FX_RELEASE||f==FX_LEVEL||f==FX_NOISE))return 0;
 if(!p->v[FX_TARGET]&&p->v[FX_NOISE]&&p->v[FX_NOISE]!=4&&p->v[FX_NOISE]!=8&&f<=FX_RATE)return 0;
 return 1;}
void FXPreset(FXPATCH *p,unsigned n){static const int a[5][12]={
 {1100,1600,0,0,0,9,12,1,0,100,0,0},
 {2200,-3400,50,12,0,10,22,0,0,100,0,0},
 {160,-120,25,9,0,20,45,0,4,100,0,0},
 {260,900,20,5,0,12,10,1,0,100,0,0},
 {760,0,0,0,0,4,5,2,0,100,0,0}};
 memcpy(p->v,a[n%5],sizeof(p->v));p->seed=1;}
void FXAdjust(FXPATCH *p,unsigned f,int delta){long n;if(!FXEnabled(p,f))return;
 n=(long)p->v[f]+(long)delta*step[f];if(n<lo[f])n=lo[f];if(n>hi[f])n=hi[f];p->v[f]=(int)n;}
void FXRandom(FXPATCH *p,int mutate){unsigned i,s=p->seed;int target=p->v[FX_TARGET];
 if(mutate){for(i=0;i<FX_FIELDS-1;i++)if(FXEnabled(p,i))FXAdjust(p,i,(int)(next(&s)%3)-1);}
 else{FXPreset(p,next(&s)%5);p->v[FX_TARGET]=target;
 p->v[FX_PITCH]=100+(int)(next(&s)%3000);p->v[FX_SLIDE]=(int)(next(&s)%6001)-3000;
 p->v[FX_DEPTH]=(int)(next(&s)%151);p->v[FX_RATE]=(int)(next(&s)%16);
 p->v[FX_HOLD]=4+(int)(next(&s)%60);p->v[FX_RELEASE]=(int)(next(&s)%50);
 p->v[FX_NOISE]=(int)(next(&s)%9);}
 p->seed=s;}
int FXCompile(const FXPATCH *p,FXCLIP *c){unsigned t,n,mode;long hz,tri;int level,on;
 if(!c||!FXValid(p))return 0;
 c->target=(unsigned)p->v[FX_TARGET];
 mode=p->v[FX_NOISE]?((unsigned)p->v[FX_NOISE]-1U)%4U:0;
 if(p->v[FX_NOISE]&&p->v[FX_NOISE]<=4)mode|=4;
 c->mask=c->target?0:p->v[FX_NOISE]?((mode&3)==3?12:8):1;
 n=c->target?(unsigned)p->v[FX_HOLD]:(unsigned)(p->v[FX_ATTACK]+p->v[FX_HOLD]+p->v[FX_RELEASE]);
 c->count=n+1;
 for(t=0;t<=n;t++){
  tri=((long)t*p->v[FX_RATE]*4L)%240L;tri=tri<60?tri:tri<180?120-tri:tri-240;
  hz=(long)p->v[FX_PITCH]+(long)p->v[FX_SLIDE]*(long)t/60L+tri*p->v[FX_DEPTH]/60L;
  if(hz<(c->target?19:110))hz=c->target?19:110;if(hz>7900)hz=7900;
  c->row[t].period=(unsigned)((3579545UL+(unsigned long)hz*16UL)/((unsigned long)hz*32UL));
  if(c->row[t].period>1023)c->row[t].period=1023;
  c->row[t].noise=mode;level=p->v[FX_LEVEL];
  if(t<(unsigned)p->v[FX_ATTACK])level=15-(int)((long)(15-level)*t/p->v[FX_ATTACK]);
  else if(t>=(unsigned)(p->v[FX_ATTACK]+p->v[FX_HOLD])&&p->v[FX_RELEASE])
   level+=(int)((long)(15-level)*(t-p->v[FX_ATTACK]-p->v[FX_HOLD])/p->v[FX_RELEASE]);
  on=(t%(unsigned)(p->v[FX_ON]+p->v[FX_OFF]))<(unsigned)p->v[FX_ON];
  c->row[t].level=(unsigned)((!on||t==n)?15:level);
  c->row[t].pit=(c->target&&on&&t<n)?(unsigned)((3579545UL+(unsigned long)hz*3UL/2UL)/((unsigned long)hz*3UL)):0;
 }
 return 1;}
unsigned FXBytes(const FXCLIP *c,unsigned t,unsigned char *b){unsigned n=0,p=c->row[t].period;
 if(t==0){b[n++]=0x9f;b[n++]=0xbf;b[n++]=0xdf;b[n++]=0xff;}
 if(c->target)return n;
 if(c->mask==1||c->mask==12){b[n++]=(unsigned char)((c->mask==1?0x80:0xc0)|(p&15));b[n++]=(unsigned char)(p>>4);}
 if(c->mask!=1&&t==0)b[n++]=(unsigned char)(0xe0|c->row[t].noise);
 b[n++]=(unsigned char)((c->mask==1?0x90:0xf0)|c->row[t].level);return n;}
unsigned FXEncode(const FXPATCH *p,char *s,unsigned cap){unsigned i,n;char b[FX_TEXT];if(!FXValid(p)||!s)return 0;
 n=(unsigned)sprintf(b,"TFX1\r\n%u\r\n",p->seed);
 for(i=0;i<FX_FIELDS;i++)n+=(unsigned)sprintf(b+n,"%d%c",p->v[i],i==FX_FIELDS-1?'\n':' ');
 if(n>=cap)return 0;memcpy(s,b,n+1);return n;}
int FXDecode(FXPATCH *out,const char *s,unsigned n){FXPATCH p;unsigned at=4,i,digits;unsigned long v;int sign;
 if(!out||!s||n<6||n>=FX_TEXT||memcmp(s,"TFX1",4)||s[4]!='\r')return 0;
 for(i=0;i<=FX_FIELDS;i++){
  while(at<n&&isspace((unsigned char)s[at]))at++;
  sign=1;if(at<n&&s[at]=='-'){sign=-1;at++;}v=digits=0;
  while(at<n&&s[at]>='0'&&s[at]<='9'){if(++digits>5)return 0;
   v=v*10UL+(unsigned)(s[at++]-'0');if(v>65535UL)return 0;}
  if(!digits||(at<n&&!isspace((unsigned char)s[at])))return 0;
  if(!i){if(sign<0)return 0;p.seed=(unsigned)v;}
  else{long field=(long)v*sign;if(field<lo[i-1]||field>hi[i-1])return 0;p.v[i-1]=(int)field;}
 }
 while(at<n&&isspace((unsigned char)s[at]))at++;
 if(at!=n||!FXValid(&p))return 0;*out=p;return 1;}
int FXName(const char *s){unsigned n=0,i;char stem[9];if(!s)return 0;
 while(s[n]&&s[n]!='.'&&n<9){if(!isalnum((unsigned char)s[n])&&s[n]!='_')return 0;stem[n]=(char)toupper((unsigned char)s[n]);n++;}
 if(!n||n>8||strlen(s+n)!=4)return 0;stem[n]=0;
 for(i=0;i<4;i++)if(toupper((unsigned char)s[n+i])!=".FXP"[i])return 0;
 if(!strcmp(stem,"CON")||!strcmp(stem,"PRN")||!strcmp(stem,"AUX")||!strcmp(stem,"NUL"))return 0;
 if(n==4&&(!strncmp(stem,"COM",3)||!strncmp(stem,"LPT",3))&&stem[3]>='1'&&stem[3]<='9')return 0;return 1;}
int FXLoad(FXPATCH *p,const char *path){FILE *f;char b[FX_TEXT];unsigned n;int bad,extra;
 f=fopen(path,"rb");if(!f)return 2;n=(unsigned)fread(b,1,sizeof(b),f);bad=ferror(f);extra=fgetc(f);fclose(f);
 if(bad)return 2;if(extra!=EOF||!FXDecode(p,b,n))return 1;return 0;}
int FXSave(const FXPATCH *p,const char *path,int replace){char b[FX_TEXT],tmp[144],bak[144];unsigned n,len;FILE *f;int exists,bad;
 n=FXEncode(p,b,sizeof(b));len=(unsigned)strlen(path);if(!n||len<4||len>=sizeof(tmp))return 1;
 strcpy(tmp,path);strcpy(tmp+len-3,"$$$");strcpy(bak,path);strcpy(bak+len-3,"BAK");
 f=fopen(path,"rb");exists=f!=0;if(f)fclose(f);if(exists&&!replace)return 3;
 f=fopen(tmp,"rb");if(f){fclose(f);return 3;}f=fopen(tmp,"wb");if(!f)return 2;
 bad=fwrite(b,1,n,f)!=n;if(fclose(f))bad=1;if(bad){remove(tmp);return 2;}
 if(exists){f=fopen(bak,"rb");if(f){fclose(f);if(remove(bak)){remove(tmp);return 2;}}
  if(rename(path,bak)){remove(tmp);return 2;}}
 if(rename(tmp,path)){if(exists&&rename(bak,path))return 4;remove(tmp);return 2;}return 0;}
int FXHeader(const FXCLIP *c,FILE *f){unsigned t;const FXROW *r;
 if(!c||!f||!c->count||c->count>FX_TICKS+1)return 0;
 fprintf(f,"/* Effects Lab: 60 Hz, PSG clock 3579545 Hz; target %s. */\n",c->target?"PC speaker":"Tandy PSG");
 fprintf(f,"/* Include FXPLAY.H from the kit. Acquire sound.mask with libtandy;\n   speaker target requires an app-owned PIT sink. Tick once per 60 Hz. */\n");
 fprintf(f,"static const FXP_ROW sound_rows[%u] = {\n",c->count);
 for(t=0;t<c->count;t++){r=&c->row[t];fprintf(f," {%u,%u,%u,%u}%s\n",r->period,r->level,r->noise,r->pit,t+1==c->count?"":",");}
 fprintf(f,"};\nstatic const FXP_SOUND sound = {%u,%u,%u,sound_rows};\n",c->count,c->target,c->mask);
 return !ferror(f);}
