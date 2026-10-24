/* Synthetic foreground contract tests, not native driver qualification. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BQSCHED.C"

#define LIMIT 2048
typedef struct { BQTIME due,message; } EVENT;
typedef struct {
    BQTIME now;
    EVENT pending[64],trace[LIMIT];
    unsigned count,used,capacity,resets;
    int putError,resetError,clockError,pendingError;
} FAKE;

static unsigned checks;
#define CHECK(x) do { checks++; if(!(x)) { \
    printf("FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)

static int Now(void *context,BQTIME *now)
{
    FAKE *f=(FAKE *)context;
    if(f->clockError)return BQ_ERROR;
    *now=f->now;return BQ_OK;
}
static int Put(void *context,BQTIME due,BQTIME message)
{
    FAKE *f=(FAKE *)context;
    if(f->putError)return BQ_ERROR;
    if(f->count>=f->capacity)return BQ_FULL;
    f->pending[f->count].due=due;
    f->pending[f->count++].message=message;
    return BQ_OK;
}
static int Reset(void *context)
{
    FAKE *f=(FAKE *)context;
    if(f->resetError)return BQ_ERROR;
    f->count=0;f->resets++;return BQ_OK;
}
static int Pending(void *context,unsigned *count)
{
    FAKE *f=(FAKE *)context;
    if(f->pendingError)return BQ_ERROR;
    *count=f->count;return BQ_OK;
}
static void Dispatch(FAKE *f)
{
    unsigned i;
    while(f->count&&!( (f->now-f->pending[0].due)&0x80000000UL)) {
        CHECK(f->used<LIMIT);
        f->trace[f->used++]=f->pending[0];
        for(i=1;i<f->count;i++)f->pending[i-1]=f->pending[i];
        f->count--;
    }
}
static BQSTATE State(BQTIME when,unsigned note,unsigned lanes)
{
    BQSTATE s;unsigned i;
    memset(&s,0,sizeof(s));s.when=when;
    for(i=0;i<lanes;i++) {
        s.note[i]=(unsigned char)(i==3?38:note+i);
        s.velocity[i]=100;
    }
    return s;
}
static BQOPS Init(FAKE *f,BQSCHED *s)
{
    BQOPS ops;
    memset(f,0,sizeof(*f));memset(s,0,sizeof(*s));f->capacity=64;
    ops.context=f;ops.now=Now;ops.put=Put;
    ops.reset=Reset;ops.pending=Pending;
    return ops;
}

static void Ordering(void)
{
    BQSTATE a=State(0,60,4),b=State(1,60,4);
    BQTIME messages[8];unsigned n,i;
    b.retrigger=15;n=BqMessages(&a,&b,messages);CHECK(n==8);
    for(i=0;i<4;i++) {
        CHECK((messages[i]&0xf0)==0x80);
        CHECK((messages[i+4]&0xf0)==0x90);
        CHECK((messages[i]&15)==(i==3?9:i));
    }
    b=a;b.retrigger=0;CHECK(BqMessages(&a,&b,messages)==0);
    b.velocity[1]=99;CHECK(BqMessages(&a,&b,messages)==2);
}
static void Partial(void)
{
    FAKE f;BQSCHED s;BQOPS ops=Init(&f,&s);
    BQSTATE seed=State(0,60,4),next=State(10,60,4),other;
    unsigned i;
    CHECK(BqStart(&s,&ops,0,1000,&seed)==BQ_OK);
    f.capacity=2;CHECK(BqSeed(&s)==BQ_FULL);
    CHECK(s.index==2&&s.seed&&f.count==2);
    f.now=55;Dispatch(&f);CHECK(BqSeed(&s)==BQ_OK);
    Dispatch(&f);CHECK(f.used==4);
    next.retrigger=15;CHECK(BqOffer(&s,&next)==BQ_FULL);
    CHECK(s.index==2&&s.planned.when==0);
    other=next;other.note[0]++;
    CHECK(BqOffer(&s,&other)==BQ_ERROR);
    f.now=65;
    while(s.staged) {
        Dispatch(&f);
        i=(unsigned)BqOffer(&s,&next);
        CHECK(i==BQ_OK||i==BQ_FULL);
    }
    Dispatch(&f);CHECK(f.used==12&&s.planned.when==10);
    for(i=0;i<8;i++) {
        CHECK(f.trace[4+i].due==65);
        CHECK((f.trace[4+i].message&0xf0)==(i<4?0x80:0x90));
    }
    CHECK(BqStop(&s)==BQ_OK);i=f.resets;
    CHECK(BqStop(&s)==BQ_OK&&f.resets==i);
}
static void Dense(void)
{
    FAKE f;BQSCHED s;BQOPS ops=Init(&f,&s);
    BQSTATE seed=State(0,60,1),next;
    unsigned t,i,tries=0;int result;
    CHECK(BqStart(&s,&ops,0,1000,&seed)==BQ_OK);
    CHECK(BqSeed(&s)==BQ_OK);
    for(t=1;t<=100;t++) {
        next=State(t,60,1);next.retrigger=1;
        do {
            result=BqOffer(&s,&next);CHECK(++tries<500);
            if(result==BQ_FULL){f.now++;Dispatch(&f);}
            else CHECK(result==BQ_OK);
        } while(result==BQ_FULL);
    }
    f.now=155;Dispatch(&f);CHECK(f.used==201);
    CHECK((f.trace[0].message&0xf0)==0x90);
    for(i=1;i<201;i++) {
        CHECK(f.trace[i].due==55+(i+1)/2);
        CHECK((f.trace[i].message&0xf0)==(i&1?0x80:0x90));
    }
    CHECK(BqStop(&s)==BQ_OK);
}
static void ClockAndSeek(void)
{
    FAKE f;BQSCHED s;BQOPS ops=Init(&f,&s);
    BQSTATE seed=State(0,60,1),next=State(5500,64,1);
    BQTIME position,origin;unsigned resets;
    f.now=0xfffffff0UL;
    CHECK(BqStart(&s,&ops,5000,6000,&seed)==BQ_OK);
    origin=s.origin;CHECK(origin==39);
    CHECK(BqPosition(&s,&position)==BQ_OK&&position==5000);
    CHECK(BqSeed(&s)==BQ_OK&&f.pending[0].due==origin);
    CHECK(BqOffer(&s,&next)==BQ_LATER);
    f.now=origin+250;
    CHECK(BqPosition(&s,&position)==BQ_OK&&position==5250);
    CHECK(BqOffer(&s,&next)==BQ_OK);
    CHECK(f.pending[f.count-1].due==origin+500);
    f.now=origin+251;
    CHECK(BqPause(&s,&position)==BQ_OK&&position==5251);
    CHECK(f.count==0&&!s.active);resets=f.resets;
    /* Old future events disappear before a backward-seek seed. */
    seed=State(0,67,1);
    CHECK(BqStart(&s,&ops,1000,6000,&seed)==BQ_OK);
    CHECK(f.resets==resets+1&&s.base==1000);
    CHECK(BqSeed(&s)==BQ_OK&&f.count==1);
    CHECK(((f.pending[0].message>>8)&127)==67);
    f.now=s.origin+100;CHECK(BqPosition(&s,&position)==BQ_OK);
    f.now--;CHECK(BqPosition(&s,&position)==BQ_ERROR);
    CHECK(BqStop(&s)==BQ_OK);
}
static void EndAndErrors(void)
{
    FAKE f;BQSCHED s;BQOPS ops=Init(&f,&s);
    BQSTATE seed=State(0,60,1),next=State(100,61,1);
    BQTIME position;unsigned resets;
    CHECK(BqStart(&s,&ops,0,300,&seed)==BQ_OK);
    CHECK(BqSeed(&s)==BQ_OK);
    CHECK(BqEnd(&s)==BQ_LATER&&!s.eof);
    f.now=s.origin+50;CHECK(BqEnd(&s)==BQ_LATER&&s.eof);
    CHECK(f.pending[f.count-1].due==s.origin+300);
    CHECK((f.pending[f.count-1].message&0xf0)==0x80);
    CHECK(BqOffer(&s,&next)==BQ_ERROR);
    f.now=s.origin+300;CHECK(BqEnd(&s)==BQ_FULL);
    f.pendingError=1;CHECK(BqEnd(&s)==BQ_ERROR);
    f.pendingError=0;Dispatch(&f);CHECK(BqEnd(&s)==BQ_DONE);
    CHECK(BqStop(&s)==BQ_OK);
    CHECK(BqStart(&s,&ops,0,300,&seed)==BQ_OK);
    CHECK(BqSeed(&s)==BQ_OK);
    f.resetError=1;resets=f.resets;
    CHECK(BqPause(&s,&position)==BQ_ERROR&&s.active&&f.count==1);
    CHECK(BqStart(&s,&ops,100,300,&seed)==BQ_ERROR);
    CHECK(f.resets==resets&&s.base==0&&f.count==1);
    f.resetError=0;CHECK(BqStop(&s)==BQ_OK&&f.count==0);
    CHECK(BqStart(&s,&ops,0,300,&seed)==BQ_OK);
    f.putError=1;CHECK(BqSeed(&s)==BQ_ERROR);
    CHECK(BqStop(&s)==BQ_OK);f.putError=0;
    f.clockError=1;CHECK(BqStart(&s,&ops,0,300,&seed)==BQ_ERROR);
    f.clockError=0;CHECK(BqStart(&s,&ops,300,300,&seed)==BQ_ERROR);
    CHECK(BqStart(&s,&ops,0,600001UL,&seed)==BQ_ERROR);
}
int main(void)
{
    CHECK(sizeof(BQTIME)==4);
    Ordering();Partial();Dense();ClockAndSeek();EndAndErrors();
    printf("PASS: %u synthetic queue/client contract checks\n",checks);
    return 0;
}
