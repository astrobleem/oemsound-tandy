/* SPDX-License-Identifier: MIT. Header-only 60 Hz libtandy export adapter.
   Include lt_audio.h first. Caller serializes audio and owns PIT sink/clock.
   Acquire sound.mask before first tick (PSG only). Keep cursor at zero to
   start. Pause: stop calling Tick and silence layer/PIT; resume restores the
   same row with FXPApply(sound,cursor-1,...,0), then continue FXPTick.
   Stop calls FXPStop then discards cursor. No timer/IO. */
#ifndef FXPLAY_H
#define FXPLAY_H
typedef struct {unsigned period,level,noise,pit;} FXP_ROW;
typedef struct {unsigned count,target,mask;const FXP_ROW *rows;} FXP_SOUND;
typedef int (*FXP_PIT)(void *,unsigned); /* 0=mute, else leased divisor; 0=OK */
static int FXPApply(const FXP_SOUND *s,unsigned t,lta_handle *a,
                   FXP_PIT pit,void *ctx,int initial)
{
 lta_request q;const FXP_ROW *r;int result=0;
 if(!s||!s->rows||t>=s->count)return -1;
 r=&s->rows[t];
 if(s->target)return pit&&!pit(ctx,r->pit)?0:-1;
 if(!a)return -1;
 lta_request_init(&q);
 if(s->mask==1||s->mask==12){q.op=LTA_OP_TONE_START;
  q.channel=(lt_u8)(s->mask==1?0:2);q.period=(lt_u16)r->period;
  q.level=(lt_u8)(s->mask==1?r->level:15);result=lta_command(a,&q);}
 if(!result&&s->mask!=1){q.op=(lt_u8)(initial?LTA_OP_NOISE_START:LTA_OP_LEVEL);
  q.channel=3;q.mode=(lt_u8)r->noise;q.level=(lt_u8)r->level;result=lta_command(a,&q);}
 return result?-1:0;
}
static int FXPTick(const FXP_SOUND *s,unsigned *cursor,lta_handle *a,
                  FXP_PIT pit,void *ctx)
{
 if(!s||!cursor||*cursor>=s->count)return 0;
 if(FXPApply(s,*cursor,a,pit,ctx,*cursor==0))return -1;
 ++*cursor;return *cursor<s->count?1:0;
}
static int FXPStop(const FXP_SOUND *s,lta_handle *a,FXP_PIT pit,void *ctx)
{
 lta_request q;if(!s)return -1;if(s->target)return pit&&!pit(ctx,0)?0:-1;
 lta_request_init(&q);q.op=LTA_OP_STOP;q.mask=(lt_u8)s->mask;
 return lta_command(a,&q)==LTA_OK?0:-1;
}
#endif
