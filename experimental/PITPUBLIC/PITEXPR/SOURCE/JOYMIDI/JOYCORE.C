#include "JOYCORE.H"
void joy_decode(const unsigned char *raw, unsigned count, JOY_SAMPLE *out) {
 unsigned i,a,bit;
 out->timed=out->early=out->timeout=out->buttons=out->last=0;
 for(a=0;a<4;a++)out->axis[a]=JOY_TIMEOUT;
 if(!count||count>JOY_SAMPLES){out->timeout=15;return;}
 out->last=raw[count-1];out->buttons=(unsigned char)((~out->last>>4)&15);
 for(a=0;a<4;a++) {
  bit=1u<<a;
  if(!(raw[0]&bit)){out->axis[a]=0;out->early|=(unsigned char)bit;continue;}
  for(i=1;i<count;i++)if(!(raw[i]&bit)){out->axis[a]=i;out->timed|=(unsigned char)bit;break;}
  if(i==count)out->timeout|=(unsigned char)bit;
 }
}
void joy_reset(JOY_CAL *c) {
 unsigned a;c->seen=c->centered=0;
 for(a=0;a<4;a++){c->low[a]=JOY_TIMEOUT;c->high[a]=0;c->center[a]=0;}
}
void joy_track(JOY_CAL *c,const JOY_SAMPLE *s) {
 unsigned a,bit;
 for(a=0;a<4;a++){
  bit=1u<<a;
  if(!(s->timeout&bit)){
   if(s->axis[a]<c->low[a])c->low[a]=s->axis[a];
   if(s->axis[a]>c->high[a])c->high[a]=s->axis[a];
   c->seen|=(unsigned char)bit;
  }
 }
}
void joy_center(JOY_CAL *c,const JOY_SAMPLE *s) {
 unsigned a,bit;c->centered=0;
 for(a=0;a<4;a++){bit=1u<<a;c->center[a]=0;if(s->timed&bit){c->center[a]=s->axis[a];c->centered|=(unsigned char)bit;}}
}
int joy_normal(const JOY_CAL *c,unsigned a,unsigned raw) {
 unsigned center,span;unsigned long v;
 if(a>=4||raw>=JOY_TIMEOUT||!(c->centered&(1u<<a))||!(c->seen&(1u<<a)))return -1;
 center=c->center[a];
 if(c->low[a]>=center||c->high[a]<=center)return -1;
 if(raw<=c->low[a])return 0;
 if(raw>=c->high[a])return 100;
 if(raw<=center){span=center-c->low[a];v=(unsigned long)(raw-c->low[a])*50UL;return (int)(v/span);}
 span=c->high[a]-center;v=(unsigned long)(raw-center)*50UL;return 50+(int)(v/span);
}
