/* WZV2/WZM1 DOS player. Open Watcom, 8086 small model. */
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
#include "AUDIO.H"
#include "lt_tick_dos.h"
#define U32 unsigned long
#define U8 unsigned char
#define VIDEO_HEADER 24
#define MUSIC_HEADER 20
#define RECORD_SIZE 14
#define MUSIC_CHUNK 64
#define MAX_FRAME 6144
#define DAY_TICKS 0x1800b0UL
extern U32 __cdecl ClockTick(void);
extern unsigned __cdecl VideoMode(void);
extern void __cdecl SetMode(unsigned);
extern void __cdecl VideoRow(U8 *,unsigned,unsigned);
extern void __cdecl PsgByte(unsigned);
extern unsigned __cdecl KeyRead(void);
extern unsigned __cdecl IsWindows(void);
static int vf=-1,mf=-1,haveHardware,displayChanged,handlers,mode=3,error;
static int graphics=-1, audioChoice=-1, tandy, timerOpen, timerStatus, chorus;
static unsigned timerWasOpen;
static const AUDIO *audio=&buddy_silent;
static unsigned long oldVector, newVector, timerLate;
static unsigned backlog, gateAfter;
static U8 cgaMap[256], row[160];
#define QUEUE_SIZE 128
static volatile unsigned qhead, qtail;
static STATE queue[QUEUE_SIZE];
static volatile U32 audioNow, audioFraction, underruns;
static volatile int audioDone, producerDone;
static int dry;
static U32 queueBytes;
typedef struct { U32 scheduled, actual; unsigned note, mask; } TRACE;
static TRACE trace[1024];
static unsigned traceCount;
static unsigned oldMode,width,height,fps,frameBytes,rowBytes,xByte,yTop;
static U8 pixels[MAX_FRAME],musicBuffer[MUSIC_CHUNK*RECORD_SIZE];
static unsigned bufferCount,bufferIndex;
static U32 frames,duration,eventCount,recordsRead,previousEvent;
static U32 startTick,lastService,lastFrame=0xffffffffUL;
static U32 playStart,playEnd,prerolled;
static int fullRange;
static U32 rendered,dropped,applied,skipped,maxLate,totalLate,maxGap;
static U32 maxVideoRead,maxMusicRead,maxDraw,elapsedEnd;

static volatile int interrupted;
static void (__interrupt __far *oldBreak)();
static void (__interrupt __far *oldCritical)();

