#include <string.h>
#include "BQSCHED.H"

static BQTIME BqMessage(unsigned lane,unsigned note,unsigned velocity,
    int on)
{
    return (BQTIME)((on?0x90:0x80)|(lane==3?9:lane))|
        ((BQTIME)note<<8)|((BQTIME)velocity<<16);
}

unsigned BqMessages(const BQSTATE *old,const BQSTATE *next,BQTIME *out)
{
    unsigned i,n=0;
    int change[4];
    for(i=0;i<4;i++) {
        change[i]=old->note[i]!=next->note[i]||
            old->velocity[i]!=next->velocity[i]||
            (next->retrigger&(1U<<i));
        if(change[i]&&old->note[i])
            out[n++]=BqMessage(i,old->note[i],0,0);
    }
    for(i=0;i<4;i++)
        if(change[i]&&next->note[i])
            out[n++]=BqMessage(i,next->note[i],next->velocity[i],1);
    return n;
}

static void BqStage(BQSCHED *s,const BQSTATE *next)
{
    s->offered=*next;
    s->count=BqMessages(&s->planned,next,s->messages);
    s->index=0;s->staged=1;
}

static int BqDrain(BQSCHED *s)
{
    int result;
    BQTIME due=s->origin+(s->offered.when-s->base);
    while(s->index<s->count) {
        result=s->ops.put(s->ops.context,due,s->messages[s->index]);
        if(result==BQ_FULL)return BQ_FULL;
        if(result!=BQ_OK)return BQ_ERROR;
        s->index++;
    }
    s->planned=s->offered;s->staged=0;
    return BQ_OK;
}

int BqStart(BQSCHED *s,const BQOPS *ops,BQTIME target,BQTIME end,
    const BQSTATE *seed)
{
    BQTIME now;
    BQSTATE initial;
    if(!s||!ops||!seed||!ops->now||!ops->put||!ops->reset||
       !ops->pending||target>=end||end>600000UL)return BQ_ERROR;
    /* A failed acknowledgement must never start a new timeline. */
    if(ops->reset(ops->context)!=BQ_OK)return BQ_ERROR;
    if(ops->now(ops->context,&now)!=BQ_OK)return BQ_ERROR;
    memset(s,0,sizeof(*s));s->ops=*ops;
    s->origin=now+BQ_LEAD;s->base=s->position=target;s->end=end;
    s->active=1;s->seed=1;
    initial=*seed;initial.when=target;initial.retrigger=15;
    BqStage(s,&initial);
    return BQ_OK;
}

int BqPosition(BQSCHED *s,BQTIME *position)
{
    BQTIME now,delta,value;
    if(!s||!position||!s->active)return BQ_ERROR;
    if(s->ops.now(s->ops.context,&now)!=BQ_OK)return BQ_ERROR;
    delta=now-s->origin;
    if(delta&0x80000000UL)value=s->base;
    else value=delta>=s->end-s->base?s->end:s->base+delta;
    if(value<s->position)return BQ_ERROR;
    s->position=*position=value;
    return BQ_OK;
}

int BqSeed(BQSCHED *s)
{
    int result;
    if(!s||!s->active)return BQ_ERROR;
    if(!s->seed)return BQ_OK;
    result=BqDrain(s);
    if(result==BQ_OK)s->seed=0;
    return result;
}

int BqOffer(BQSCHED *s,const BQSTATE *next)
{
    BQTIME position;
    int result;
    if(!s||!s->active||!next||s->eof)return BQ_ERROR;
    result=BqSeed(s);if(result!=BQ_OK)return result;
    if(s->staged) {
        /* Producer must retain its snapshot until fully accepted. */
        if(next->when!=s->offered.when||
           memcmp(next->note,s->offered.note,4)||
           memcmp(next->velocity,s->offered.velocity,4)||
           next->retrigger!=s->offered.retrigger)return BQ_ERROR;
        return BqDrain(s);
    }
    if(next->when<s->base||next->when>s->end||
       next->when<s->planned.when)return BQ_ERROR;
    if(BqPosition(s,&position)!=BQ_OK)return BQ_ERROR;
    if(next->when>position&&next->when-position>BQ_LOOKAHEAD)
        return BQ_LATER;
    BqStage(s,next);
    return BqDrain(s);
}

int BqEnd(BQSCHED *s)
{
    BQSTATE off;
    BQTIME position;
    unsigned pending;
    int result;
    if(!s||!s->active)return BQ_ERROR;
    if(!s->eof) {
        memset(&off,0,sizeof(off));off.when=s->end;
        result=BqOffer(s,&off);
        if(result!=BQ_OK)return result;
        s->eof=1;
    }
    if(BqPosition(s,&position)!=BQ_OK)return BQ_ERROR;
    if(position<s->end)return BQ_LATER;
    if(s->ops.pending(s->ops.context,&pending)!=BQ_OK)return BQ_ERROR;
    return pending?BQ_FULL:BQ_DONE;
}

int BqPause(BQSCHED *s,BQTIME *position)
{
    if(BqPosition(s,position)!=BQ_OK)return BQ_ERROR;
    return BqStop(s);
}

int BqStop(BQSCHED *s)
{
    if(!s)return BQ_ERROR;
    if(!s->active)return BQ_OK;
    if(s->ops.reset(s->ops.context)!=BQ_OK)return BQ_ERROR;
    s->active=0;s->seed=s->staged=s->eof=0;
    s->count=s->index=0;
    return BQ_OK;
}
