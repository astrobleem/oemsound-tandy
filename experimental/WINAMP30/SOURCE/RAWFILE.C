/* Bounded TND v2 / raw AGI four-track score engine. MIT.
   Format and Hz table reference: Jan Knipperts, MIT TNDY-Tracker,
   commit 3c8d8808047a158c84c601de88f93653f051cb7c.
   No DOS IRQ/PIT code, interpreter dissolve, or borrowed GPL implementation. */
#include "RAWFILE.H"
static RF16 word(RF8 RF_FAR *p){return (RF16)(p[0]|((RF16)p[1]<<8));}
static const RF16 hztable[108]={
33,35,37,39,41,44,46,49,52,55,58,62,
65,69,73,78,82,87,92,98,104,110,117,123,
131,139,147,156,165,175,185,196,208,220,233,247,
262,277,294,311,330,349,370,392,415,440,466,494,
523,554,587,622,659,698,740,784,831,880,932,988,
1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,
2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,
4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,
8372,8870,9397,9956,10548,11175,11840,12544,13290,14080,14917,15804};
static RF16 frequency(RF16 n,RF16 o){return hztable[(o-1)*12+n-1];}
static RF16 eventindex(RF16 p,RF16 c,RF16 r){return ((p*4+c)*64+r)*3;}
static void mute(RFSong *s){RF16 c;for(c=0;c<4;c++)s->volume[c]=15;s->dirty=15;}
RF16 RFLoad(RFSong *s,RF8 RF_FAR *d,RF16 n,RF8 RF_FAR *e,RF16 kind)
{
 RF16 i,c,p,r,k,q,m,x,y,z,a,b,count,expanded;RF32 total;
 s->data=d;s->events=e;s->size=n;s->kind=kind;s->error=0;
 if(n<8 || n>RF_MAX)return 1;
 if(kind==1){
  if(n<13 || d[0]!='T'||d[1]!='N'||d[2]!='D'||d[3]!='Y')return 1;
  if(d[4]!=2 || d[5]!=4)return 2;
  s->length=d[6];s->loop=d[7];s->patterns=d[8];s->speed=d[9];s->hz=d[10];
  if(!s->length || !s->patterns || s->patterns>RF_PATTERNS || !s->speed || !s->hz)return 2;
  if(s->loop>=s->length)return 1;
  m=word(d+11);q=13;if(m>n-q)return 1;expanded=0;
  /* Description marker repeats preceding literal; FF extends count by255. */
  for(i=0;i<m;i++){
   if(d[q+i]==1){if(!i || i+1>=m || d[q+i-1]==1)return 1;
    i++;count=d[q+i];if(count==255){if(i+1>=m)return 1;i++;count+=d[q+i];}
    if(!count || expanded+count>401)return 1;expanded+=count;
   }else {if(++expanded>401)return 1;}
  }
  q+=m;if(s->length>n-q)return 1;
  for(i=0;i<s->length;i++){s->order[i]=d[q++];if(s->order[i]>=s->patterns)return 1;}
  for(p=0;p<s->patterns;p++)for(c=0;c<4;c++){
   r=0;while(r<64){
    if(q>=n)return 1;x=d[q++];
    if(x==255){if(q>=n)return 1;count=d[q++];if(!count || count>(RF16)(64-r))return 1;
     for(i=0;i<count;i++){k=eventindex(p,c,r++);e[k]=255;e[k+1]=e[k+2]=0;}continue;}
    y=z=0;if(c<3){if(n-q<2)return 1;y=d[q++];z=d[q++];a=x>>4;b=x&15;
     if(a==15 || (a>=1 && a<=12 && (!b || b>9)))return 2;
     if(a==0 && b)return 1;
     i=y&15;if(!(i<=5 || i==10 || i==11 || i==13 || i==15))return 2;
     if(i==0 && z && a>=1 && a<=12 && ((b-1)*12+a-1+(z>>4)>=108 || (b-1)*12+a-1+(z&15)>=108))return 2;
     if(i==11 && z>=s->length)return 1;
     if(i==13 && z>=64)return 1;
     if(i==15 && !z)return 2;
    }else if((x>>5)>5)return 2;
    k=eventindex(p,c,r++);e[k]=(RF8)x;e[k+1]=(RF8)y;e[k+2]=(RF8)z;
   }
  }
  if(q!=n)return 1;
 }else if(kind==2){
  for(c=0;c<4;c++){s->start[c]=word(d+c*2);if(s->start[c]<8 || s->start[c]>=n)return 1;
   if(c && s->start[c]<=s->start[c-1])return 1;}
  if(s->start[0]!=8)return 2;
  for(c=0;c<4;c++){
   s->end[c]=c==3?n:s->start[c+1];q=s->start[c];total=0;
   for(;;){if(s->end[c]-q<2)return 1;a=word(d+q);q+=2;
    if(a==65535U){if(q!=s->end[c])return 1;break;}
    if(s->end[c]-q<3 || !a)return 1;
    /* A raw AGI PSG record stores high six divider bits, then latch nibble,
       then attenuation nibble; register selector upper bits are ignored. */
    if(c<3 && (d[q]&192))return 2;
    total+=a;if(total>108000UL)return 2;q+=3;
   }
  }
 }else return 2;
 RFReset(s);return 0;
}
void RFReset(RFSong *s)
{
 RF16 c;s->debt=0;s->ticks=0;s->error=0;s->position=s->row=s->tick=0;s->noise=0;
 if(s->kind==1){s->speed=s->data[9];s->hz=s->data[10];}else s->hz=60;
 for(c=0;c<4;c++){s->cursor[c]=s->start[c];s->left[c]=s->done[c]=s->period[c]=0;}
 for(c=0;c<3;c++){s->freq[c]=s->target[c]=s->note[c]=s->fx[c]=s->param[c]=s->frac[c]=s->arp[c]=0;s->octave[c]=1;}
 mute(s);
}
static void pitch(RFSong *s,RF16 c,RF16 f)
{
 RF32 p;if(!f){s->volume[c]=15;s->dirty|=1<<c;return;}
 p=111860UL/f;if(p>1023)p=1023;if(p<1)p=1;s->period[c]=(RF16)p;s->dirty|=1<<c;
}
static void effect(RFSong *s,RF16 c)
{
 RF16 f=s->fx[c],p=s->param[c],v,a,delta,n;
 if(!p)return;
 if(f==0){n=(s->octave[c]-1)*12+s->note[c]-1;
  if(s->arp[c]==1)n+=p>>4;else if(s->arp[c]==2)n+=p&15;
  s->arp[c]=(s->arp[c]+1)%3;if(s->note[c]){if(n>=108){s->error=2;mute(s);return;}pitch(s,c,hztable[n]);}return;}
 if(f==1){if(s->freq[c]<=65535U-p)s->freq[c]+=p;}
 if(f==2){if(s->freq[c]>=110U+p)s->freq[c]-=p;}
 if(f==3 || f==5){if(s->target[c]){
  if(s->freq[c]<s->target[c]){delta=s->target[c]-s->freq[c];s->freq[c]+=delta<p?delta:p;}
  else {delta=s->freq[c]-s->target[c];s->freq[c]-=delta<p?delta:p;}}}
 if(f==4 || f==5){a=p>>4;v=s->volume[c];s->frac[c]+=23*(a?a:p&15);
  delta=s->frac[c]/100;if(delta){s->frac[c]=0;if(a)v=delta>v?0:v-delta;else v=v+delta>15?15:v+delta;}
  s->volume[c]=v;s->dirty|=1<<c;}
 if(f>=1 && f<=5)pitch(s,c,s->freq[c]);
}
RF16 RFStep(RFSong *s)
{
 RF16 c,q,k,x,y,z,n,o,f,nextpos,nextrow,ended=0,a;
 if(++s->ticks>108000UL){s->error=3;mute(s);return 0;}
 if(s->kind==2){
  for(c=0;c<4;c++){
   if(s->done[c]){ended++;continue;}
   if(s->left[c])s->left[c]--;
   if(!s->left[c]){q=s->cursor[c];a=word(s->data+q);
    if(a==65535U){s->done[c]=1;s->volume[c]=15;s->dirty|=1<<c;ended++;continue;}
    s->left[c]=a;s->cursor[c]+=5;s->volume[c]=s->data[q+4]&15;
    if(c<3){s->period[c]=((RF16)(s->data[q+2]&63)<<4)|(s->data[q+3]&15);if(!s->period[c])s->volume[c]=15;}
    else s->noise=s->data[q+3]&7;s->dirty|=1<<c;
   }
  }return ended!=4;
 }
 if(s->tick){for(c=0;c<3;c++)effect(s,c);s->tick--;return s->error?0:1;}
 if(s->position>=s->length){mute(s);return 0;}
 nextpos=s->position;nextrow=s->row+1;if(nextrow==64){nextrow=0;nextpos++;}
 for(c=0;c<4;c++){
  k=eventindex(s->order[s->position],c,s->row);x=s->events[k];y=s->events[k+1];z=s->events[k+2];
  if(x==255){if(c<3)s->fx[c]=s->param[c]=0;continue;}
  if(c==3){n=x>>5;if(n){if(n>=5 || (x&15)==15)s->volume[c]=15;
   else {s->noise=(n-1)|((x&16)?4:0);s->volume[c]=x&15;}s->dirty|=8;}continue;}
  f=y&15;s->fx[c]=f;s->param[c]=z;
  n=x>>4;o=x&15;
  if(n==13){s->volume[c]=15;s->dirty|=1<<c;continue;}
  if(n!=14){s->volume[c]=(!n && !s->freq[c])?15:y>>4;s->dirty|=1<<c;}
  if(n>=1 && n<=12){s->note[c]=n;s->octave[c]=o;a=frequency(n,o);
   if(f==3)s->target[c]=a;
   else if(f!=5){if(f==10){if(z<128)a=a>z?a-z:1;else a+=z-128;}s->freq[c]=a;pitch(s,c,a);}}
  if(f!=0 && f!=3 && f!=10)s->frac[c]=0;
  if(f==0)s->arp[c]=(n==14)?0:1;
  if(f==11){nextpos=z;nextrow=0;}
  else if(f==13){nextpos=s->position+1;nextrow=z;}
  else if(f==15){if(z<31)s->speed=z;else s->hz=z;}
  else effect(s,c);
 }
 s->position=nextpos;s->row=nextrow;s->tick=s->speed-1;return s->error?0:1;
}
RF16 RFAdvance(RFSong *s,RF16 ms)
{
 RF16 count=0;RF32 interval;
 if(ms>500){s->error=3;mute(s);return 0;}
 s->debt+=(RF32)ms*1000UL;
 for(;;){interval=1000000UL/s->hz;if(s->debt<interval)return 1;
  if(++count>128){s->error=3;mute(s);return 0;}
  s->debt-=interval;if(!RFStep(s))return 0;
 }
}