static STATE nextState,seed;
static int seedValid;
static int nextValid;
static unsigned wordAt(U8 *p) {return (unsigned)p[0]|((unsigned)p[1]<<8);}
static U32 longAt(U8 *p) {return (U32)wordAt(p)|((U32)wordAt(p+2)<<16);}
static U32 since(U32 first,U32 last) {return last>=first?last-first:DAY_TICKS-first+last;}
static U32 elapsed(void)
{
    U32 cycles;
    if(!timerOpen)return playStart;
    cycles=lttk_now();
    return playStart+(cycles/1193182UL)*1000UL+
           (cycles%1193182UL)*1000UL/1193182UL;
}
static void mute(void)
{
    if(haveHardware)audio->mute();
}
static int fail(int n) {error=n;return 0;}
static int readExact(int f,void *p,unsigned n)
{
    unsigned got;return !_dos_read(f,(void __far *)p,n,&got)&&got==n;
}
void __interrupt __far onBreak(void) {interrupted=1;}
int __far onCritical(unsigned deverr,unsigned errcode,unsigned __far *devhdr)
{
    (void)deverr;(void)errcode;(void)devhdr;_hardresume(_HARDERR_FAIL);return 3;
}
static void cleanup(void)
{
    if(timerOpen){
        lttk_run(0);backlog=lttk_backlog_max();timerLate=lttk_late();
        timerStatus=lttk_close();timerOpen=0;
    }
    mute();gateAfter=inp(0x61)&3;
    newVector=(U32)_dos_getvect(8);
    if(displayChanged){SetMode(oldMode);displayChanged=0;}
    if(vf>=0){_dos_close(vf);vf=-1;}
    if(mf>=0){_dos_close(mf);mf=-1;}
}
static void restoreHandlers(void)
{
    if(handlers){_dos_setvect(0x23,oldBreak);_dos_setvect(0x24,oldCritical);handlers=0;}
}
static int openVideo(void)
{
    U8 h[VIDEO_HEADER];U32 length,expected,step;
    if(_dos_open("WEEZER.WZV",O_RDONLY,&vf))return fail(10);
    if(!readExact(vf,h,VIDEO_HEADER))return fail(11);
    width=wordAt(h+4);height=wordAt(h+6);fps=wordAt(h+8);
    if(memcmp(h,"WZV2",4)||wordAt(h+10)!=VIDEO_HEADER||
       !((width==64&&height==48&&fps==4)||(width==128&&height==96&&fps==8)))return fail(12);
    frames=longAt(h+12);duration=longAt(h+16);
    frameBytes=width*height/2;rowBytes=width/2;
    if(longAt(h+20)!=(U32)frameBytes)return fail(12);
    step=1000UL/fps;
    if(!frames||frames>4800UL||!duration||duration>600000UL||
       duration>frames*step||duration<=(frames-1)*step)return fail(13);
    expected=VIDEO_HEADER+frames*(U32)frameBytes;length=(U32)lseek(vf,0L,2);
    if(length!=expected)return fail(14);
    xByte=(320-width)/(graphics==4?8:4);yTop=(200-height)/2;return 1;
}
static int fetchEvent(void)
{
    U8 *p;unsigned i,wanted;U32 n,t,spent;
    if(recordsRead==eventCount){nextValid=0;return 1;}
    if(bufferIndex==bufferCount){
        n=eventCount-recordsRead;if(n>MUSIC_CHUNK)n=MUSIC_CHUNK;
        wanted=(unsigned)n*RECORD_SIZE;t=ClockTick();
        if(!readExact(mf,musicBuffer,wanted))return fail(23);
        spent=since(t,ClockTick());if(spent>maxMusicRead)maxMusicRead=spent;
        bufferCount=(unsigned)n;bufferIndex=0;
    }
    p=musicBuffer+bufferIndex*RECORD_SIZE;nextState.when=longAt(p);
    if(nextState.when>duration||(recordsRead&&nextState.when<=previousEvent)||
       (p[12]&0xf0)||p[13])return fail(24);
    for(i=0;i<4;i++){
        nextState.note[i]=p[4+i];nextState.velocity[i]=p[8+i];
        if(p[8+i]>127||(!p[4+i]&&p[8+i])||(p[4+i]&&!p[8+i])||
           (p[4+i]&&((unsigned)p[4+i]<(audioChoice==1?24U:(i==3?35U:45U))||(unsigned)p[4+i]>(audioChoice==1?96U:(i==3?81U:96U)))))return fail(25);
    }
    nextState.retrigger=p[12];previousEvent=nextState.when;
    recordsRead++;bufferIndex++;nextValid=1;return 1;
}
static int openMusic(void)
{
    U8 h[MUSIC_HEADER];U32 musicDuration,length;
    if(_dos_open(audioChoice==1?"SPEAKER.WZM":"WEEZER.WZM",O_RDONLY,&mf))return fail(20);
    if(!readExact(mf,h,MUSIC_HEADER))return fail(21);
    if(memcmp(h,"WZM1",4)||wordAt(h+4)!=MUSIC_HEADER||
       wordAt(h+6)!=RECORD_SIZE||longAt(h+16))return fail(22);
    eventCount=longAt(h+8);musicDuration=longAt(h+12);
    if(!eventCount||eventCount>10000UL||!musicDuration||musicDuration>600000UL)return fail(22);
    if((mode&2)&&musicDuration!=duration)return fail(26);
    duration=musicDuration;length=(U32)lseek(mf,0L,2);
    if(length!=MUSIC_HEADER+eventCount*RECORD_SIZE)return fail(27);
    if(lseek(mf,MUSIC_HEADER,0)!=MUSIC_HEADER)return fail(23);
    return fetchEvent();
}
static int openCue(void)
{
    int f;U8 h[16];long length;
    if(_dos_open("WEEZER.CUE",O_RDONLY,&f))return fail(50);
    length=lseek(f,0L,2);
    if(length!=16L||lseek(f,0L,0)!=0L||!readExact(f,h,16)){
        _dos_close(f);return fail(51);
    }
    _dos_close(f);playStart=longAt(h+8);playEnd=longAt(h+12);
    if(memcmp(h,"WZC1",4)||longAt(h+4)!=duration||
       playStart>=playEnd||playEnd>duration)return fail(52);
    if(fullRange){playStart=0;playEnd=duration;}
    if(chorus){
        if(duration<87174UL)return fail(52);
        playStart=63395UL;playEnd=87174UL;
    }
    return 1;
}
static int primeMusic(void)
{
    if(!(mode&1))return 1;
    while(nextValid&&nextState.when<=playStart){
        seed=nextState;seedValid=1;prerolled++;if(!fetchEvent())return 0;
    }
    return 1;
}
/* Single producer/consumer ring: foreground publishes complete slots;
 * interrupt alone consumes them. No file, DOS, BIOS, or heap in callback. */
