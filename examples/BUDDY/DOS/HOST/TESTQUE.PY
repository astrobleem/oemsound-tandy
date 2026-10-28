from pathlib import Path
import hashlib, json, subprocess

import argparse,sys
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);p.add_argument('--cc',default='gcc');p.add_argument('--launcher',type=Path);args=p.parse_args()
ROOT=Path(__file__).resolve().parents[1]
OUT=args.out.resolve();OUT.mkdir(parents=True,exist_ok=False)
COMPILER=[sys.executable,str(args.launcher),'--'] if args.launcher else [args.cc]
raw=(ROOT/'SOURCE/DOSPLAY.C').read_bytes()
source=raw.decode('ascii')
(OUT/'PINNED-DOSPLAY.C').write_bytes(raw)
tick=source[source.index('static void audioTick('):source.index('static void initCga(')]
assert tick.count('if(recordsRead==eventCount)producerDone=1;')==1
assert tick.count('qtail=(tail+1)&(QUEUE_SIZE-1);')==1
instrumented=tick.replace('if(recordsRead==eventCount)producerDone=1;',
    'if(recordsRead==eventCount){producerDone=1; if(inject_at==1){++injections;audioTick(0);}}')
instrumented=instrumented.replace('qtail=(tail+1)&(QUEUE_SIZE-1);',
    'qtail=(tail+1)&(QUEUE_SIZE-1); if(producerDone&&inject_at==2){++injections;audioTick(0);}')
instrumented=instrumented.replace('if(!fetchEvent())return 0;',
    'if(!fetchEvent()){return 0;}\n        if(producerDone&&inject_at==3){++injections;audioTick(0);}')

