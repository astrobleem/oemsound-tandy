#include "JMCORE.H"
static void clear(JM_OUT *o){o->count=0;o->stopped=0;}
static void emit(JM_OUT *o,unsigned st,unsigned a,unsigned b){
 if(o->count<JM_MAX_MSG){o->msg[o->count].status=(unsigned char)st;o->msg[o->count].a=(unsigned char)a;o->msg[o->count].b=(unsigned char)b;o->count++;}
}
static int median(int a,int b,int c){int t;if(a>b){t=a;a=b;b=t;}if(b>c){t=b;b=c;c=t;}if(a>b)b=a;return b;}
static int pitch(int x){return JM_NOTE_MIN+x*13/101;}
static int velocity(int y){int v=16+((100-y)*14/100)*8;return v>127?127:v;}
static unsigned long elapsed(unsigned long a,unsigned long b){return (a-b)&0xffffffffUL;}
void jm_init(JM_STATE *s){unsigned i;s->armed=s->releaseGate=s->sounding=0;s->note=66;s->velocity=72;s->x=s->y=50;s->pressCount=s->candidateCount=s->histNext=0;s->candidate=66;s->lastTick=s->lastNote=0;for(i=0;i<3;i++)s->hx[i]=s->hy[i]=50;}
int jm_ready(const JOY_CAL *c,const JOY_SAMPLE *p,unsigned port,int *x,int *y){
 unsigned a,mask;if(port>1)return 0;a=port*2;mask=3u<<a;
 if((p->timed&mask)!=mask||(p->timeout&mask)||(p->early&mask))return 0;
 if((c->centered&mask)!=mask||(c->seen&mask)!=mask)return 0;
 if(c->center[a]<c->low[a]||c->high[a]<c->center[a]||c->center[a+1]<c->low[a+1]||c->high[a+1]<c->center[a+1])return 0;
 if(c->center[a]-c->low[a]<8||c->high[a]-c->center[a]<8||c->center[a+1]-c->low[a+1]<8||c->high[a+1]-c->center[a+1]<8)return 0;
 *x=joy_normal(c,a,p->axis[a]);*y=joy_normal(c,a+1,p->axis[a+1]);return *x>=0&&*y>=0;
}
void jm_arm(JM_STATE *s,int x,int y,unsigned long now){unsigned i;jm_init(s);s->armed=1;s->releaseGate=1;s->x=x;s->y=y;s->note=pitch(x);s->velocity=velocity(y);s->candidate=s->note;s->lastTick=now;s->lastNote=(now-JM_NOTE_GAP)&0xffffffffUL;for(i=0;i<3;i++){s->hx[i]=x;s->hy[i]=y;}}
void jm_stop(JM_STATE *s,JM_OUT *o){clear(o);if(s->armed){if(s->sounding)emit(o,0x80,(unsigned)s->sounding,0);emit(o,0xb0,123,0);emit(o,0xb0,120,0);}s->armed=s->releaseGate=s->sounding=0;s->pressCount=s->candidateCount=0;o->stopped=1;}
void jm_tick(JM_STATE *s,int valid,int x,int y,unsigned buttons,unsigned long now,JM_OUT *o){
 int want,lo,hi;clear(o);if(!s->armed)return;
 if(!valid||x<0||x>100||y<0||y>100||elapsed(now,s->lastTick)>JM_STALL||(buttons&2)){jm_stop(s,o);return;}
 s->lastTick=now;s->hx[s->histNext]=x;s->hy[s->histNext]=y;s->histNext=(s->histNext+1)%3;
 s->x=median(s->hx[0],s->hx[1],s->hx[2]);s->y=median(s->hy[0],s->hy[1],s->hy[2]);s->velocity=velocity(s->y);
 want=pitch(s->x);lo=((s->note-JM_NOTE_MIN)*101+12)/13;hi=((s->note-JM_NOTE_MIN+1)*101+12)/13;
 if(want<s->note&&s->x>=lo-2)want=s->note;
 if(want>s->note&&s->x<hi+2)want=s->note;
 if(want!=s->note){if(want==s->candidate){if(s->candidateCount<2)s->candidateCount++;}else{s->candidate=want;s->candidateCount=1;}}
 else{s->candidate=want;s->candidateCount=0;}
 if(s->candidateCount>=2&&elapsed(now,s->lastNote)>=JM_NOTE_GAP){
  s->note=want;s->candidateCount=0;
  if(s->sounding&&(buttons&1)){emit(o,0x80,(unsigned)s->sounding,0);emit(o,0x90,(unsigned)s->note,(unsigned)s->velocity);s->sounding=s->note;s->lastNote=now;}
 }
 if(!(buttons&1)){s->releaseGate=0;s->pressCount=0;if(s->sounding){emit(o,0x80,(unsigned)s->sounding,0);s->sounding=0;}return;}
 if(s->releaseGate)return;
 if(s->pressCount<2)s->pressCount++;
 if(s->pressCount>=2&&!s->sounding&&elapsed(now,s->lastNote)>=JM_NOTE_GAP){emit(o,0x90,(unsigned)s->note,(unsigned)s->velocity);s->sounding=s->note;s->lastNote=now;}
}