static void audioTick(void *context)
{
    unsigned count=0, head;
    U32 late;
    (void)context;
    audioNow+=8;
    audioFraction+=397544UL;
    if(audioFraction>=1193182UL){audioFraction-=1193182UL;++audioNow;}
    if(audioDone)return;
    if(audioNow>=playEnd){audio->mute();audioDone=1;return;}
    if(!(mode&1))return;
    while((head=qhead)!=qtail && queue[head].when<=audioNow && count<8){
        late=audioNow-queue[head].when;
        if(late>maxLate)maxLate=late;
        totalLate+=late;applied++;
        audio->state(&queue[head]);
        if(traceCount<1024){
            trace[traceCount].scheduled=queue[head].when;
            trace[traceCount].actual=audioNow;
            trace[traceCount].note=queue[head].note[1];
            trace[traceCount].mask=audio_mask;++traceCount;
        }
        qhead=(head+1)&(QUEUE_SIZE-1);++count;dry=0;
    }
    if(qhead==qtail&&!producerDone&&!dry){
        audio->mute();++underruns;dry=1;
    }
}
static int musicService(U32 now)
{
    unsigned tail;
    U32 gap=now-lastService;
    if(gap>maxGap)maxGap=gap;
    lastService=now;
    if(!(mode&1))return 1;
    while(nextValid){
        tail=qtail;
        if(((tail+1)&(QUEUE_SIZE-1))==qhead)break;
        queue[tail]=nextState;
        /* The final state is validated already. Publish EOF before its tail
         * so an IRQ draining that slot cannot mistake EOF for starvation. */
        if(recordsRead==eventCount)producerDone=1;
        qtail=(tail+1)&(QUEUE_SIZE-1);
        if(!fetchEvent())return 0;
    }
    if(!nextValid)producerDone=1;
    return 1;
}
static void initCga(void)
{
    /* CGA palette 1 high intensity: black, cyan, magenta, white. */
    static U8 map[16]={0,1,1,1,2,2,2,3,0,1,1,1,2,2,3,3};
    unsigned i;
    union REGS r;
    for(i=0;i<256;i++)cgaMap[i]=(map[i>>4]<<2)|map[i&15];
    r.h.ah=0x0b;r.h.bh=0;r.h.bl=16;int86(0x10,&r,&r);
    r.h.ah=0x0b;r.h.bh=1;r.h.bl=1;int86(0x10,&r,&r);
}
static void textFrame(void)
{
    static char shades[]=" .,:;ox%#@";
    static U8 light[16]={0,1,3,4,2,3,4,6,3,4,6,7,5,6,8,9};
    unsigned x,y,sx,sy,w=width/2,h=height/4,n,base;
    U8 pixel;
    U8 __far *screen=(U8 __far *)MK_FP(graphics==7?0xb000:0xb800,0);
    for(y=0;y<h;y++){
        sy=y*4;base=((25-h)/2+y)*160+(80-w);
        for(x=0;x<w;x++){
            sx=x*2;pixel=pixels[sy*rowBytes+sx/2];n=pixel>>4;
            screen[base+x*2]=shades[light[n]];
            screen[base+x*2+1]=(graphics==7)?7:(n?n:8);
        }
    }
}
static int videoService(U32 now)
{
    U32 frame,t,spent,offset;unsigned y,screenY,bankOffset;
    if(!(mode&2))return 1;
    frame=now*fps/1000UL;if(frame>=frames)frame=frames-1;
    if(frame==lastFrame)return 1;
    if(lastFrame!=0xffffffffUL&&frame>lastFrame+1)dropped+=frame-lastFrame-1;
    t=ClockTick();offset=VIDEO_HEADER+frame*(U32)frameBytes;
    if(lseek(vf,(long)offset,0)!=(long)offset||!readExact(vf,pixels,frameBytes))return fail(15);
    spent=since(t,ClockTick());if(spent>maxVideoRead)maxVideoRead=spent;
    now=elapsed();if(now>=playEnd)return 1;
    if(!musicService(now))return 0;
    t=ClockTick();
    if(graphics==3||graphics==7)textFrame();
    else for(y=0;y<height;y++){
        unsigned x;
        screenY=y+yTop;
        if(graphics==4){
            for(x=0;x<rowBytes;x+=2)
                row[x/2]=(cgaMap[pixels[y*rowBytes+x]]<<4)|
                          cgaMap[pixels[y*rowBytes+x+1]];
            bankOffset=(screenY&1)*8192+(screenY>>1)*80+xByte;
            VideoRow(row,bankOffset,rowBytes/2);
        }else{
            bankOffset=(screenY&3)*8192+(screenY>>2)*160+xByte;
            VideoRow(pixels+y*rowBytes,bankOffset,rowBytes);
        }
    }
    spent=since(t,ClockTick());if(spent>maxDraw)maxDraw=spent;
    lastFrame=frame;rendered++;return 1;
}
static void report(char *reason)
{
    FILE *f=fopen("DOSPLAY.LOG","w");unsigned i,mask=0;
    if(!f)return;
    mask=audio_mask;
    fprintf(f,"%s\nversion=2\nmode=%d\nerror=%d\n",reason,mode,error);
    fprintf(f,"timer_opened=%u\n",timerWasOpen);
    fprintf(f,"backend=%s\ngraphics=%d\ntimer_hz=120\nqueue_bytes=%lu\n",audio->name,graphics,queueBytes);
    fprintf(f,"timer_close=%d\nvector_restored=%d\nbacklog_max=%u\ntimer_late=%lu\nunderruns=%lu\nspeaker_gate_after_stop=%u\n",timerStatus,oldVector==newVector,backlog,timerLate,underruns,gateAfter);
    fprintf(f,"width=%u\nheight=%u\nfps=%u\nduration_ms=%lu\nelapsed_ms=%lu\n",width,height,fps,duration,elapsedEnd);
    fprintf(f,"frames=%lu\ndropped_frames=%lu\nmusic_events_applied=%lu\nmusic_events_skipped=%lu\nmusic_events_read=%lu\n",rendered,dropped,applied,skipped,recordsRead);
    fprintf(f,"music_max_late_ms=%lu\nmusic_total_late_ms=%lu\nservice_max_gap_ms=%lu\n",maxLate,totalLate,maxGap);
    fprintf(f,"video_read_max_ticks=%lu\nmusic_read_max_ticks=%lu\ndraw_max_ticks=%lu\n",maxVideoRead,maxMusicRead,maxDraw);
    fprintf(f,"play_start_ms=%lu\nplay_end_ms=%lu\nrun_elapsed_ms=%lu\nprerolled_records=%lu\nfull_range=%d\n",playStart,playEnd,elapsedEnd>=playStart?elapsedEnd-playStart:0UL,prerolled,fullRange);
    fprintf(f,"sound_mask_after_stop=%u\npsg_writes=%lu\nold_mode=%u\nrestored_mode=%u\n",mask,audio_writes,oldMode,VideoMode());
    for(i=0;i<traceCount;i++)fprintf(f,"event=%lu,%lu,%u,%u\n",trace[i].scheduled,trace[i].actual,trace[i].note,trace[i].mask);
    fprintf(f,"report_end=1\n");
    fclose(f);
}
int main(int argc,char **argv)
{
    unsigned key;U32 now;char *reason="Complete";int marker,i;FILE *markFile;union REGS r;
    for(i=1;i<argc;i++){
        if(strlen(argv[i])!=2||argv[i][0]!='/')goto usage;
        switch(argv[i][1]){
        case 'A':case 'a':mode=1;break;
        case 'V':case 'v':mode=2;break;
        case 'B':case 'b':mode=3;break;
        case 'F':case 'f':fullRange=1;break;
        case 'S':case 's':audioChoice=1;break;
        case 'T':case 't':audioChoice=0;break;
        case 'N':case 'n':audioChoice=2;break;
        case 'C':case 'c':graphics=4;break;
        case 'X':case 'x':graphics=3;break;
        case 'M':case 'm':graphics=7;break;
        case 'G':case 'g':graphics=9;break;
        case 'H':case 'h':chorus=1;break;
        default:goto usage;
        }
    }
    if(IsWindows()){puts("Exit Windows first. DOSPLAY needs exclusive DOS hardware.");return 1;}
    r.h.ah=0x30;intdos(&r,&r);if(r.h.al<3){puts("DOS 3.0 or later required.");return 1;}
    tandy=*(U8 __far *)MK_FP(0xfc00,0)==0x21;
    oldMode=VideoMode();
    if(audioChoice<0)audioChoice=tandy?0:1;
    if(graphics<0)graphics=tandy?9:(oldMode==7?7:4);
    if(!(mode&1))audioChoice=2;
    audio=audioChoice==0?&buddy_psg:(audioChoice==1?&buddy_speaker:&buddy_silent);
    haveHardware=1;audio->open();
    oldVector=(U32)_dos_getvect(8);queueBytes=sizeof(queue);
    oldBreak=_dos_getvect(0x23);oldCritical=_dos_getvect(0x24);
    _dos_setvect(0x23,onBreak);_harderr(onCritical);handlers=1;
    if(((mode&2)&&!openVideo())||((mode&1)&&!openMusic())||!openCue()||!primeMusic()){reason="Error";goto done;}
    if(mode&2){displayChanged=1;SetMode(graphics);if(VideoMode()!=(unsigned)graphics){fail(40);reason="Error";goto done;}}
    if((mode&2)&&graphics==4)initCga();
    lastService=playStart;
    if(!musicService(playStart)){reason="Error";goto done;}
    if(!_dos_creat("DOSSTART.LOG",0,&marker))_dos_close(marker);
    markFile=fopen("MARK.LOG","w");
    if(markFile){
        fprintf(markFile,"segment=%u\noffset=%u\n",
            FP_SEG((void __far *)&audioNow),
            FP_OFF((void __far *)&audioNow));
        fclose(markFile);
    }
    startTick=ClockTick();lastService=playStart;audioNow=playStart;
    if(seedValid){audio->state(&seed);applied++;}
    if(lttk_open(9943U,audioTick,0)!=LTTK_OK){
        fail(60);reason="Error";goto done;
    }
    timerOpen=1;timerWasOpen=1;lttk_run(1);
    for(;;){
        key=KeyRead();if(interrupted||key==3||key==27||key==32){reason="Stopped";break;}
        now=elapsed();
        if(now>=playEnd)break;
        if(!musicService(now)){reason="Error";break;}
        if(!videoService(now)||!musicService(elapsed())){reason="Error";break;}
    }
    elapsedEnd=elapsed();
done:
    cleanup();report(reason);restoreHandlers();printf("DOSPLAY: %s. Error %d. See DOSPLAY.LOG.\n",reason,error);
    return error?1:0;
usage:
    puts("DOSPLAY [/F full video] [/A audio | /V video | /B both]\n[/S speaker | /T PSG | /N silent] [/C CGA | /X text | /M mono | /G Tandy]\nDefault: auto device, song cue. /H chorus. Escape/Space/Ctrl+C: stop.");return 1;
}
