/* Actual producer + actual RAWCLK, with the library's existing Win/port mocks.
   Reproduce the observed native underrun, then stream without GUI timer calls. */
#define SPEAKER_CLOCK 1
#define main RawRegressionMain
#include BEATS_RAW_TEST_SOURCE
#undef main
#include "BEATPLAY.H"
static BBPLAYER player;
static BPAT song;
static BBBURST produced;
static TCXBURST queued;
static void publish(unsigned n)
{
    unsigned i,j;
    for(i=0;i<n;++i){
        C(BBNext(&player,&produced));queued.at=(DWORD)produced.at;
        queued.count=(WORD)produced.count;queued.pitDivisor=(WORD)produced.pit;
        for(j=0;j<produced.count;++j)queued.data[j]=produced.data[j];
        C(TCXPush(&queued)==0);
    }
}
static DWORD paintNow;static unsigned paintTick;
static void paintElapsed(DWORD delta)
{
    paintNow+=delta;
    while((DWORD)BBStamp(paintTick)<=paintNow){tick((DWORD)BBStamp(paintTick));++paintTick;}
}
static void paintPublish(unsigned n)
{
    unsigned i;
    for(i=0;i<n;++i){
        /* Conservative measured producer CPU cost:5.5ms per prepared frame. */
        paintElapsed(6562UL);publish(1);
    }
}
int main(void)
{
    unsigned i,n;DWORD stamp;WORD sentBefore;
    C(RawRegressionMain()==0);
    BPDemo(&song,5);C(BBPrepare(&player,&song));
    C(TCXOpen(1)==0&&TCXStatus()==TCX_AVAILABLE);
    publish(TCB_CAPACITY);C(TCStart()==0);tick(0);
    /* A GUI timer that never replenishes eventually loses the native queue. */
    for(i=1;i<600;++i)tick((DWORD)BBStamp(i));
    C(TCDebug(6)==4&&TCDebug(2)==TCB_CAPACITY);C(TCClose()==0);
    C(BBPrepare(&player,&song));C(TCXOpen(1)==0);publish(BB_PREFILL);
    now=0;C(TCStart()==0);tick(0);
    for(i=1;i<1800;++i){
        stamp=(DWORD)BBStamp(i);tick(stamp);
        /* Foreground message-loop service, no WM_TIMER, file IO or parsing. */
        if(i<350||i>370){
            do{n=TCB_CAPACITY-TCDebug(0);if(n>BB_BATCH)n=BB_BATCH;
                if(n)publish(n);
            }while(n);
        }
        C(TCDebug(6)==0);C(TCDebug(2)==i+1);
        if(i==400){
            sentBefore=TCDebug(2);C(TCPause()==0);C(!hi&&!pitDivisor);
            C(TCDebug(2)==sentBefore);C(TCUnpause()==0);tick(stamp+1);
            C(TCDebug(2)==sentBefore&&hi&&pitDivisor==bassLast);
        }
    }
    C(TCClose()==0);C(!owner&&!sound&&!leaseHeld&&!pitToken&&!unsafe);
    /* Native06 measured a9.5s full grid paint. Refill between its five rows,
       including producer CPU time, using actual RAWCLK deadlines/ownership. */
    C(BBPrepare(&player,&song));C(TCXOpen(1)==0);publish(BB_PREFILL);
    now=0;paintNow=0;paintTick=1;C(TCStart()==0);tick(0);
    for(i=0;i<5;++i){
        unsigned batch;
        paintElapsed(2267045UL); /*1.9s row, five rows=9.5s*/
        for(batch=0;batch<8&&TCDebug(0)<TCB_CAPACITY-BB_BATCH;++batch){
            n=TCB_CAPACITY-TCDebug(0);if(n>BB_BATCH)n=BB_BATCH;
            paintPublish(n);
        }
        C(TCDebug(6)==0);
    }
    C(paintTick>BB_PREFILL&&TCDebug(2)==paintTick);C(TCClose()==0);
    C(!owner&&!sound&&!leaseHeld&&!pitToken&&!unsafe);
    printf("PASS merged FX foreground refill and GUI-stall checks=%d failures=%d\n",checks,failures);
    return failures!=0;
}