prefix=r'''
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
typedef unsigned long U32;
typedef unsigned char U8;
typedef struct { U32 when; U8 note[4],velocity[4],retrigger; } STATE;
typedef struct { void (*state)(const STATE *); void (*mute)(void); } AUDIO;
#define QUEUE_SIZE 128
static volatile unsigned qhead,qtail;
static STATE queue[QUEUE_SIZE],nextState,feed[2048];
static volatile U32 audioNow,audioFraction,underruns;
static volatile int audioDone,producerDone;
static int dry,mode=1,nextValid,inject_at,injections;
static U32 playEnd,maxLate,totalLate,applied,lastService,maxGap;
static U32 recordsRead,eventCount,observed[2048];
static unsigned audio_mask,traceCount,stateCalls,muteCalls;
typedef struct { U32 scheduled,actual; unsigned note,mask; } TRACE;
static TRACE trace[1024];
static void state(const STATE *s) {
    assert(stateCalls<2048);observed[stateCalls++]=s->when;
    audio_mask=s->note[1]?2:0;
}
static void mute(void) { ++muteCalls;audio_mask=0; }
static const AUDIO backend={state,mute};
static const AUDIO *audio=&backend;
static int fetchEvent(void) {
    if(recordsRead==eventCount){nextValid=0;return 1;}
    nextState=feed[recordsRead++];nextValid=1;return 1;
}
static void reset(void) {
    qhead=qtail=0;audioNow=audioFraction=underruns=0;
    audioDone=producerDone=dry=nextValid=0;mode=1;
    playEnd=100000;maxLate=totalLate=applied=lastService=maxGap=0;
    recordsRead=eventCount=0;audio_mask=traceCount=stateCalls=muteCalls=0;
    inject_at=injections=0;
    memset(queue,0,sizeof(queue));memset(feed,0,sizeof(feed));
    memset(observed,0,sizeof(observed));memset(trace,0,sizeof(trace));
    memset(&nextState,0,sizeof(nextState));
}
static void load(unsigned count) {
    unsigned i;eventCount=count;
    for(i=0;i<count;++i){feed[i].when=i+1;feed[i].note[1]=(U8)(24+i%73);feed[i].velocity[1]=100;}
    assert(fetchEvent());
}
'''
tests=r'''
static void eof_interleavings(void) {
    unsigned ring,count,hook,i;
    for(ring=0;ring<2;++ring)for(count=1;count<=2;++count)for(hook=0;hook<4;++hook){
        reset();qhead=qtail=ring?127:0;load(count);inject_at=hook;
        assert(musicService(0));
        while(applied<count)audioTick(0);
        assert(producerDone&&qhead==qtail&&audio_mask==2&&underruns==0);
        assert(stateCalls==count&&injections==(hook?1:0));
        for(i=0;i<count;++i)assert(observed[i]==i+1);
        printf("EOF ring=%u states=%u hook=%u applied=%lu mask=%u underruns=%lu\n",ring?127:0,count,hook,applied,audio_mask,underruns);
    }
}
static void starvation_refill(void) {
    unsigned i,before;reset();load(4);
    feed[1].when=20;feed[2].when=40;feed[3].when=80;
    queue[0]=nextState;qtail=1;assert(fetchEvent());
    audioTick(0);assert(applied==1&&audio_mask==0&&underruns==1&&dry);
    audioTick(0);assert(applied==1&&underruns==1&&muteCalls==1);
    queue[qtail]=nextState;qtail=(qtail+1)&127;assert(fetchEvent());
    audioTick(0);assert(applied==2&&audio_mask==0&&underruns==2&&dry);
    assert(musicService(audioNow));assert(producerDone);
    audioTick(0);assert(applied==2&&audio_mask==0&&underruns==2);
    audioTick(0);assert(applied==3&&audio_mask==2&&!dry);
    while(applied<4)audioTick(0);
    assert(audio_mask==2&&underruns==2&&qhead==qtail&&maxLate<=8);
    assert(observed[0]==1&&observed[1]==20&&observed[2]==40&&observed[3]==80);
    playEnd=90;before=muteCalls;
    while(!audioDone)audioTick(0);
    assert(audio_mask==0&&muteCalls==before+1);
    for(i=0;i<10;++i)audioTick(0);
    assert(muteCalls==before+1&&applied==4&&underruns==2);
    printf("STARVATION applied=%lu underruns=%lu final_mask=%u mute_once_per_gap=1 deadline_once=1\n",applied,underruns,audio_mask);
}
static void dense(unsigned count) {
    unsigned i,before,delta,max_delta=0,callbacks=0,tail,read;U32 first;
    reset();load(count);audioNow=10000;
    assert(musicService(audioNow));
    if(count>127){
        assert(((qtail-qhead)&127)==127&&!producerDone);
        tail=qtail;read=(unsigned)recordsRead;first=nextState.when;
        assert(musicService(audioNow));
        assert(qtail==tail&&recordsRead==read&&nextState.when==first);
    }
    while(applied<count){
        before=stateCalls;audioTick(0);delta=stateCalls-before;
        assert(delta<=8);if(delta>max_delta)max_delta=delta;++callbacks;
        assert(((qtail-qhead)&127)<=127);
        assert(musicService(audioNow));assert(callbacks<2048);
    }
    assert(stateCalls==count&&producerDone&&qhead==qtail&&underruns==0);
    assert(traceCount==(count>1024?1024:count));
    for(i=0;i<count;++i)assert(observed[i]==i+1);
    if(count)assert(audio_mask==2);
    printf("DENSE states=%u max_per_callback=%u callbacks=%u trace=%u underruns=%lu\n",count,max_delta,callbacks,traceCount,underruns);
}
static void diagnostics(void) {
    reset();lastService=63395UL;assert(musicService(63395UL));assert(maxGap==0);
    assert(musicService(63395UL+453));assert(maxGap==453);
    printf("GAP initial=0 after_service=453\n");
    reset();mode=2;load(1);playEnd=16;assert(musicService(0));
    assert(qhead==qtail&&!producerDone);
    audioTick(0);assert(applied==0&&underruns==0&&muteCalls==0);
    audioTick(0);assert(applied==0&&audioDone&&muteCalls==1);
    printf("VIDEO_ONLY callback_states=0 underruns=0 deadline_once=1\n");
    reset();producerDone=1;playEnd=0xffffffffUL;
    {unsigned i;for(i=0;i<60000;++i)audioTick(0);}
    printf("CLOCK callbacks=60000 logical_ms=%lu fraction=%lu\n",audioNow,audioFraction);
}
int main(void) {
    static const unsigned cases[]={0,1,8,9,16,17,127,128,200,1500};unsigned i;
    assert(ULONG_MAX>=0xffffffffUL);
    eof_interleavings();starvation_refill();
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)dense(cases[i]);
    diagnostics();return 0;
}
'''

