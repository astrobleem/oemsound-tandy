#include "FMTCORE.H"
static int fail(FXCORE *s,unsigned short e){s->error=e;return -1;}
static int accept(void *u,const VGM_BURST_GROUP *g){(void)u;(void)g;return VGM_SINK_ACCEPT;}
int FXOpen(FXCORE *s,VGM_READ_AT read,void *u,unsigned long size,unsigned short kind)
{
 unsigned short i;unsigned char *p=(unsigned char *)s;
 for(i=0;i<sizeof(*s);i++)p[i]=0;s->kind=kind;
 for(i=0;i<4;i++)s->lane[i]=255;for(i=0;i<3;i++)s->volume[i]=15;
 if(kind==FX_VGM){if(vgm_burst_open(&s->vgm,read,u,size,FX_MIN)<0)return fail(s,1);
  /* Exact Tandy SN76496 profile. Other SN variants need an explicit adapter. */
  if(s->vgm.source.sn_clock_hz!=3579545UL||s->vgm.source.sn_feedback!=6||s->vgm.source.sn_shift_width!=15||s->vgm.source.sn_flags)return fail(s,2);
 }else if(kind==FX_MOD){if(!mod_open(&s->mod,read,u,size,&s->modError)||!mod_preflight(&s->mod,FX_LIMIT,&s->modError))return fail(s,3);
  mod_player_init(&s->player,&s->mod);s->player.stop_at_song_end=1;
 }else return fail(s,4);return 0;
}
static int terminal(FXCORE *s,FXBURST *b){
 if(s->terminal)return 0;s->terminal=1;
 if(!s->groups)return fail(s,5);
 if(s->nextAt>2147483647UL)return fail(s,6);
 if(s->nextAt==s->lastAt)return 0;
 if(s->nextAt<s->lastAt||s->nextAt-s->lastAt<FX_MIN)return fail(s,6);
 b->at=s->nextAt;b->count=0;return 1;
}
int FXNext(FXCORE *s,FXBURST *b)
{
 int r;unsigned short c,v,i,p,mask,chosen;unsigned long n,frac;
 MOD_TickFrame f;
 if(s->error)return -1;if(s->done)return terminal(s,b);
 if(s->groups>=FX_LIMIT)return fail(s,7);
 if(s->kind==FX_VGM){for(;;){
  r=vgm_burst_step(&s->vgm);if(r<0){s->sourceOffset=s->vgm.error_offset;return fail(s,1);}
  if(r==VGM_BURST_DONE){
   s->nextAt=s->vgm.source.pit_cycles;
   if(s->loopOffset&&!s->looped){
    if(s->nextAt<=s->loopAt)return fail(s,8);s->looped=1;
    s->vgm.source.cursor=s->loopOffset;s->vgm.source.ended=0;
    s->vgm.source.total_samples_declared=0;s->vgm.source.loop_samples_declared=0;
    s->vgm.source.loop_offset=0;s->vgm.source.loop_pending=s->vgm.source.loop_seen=0;
    s->vgm.source_done=0;continue;
   }s->done=1;return terminal(s,b);
  }
  if(s->vgm.pending.loop_boundary&&!s->looped){s->loopOffset=s->vgm.pending.loop_source_offset;s->loopAt=s->vgm.pending.at;}
  b->at=s->vgm.pending.at;b->count=s->vgm.pending.count;s->sourceOffset=s->vgm.pending.source_offset;
  for(i=0;i<b->count;i++)b->data[i]=s->vgm.pending.bytes[i];
  vgm_burst_dispatch(&s->vgm,accept,0);if(!b->count)continue;break;
 }}else{
  r=mod_player_tick(&s->player,&f,&s->modError);if(r<0)return fail(s,3);if(!r){s->done=1;return terminal(s,b);}
  b->at=s->nextAt;b->count=0;mask=0;
  for(c=0;c<4;c++)if((f.enabled_mask&(1<<c))&&f.attenuation[c]!=15)mask|=1<<c;
  /* Sticky lane allocation. No voice stealing and no implicit noise lane. */
  for(c=0;c<4;c++)if(!(mask&(1<<c)))s->lane[c]=255;
  for(c=0;c<4;c++)if((mask&(1<<c))&&s->lane[c]==255){
   for(v=0;v<3;v++){chosen=1;for(i=0;i<4;i++)if(s->lane[i]==v)chosen=0;if(chosen)break;}
   if(v==3)return fail(s,9);s->lane[c]=(unsigned char)v;
  }
  for(v=0;v<3;v++){
   chosen=255;for(c=0;c<4;c++)if(s->lane[c]==v)chosen=c;
   if(chosen==255){if(s->volume[v]!=15){b->data[b->count++]=(unsigned char)(0x90|(v<<5)|15);s->volume[v]=15;}continue;}
   c=chosen;if(!mod_period_to_psg(f.period[c],&p))return fail(s,10);
   if(s->period[v]!=p){b->data[b->count++]=(unsigned char)(0x80|(v<<5)|(p&15));b->data[b->count++]=(unsigned char)(p>>4);s->period[v]=p;}
   if(s->volume[v]!=f.attenuation[c]){b->data[b->count++]=(unsigned char)(0x90|(v<<5)|f.attenuation[c]);s->volume[v]=f.attenuation[c];}
  }
  n=f.pit_cycles_numerator;frac=((n%f.pit_cycles_denominator)<<16)/f.pit_cycles_denominator;
  n=n/f.pit_cycles_denominator;frac+=s->phase;s->phase=(unsigned short)frac;
  s->nextAt+=n+(frac>>16);
 }
 if(b->at>2147483647UL||(s->groups&&(b->at<s->lastAt||b->at-s->lastAt<FX_MIN)))return fail(s,6);
 s->lastAt=b->at;s->groups++;return 1;
}
int FXPreflight(FXCORE *s,VGM_READ_AT read,void *u,unsigned long size,unsigned short kind)
{
 FXBURST b;int r;if(FXOpen(s,read,u,size,kind))return -1;
 while((r=FXNext(s,&b))>0){}return r;
}
const char *FXReason(FXCORE *s){switch(s->error){
 case 0:return "Ready";case 1:return vgm_burst_error_text(s->vgm.error);
 case 2:return "Needs Tandy 3579545Hz SN76496, feedback6/width15";
 case 3:return s->modError.reason;case 4:return "Unsupported format";
 case 5:return "No playable events";case 6:return "Unsupported timing: needs >=8ms groups and <30min";
 case 7:return "Too many groups (limit65535)";case 8:return "Empty VGM loop";
 case 9:return "More than three audible MOD lanes";case 10:return "MOD tone outside PSG range";
 }return "Format rejected";
}
