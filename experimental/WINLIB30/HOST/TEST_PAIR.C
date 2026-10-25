/* Production callback and the established host hardware/provider harness. */
#define SPEAKER_CLOCK 1
#define main BurstMain
#include "TEST_BURST.C"
#undef main

static void preparePair(TCXPAIR *p,DWORD at,WORD primary,WORD folded)
{
 memset(p,0,sizeof(*p));p->frame.at=at;p->frame.mask=15;
 p->frame.p[0]=primary;p->frame.volume=0xfff4U;p->frame.noise=6;
 p->fallbackP[0]=folded;p->fallbackVolume=0xfff5U;p->pitDivisor=2712;
}
static WORD noiseCount(void)
{
 WORD i,n=0;for(i=0;i<traceCount;i++)if(traceBytes[i]==0xe6)n++;
 return n;
}
static void finishPair(TCXPAIR *p,DWORD at)
{
 memset(p,0,sizeof(*p));p->frame.at=at;p->frame.mask=15;
 p->frame.noise=6;p->frame.volume=p->fallbackVolume=65535U;
}
int main(void)
{
 TCXPAIR p,bad;TCXPAIRINFO cap;WORD n,t,releases,claims;
 int baseline=BurstMain();
 C(!baseline);C(sizeof(TCXPAIR)==28&&sizeof(TCXPAIRINFO)==14);
 C(sizeof(PAIRFOLD)==8&&sizeof(foldQueue)==8192);
 C(TCXPairCaps(TCXP_VERSION,&cap,sizeof(cap))==0);
 C(cap.features==7&&cap.frameBytes==28&&cap.reserved==0);
 C(cap.minDivisor==151&&cap.maxDivisor==65535U&&!owner&&!sound);
 C(TCXPairCaps(2,&cap,sizeof(cap))==3);
 C(TCXPairCaps(TCXP_VERSION,&cap,1)==3);
 preparePair(&p,0,100,500);C(TCXPairPush(&p)==1);
 C(TCXPairLive(&p)==1);
 C(TCOpen()==0);C(TCXFrameMode(2)==3&&bassMode==0);
 C(TCXFrameMode(1)==0&&TCXStatus()==TCX_AVAILABLE);
 n=count;bad=p;bad.flags=TCXP_FORCE_FOLD;
 C(TCXPairPush(&bad)==3&&count==n&&!pairMode);
 bad=p;bad.fallbackP[1]=1024;C(TCXPairPush(&bad)==3&&count==n);
 C(TCXPairPush(&p)==0);
 p.frame.at=100000;p.frame.p[0]=110;p.fallbackP[0]=510;
 C(TCXPairPush(&p)==0);p.fallbackP[0]=1024;
 C(TCStart()==0);t=timer;tick(0);
 C(!pairUsingFold&&lastFrame.p[0]==100&&pitDivisor==2712);
 C(leaseHeld&&owner==1&&timer==t&&unsafe==0);
 /* A DOS suspend followed by a busy reentry chooses already queued fold. */
 HiStop();inIRQ=1;TCBassSuspend();inIRQ=0;
 C(pitSuspended&&!pitDivisor&&bassSuspended);
 pitBusy=1;C(TCResume()==0);tick(1);
 C(pairUsingFold&&lastFrame.p[0]==500&&!pitDivisor);
 C(TCXStatus()==TCX_BUSY&&leaseHeld&&timer==t);
 pitBusy=0;tick(100000);
 C(!pairUsingFold&&lastFrame.p[0]==110&&pitDivisor==2712);
 C(TCXStatus()==TCX_AVAILABLE&&count==0&&timer==t);
 finishPair(&p,200000);C(TCXPairPush(&p)==0);C(TCEnd()==0);
 tick(200000);C(finished&&!active&&!pitDivisor);
 C(TCClose()==0&&!owner&&!pitToken&&!leaseHeld&&!sound);
 /* Initial contention has no token. Foreground retry acquires it later. */
 pitBusy=1;C(TCOpen()==0);C(TCXFrameMode(1)==0);
 C(TCXStatus()==TCX_BUSY&&!pitToken);
 preparePair(&p,0,120,520);C(TCXPairLive(&p)==0);
 C(pairUsingFold&&lastFrame.p[0]==520&&!pitDivisor);
 p.frame.at=200000;C(TCXPairPush(&p)==0);C(TCStart()==0);tick(0);
 t=timer;traceCount=0;pitBusy=0;C(TCXSetBass(1)==0);
 C(!pairUsingFold&&lastFrame.p[0]==120&&pitDivisor==2712);
 C(noiseCount()==0&&timer==t&&leaseHeld);
 C(TCXSetBass(0)==0&&pairUsingFold&&lastFrame.p[0]==520);
 C(!pitDivisor&&noiseCount()==0);
 C(TCXSetBass(1)==0&&!pairUsingFold&&lastFrame.p[0]==120);
 C(pitDivisor==2712&&noiseCount()==0);C(TCClose()==0);
 /* A flushed existing timer can accept live state and rearm, without a
    second clock or losing the retained range-fallback metadata. */
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 preparePair(&p,0,200,600);C(TCXPairLive(&p)==0);
 p.frame.at=100000;C(TCXPairPush(&p)==0);C(TCStart()==0);tick(0);
 t=timer;C(TCFlush()==0);C(!active&&!count&&timer==t);
 preparePair(&p,0,205,605);p.flags=TCXP_FORCE_FOLD;p.pitDivisor=0;
 C(TCXPairLive(&p)==0&&pairUsingFold&&!active&&timer==t);
 C(pairLast.flags==TCXP_FORCE_FOLD&&lastFrame.p[0]==605);
 preparePair(&p,100000,210,610);C(TCXPairPush(&p)==0);
 C(TCRearm()==0);tick(0);C(lastFrame.p[0]==605);
 tick(100000);C(lastFrame.p[0]==210&&!pairUsingFold&&timer==t);
 C(TCClose()==0);
 /* A complete silent live frame can finish without a queued ring slot. */
 C(TCOpen()==0);C(TCXFrameMode(1)==0);finishPair(&p,0);
 C(TCXPairLive(&p)==0);C(TCEnd()==0);
 C(finished&&!active&&!pitDivisor&&lastFrame.volume==65535U);
 C(TCClose()==0);
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 preparePair(&p,0,215,615);C(TCXPairLive(&p)==0);
 C(TCEnd()==3&&!irqDisabled);C(TCClose()==0);
 /* A fast provider failure is observed before the first PSG output byte. */
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 preparePair(&p,0,130,530);C(TCXPairPush(&p)==0);
 p.frame.at=100000;p.frame.p[0]=140;p.fallbackP[0]=540;
 C(TCXPairPush(&p)==0);p.frame.at=200000;C(TCXPairPush(&p)==0);
 C(TCStart()==0);tick(0);pitFailNext=1;traceCount=0;tick(100000);
 C(pairUsingFold&&lastFrame.p[0]==540&&!pitDivisor);
 C(traceBytes[0]==(WORD)(0x80|(540&15)));
 tick(200000);C(!pairUsingFold&&lastFrame.p[0]==140&&pitDivisor);
 C(TCClose()==0);
 /* A forced range fallback survives pause and live state replacement. */
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 preparePair(&p,0,150,550);p.flags=TCXP_FORCE_FOLD;p.pitDivisor=0;
 C(TCXPairPush(&p)==0);p.frame.at=100000;C(TCXPairPush(&p)==0);
 C(TCStart()==0);tick(0);
 C(pairUsingFold&&pairLast.flags==TCXP_FORCE_FOLD&&!pitDivisor);
 C(TCPause()==0);p.frame.at=0;C(TCXPairLive(&p)==1);
 C(TCUnpause()==0);tick(1);
 C(pairUsingFold&&pairLast.flags==TCXP_FORCE_FOLD&&lastFrame.p[0]==550);
 t=timer;claims=(WORD)leaseClaims;releases=pitReleases;
 preparePair(&p,0,160,560);traceCount=0;
 /* Real broker states: reject stopping/detached, accept armed/running.
    The old always-idle host stub concealed refusal of every active update. */
 brokerStateOverride=3;C(TCXPairLive(&p)==1&&traceCount==0&&timer==t);
 brokerStateOverride=0;C(TCXPairLive(&p)==1&&traceCount==0&&timer==t);
 brokerStateOverride=2;hi=0;C(TCXPairLive(&p)==1&&traceCount==0&&timer==t);hi=1;
 brokerStateOverride=1;C(TCXPairLive(&p)==0&&timer==t&&owner==1);
 brokerStateOverride=-1;traceCount=0;C(TCXPairLive(&p)==0);
 C(count==0&&sent==1&&!pairUsingFold&&lastFrame.p[0]==160);
 C(timer==t&&owner==1&&leaseClaims==claims&&pitReleases==releases);
 C(noiseCount()==1&&pairLast.flags==0&&pitDivisor==2712);
 p.frame.at=120000;p.frame.p[0]=170;p.fallbackP[0]=570;
 C(TCXPairPush(&p)==0);tick(20000);
 C(count==1&&lastFrame.p[0]==160);tick(120001);
 C(count==0&&lastFrame.p[0]==170);
 task=2;n=traceCount;C(TCXPairLive(&p)==1);
 C(TCXPairPush(&p)==1);C(TCXSetBass(0)==1);C(TCClose()==1);
 C(traceCount==n&&owner==1&&leaseHeld&&pitToken&&timer==t);task=1;
 /* Failed detach retains both real owners and their timer until retry. */
 failDetach=1;releases=pitReleases;C(TCClose()==1);
 C(owner==1&&leaseHeld&&pitToken&&timer==t&&timerPin);
 C(pitReleases==releases);failDetach=0;C(TCClose()==0);
 C(!owner&&!pitToken&&!leaseHeld&&!timer&&!timerPin);
 /* Missing speaker support and failed starts keep normal close usable. */
 pitMissing=1;C(TCOpen()==0);C(TCXFrameMode(1)==0);
 C(TCXStatus()==TCX_UNAVAILABLE);
 preparePair(&p,0,180,580);C(TCXPairLive(&p)==0);
 C(pairUsingFold&&lastFrame.p[0]==580);C(TCClose()==0);pitMissing=0;
 C(TCOpen()==0);C(TCXFrameMode(1)==0);
 preparePair(&p,0,190,590);C(TCXPairPush(&p)==0);
 failTimer=1;C(TCStart()==2);C(owner&&pitToken&&leaseHeld&&!timer);
 failTimer=0;C(TCClose()==0&&!owner&&!pitToken&&!leaseHeld);
 C(unsafe==0&&!irqDisabled);
 printf("paired RAWCLK: %d total checks, %d failures, unsafe IRQ calls=%d\n",
        checks,failures,unsafe);
 return failures!=0;
}