cue=source[source.index('static int openCue('):source.index('static int primeMusic(')]
helpers=source[source.index('static unsigned wordAt('):source.index('static U32 since(')]
cue_prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char U8;
typedef unsigned long U32;
#define O_RDONLY 0
static U32 duration,playStart,playEnd;
static int fullRange,chorus,error,closeCalls,openFails,shortRead;
static U8 cueBytes[16];
static int fail(int n){error=n;return 0;}
static int _dos_open(const char *s,int f,int *handle){(void)s;(void)f;*handle=1;return openFails;}
static int _dos_close(int f){(void)f;++closeCalls;return 0;}
static long lseek(int f,long n,int whence){(void)f;(void)n;return whence==2?16L:0L;}
static int readExact(int f,void *p,unsigned n){(void)f;memcpy(p,cueBytes,n);return !shortRead;}
static void put32(unsigned at,U32 n){unsigned i;for(i=0;i<4;++i)cueBytes[at+i]=(U8)(n>>(i*8));}
static void setup(U32 n){
    memset(cueBytes,0,sizeof(cueBytes));memcpy(cueBytes,"WZC1",4);
    put32(4,n);put32(8,0);put32(12,n);
    duration=n;playStart=playEnd=0;error=closeCalls=openFails=shortRead=0;chorus=1;fullRange=0;
}
'''
cue_tests=r'''
int main(void){
    static const U32 durations[]={1000,63394,63395,70000,87173,87174,87175,241400};
    unsigned i,full;int ok;
    for(full=0;full<2;++full)for(i=0;i<sizeof(durations)/sizeof(durations[0]);++i){
        setup(durations[i]);fullRange=full;ok=openCue();
        assert(closeCalls==1);
        if(durations[i]<87174UL)assert(!ok&&error==52);
        else assert(ok&&!error&&playStart==63395UL&&playEnd==87174UL);
        printf("CUE duration=%lu full=%u accepted=%d error=%d start=%lu end=%lu\n",duration,full,ok,error,playStart,playEnd);
    }
    setup(241400);chorus=0;assert(openCue()&&playStart==0&&playEnd==241400);
    setup(241400);cueBytes[0]='X';assert(!openCue()&&error==52&&closeCalls==1);
    setup(241400);put32(4,241399);assert(!openCue()&&error==52&&closeCalls==1);
    setup(241400);shortRead=1;assert(!openCue()&&error==51&&closeCalls==1);
    setup(241400);openFails=1;assert(!openCue()&&error==50&&closeCalls==0);
    printf("CUE_ERRORS non_chorus=pass magic=52 duration_mismatch=52 short_read=51 missing=50\n");
    return 0;
}
'''

def build_run(name,text):
    c=OUT/(name+'.c');exe=OUT/(name+'.exe');c.write_text(text)
    build=subprocess.run(COMPILER+['-x','c','-std=c89','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],capture_output=True,text=True)
    (OUT/(name+'-build.log')).write_text(build.stdout+build.stderr)
    if build.returncode:print(build.stdout+build.stderr,file=sys.stderr)
    build.check_returncode()
    run=subprocess.run([str(exe)],capture_output=True,text=True)
    (OUT/(name+'.log')).write_text(run.stdout+run.stderr)
    run.check_returncode()
    return run.stdout.splitlines()

report={'production_source_sha256':hashlib.sha256(raw).hexdigest(),
        'scope':'Host logic checks of extracted production functions; no emulator, hardware timing or ABI qualification',
        'extracted_audio_service_sha256':hashlib.sha256(tick.encode()).hexdigest(),
        'unmodified_functions':build_run('production-control',prefix+tick+tests.replace('for(hook=0;hook<4;++hook)','for(hook=0;hook<1;++hook)')),
        'injected_functions':build_run('production-interleavings',prefix+instrumented+tests),
        'cue_boundary_functions':build_run('production-cue',cue_prefix+helpers+cue+cue_tests)}
assert 'lastService=playStart;\n    if(!musicService(playStart))' in source.replace('\r\n','\n')
report['main_initial_gap_order_verified']=True
report['timer_opened_diagnostic_verified']='timerOpen=1;timerWasOpen=1;lttk_run(1);' in source and 'timer_opened=%u' in source
report['report_end_marker_verified']=source.index('report_end=1')>source.index('for(i=0;i<traceCount;i++)fprintf')
for rows in [report['unmodified_functions'],report['injected_functions']]:
    clock=next(x for x in rows if x.startswith('CLOCK '))
    value=int(clock.split('logical_ms=')[1].split()[0]);fraction=int(clock.split('fraction=')[1])
    assert value==(60000*9943*1000)//1193182 and fraction==(60000*397544)%1193182
report['status']='HOST_LOGIC_PASS'
(OUT/'FUNCTION-CHECKS.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
