/* Execute selected actual TINYVID functions against synthetic memory I/O. */
#define main ContractMain
#include "BQTEST.C"
#undef main

typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef BQSTATE MUSICSTATE;
#define MUSIC_HEADER 20
#define RECORD_SIZE 14
#define MUSIC_CHUNK 64
#define MODM_DATA 7
#define MIDI_COOKIE 0x544dUL

static BQSCHED queue;
static int mode=3,queueReady,queueBlocked,finished,lastError;
static int nextValid,musicFile=1;
static DWORD timelineBase,playEnd,startTick,lastElapsed,wallNow;
static DWORD eventCount,recordsRead,previousEvent;
static DWORD appliedEvents,skippedEvents,maxLateness,totalLateness;
static DWORD maxMusicRead,duration;
static MUSICSTATE current,nextState;
static BYTE musicBuffer[MUSIC_CHUNK*RECORD_SIZE],raw[8192];
static unsigned bufferCount,bufferIndex;
static unsigned offset,rawBytes,directCount;
static DWORD directMessages[2048];

static int fail(int code,int music)
{
    (void)music;lastError=code;return 0;
}
static DWORD GetTickCount(void) {return wallNow;}
static long _llseek(int file,long where,int origin)
{
    (void)file;
    if(origin||where<0||(unsigned long)where>rawBytes)return -1L;
    offset=(unsigned)where;return where;
}
static unsigned _lread(int file,void *bytes,unsigned count)
{
    (void)file;
    if(count>rawBytes-offset)count=rawBytes-offset;
    memcpy(bytes,raw+offset,count);offset+=count;return count;
}
static unsigned wordAt(BYTE *p)
{
    return (unsigned)p[0]|((unsigned)p[1]<<8);
}
static DWORD longAt(BYTE *p)
{
    return (DWORD)wordAt(p)|((DWORD)wordAt(p+2)<<16);
}
static DWORD Midi(unsigned device,unsigned message,DWORD cookie,
    DWORD packed,DWORD unused)
{
    (void)device;(void)unused;
    CHECK(message==MODM_DATA&&cookie==MIDI_COOKIE);
    CHECK(!queueReady&&directCount<2048);
    directMessages[directCount++]=packed;return 0;
}
static DWORD (*midi)(unsigned,unsigned,DWORD,DWORD,DWORD)=Midi;

/* TESTBQ.PY extracts these function bodies from the real TINYVID.C. */
#include "BQPROD.H"

static void Fixture(unsigned count,DWORD end)
{
    unsigned i,j,p;
    memset(raw,0,sizeof(raw));eventCount=count;duration=end;
    rawBytes=MUSIC_HEADER+count*RECORD_SIZE;
    for(i=0;i<count;i++) {
        p=MUSIC_HEADER+i*RECORD_SIZE;
        for(j=0;j<4;j++)raw[p+j]=(BYTE)((DWORD)i*10>>(8*j));
        raw[p+4]=60;raw[p+8]=100;raw[p+12]=1;
    }
    recordsRead=previousEvent=0;bufferCount=bufferIndex=0;
    nextValid=0;offset=MUSIC_HEADER;
    appliedEvents=skippedEvents=directCount=0;
    lastError=queueBlocked=finished=0;
    memset(&current,0,sizeof(current));CHECK(fetchEvent());
}
static void ImmediateBacklog(void)
{
    unsigned i;
    queueReady=0;playEnd=3000;Fixture(201,3000);
    CHECK(updateMusic(2000));
    CHECK(appliedEvents==64&&queueBlocked&&nextState.when==640);
    while(queueBlocked)CHECK(updateMusic(2000));
    CHECK(appliedEvents==201&&skippedEvents==0&&!nextValid);
    CHECK(directCount==401);
    for(i=0;i<directCount;i++)
        CHECK((directMessages[i]&0xf0)==(i&1?0x80:0x90));
}
static void Reconstruction(void)
{
    MUSICSTATE seed;
    Fixture(10,3000);
    CHECK(musicAt(45,&seed));
    CHECK(seed.when==45&&seed.note[0]==60&&seed.retrigger==15);
    CHECK(nextValid&&nextState.when==50);
    CHECK(musicAt(0,&seed)&&nextState.when==10);
    CHECK(musicAt(200,&seed)&&!nextValid);
    /* Seek does not emit historical note attacks. */
    CHECK(directCount==0);
}
static void QueuedSource(void)
{
    FAKE f;BQOPS ops=Init(&f,&queue);MUSICSTATE seed;
    unsigned i,attempts=0;
    queueReady=1;timelineBase=0;playEnd=120;Fixture(11,120);
    CHECK(musicAt(0,&seed));
    CHECK(BqStart(&queue,&ops,0,120,&seed)==BQ_OK);
    f.capacity=3;
    CHECK(updateMusic(0)&&queueBlocked);
    CHECK(directCount==0&&appliedEvents==1);
    CHECK(nextState.when==20&&queue.staged);
    while(!finished) {
        CHECK(++attempts<1000);
        f.now++;Dispatch(&f);CHECK(updateMusic(0));
    }
    CHECK(appliedEvents==10&&skippedEvents==0&&directCount==0);
    CHECK(f.used==22&&f.count==0);
    for(i=1;i<21;i++)
        CHECK(f.trace[i].due==55+((i+1)/2)*10);
    CHECK(f.trace[21].due==175);
    wallNow=99999;
    CHECK(movieNow()==120);
    CHECK(BqStop(&queue)==BQ_OK);
    queueReady=0;
    timelineBase=100;playEnd=500;startTick=1000;wallNow=1050;
    CHECK(movieNow()==150);
    wallNow=2000;CHECK(movieNow()==500);
}
int main(void)
{
    ImmediateBacklog();Reconstruction();QueuedSource();
    printf("PASS: %u checks of actual TINYVID scheduling functions\n",
        checks);
    return 0;
}
