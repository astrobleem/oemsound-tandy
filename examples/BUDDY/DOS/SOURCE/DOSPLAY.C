/* Private WZV2/WZM1 Tandy DOS player. MSC 6, /G0, small model. */
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "PERIODS.H"
#define U32 unsigned long
#define U8 unsigned char
#define VIDEO_HEADER 24
#define MUSIC_HEADER 20
#define RECORD_SIZE 14
#define MUSIC_CHUNK 64
#define MAX_FRAME 6144
#define DAY_TICKS 0x1800b0UL
extern U32 _cdecl ClockTick(void);
extern unsigned _cdecl VideoMode(void);
extern void _cdecl SetMode(unsigned);
extern void _cdecl VideoRow(U8 *,unsigned,unsigned);
extern void _cdecl PsgByte(unsigned);
extern unsigned _cdecl KeyRead(void);
extern unsigned _cdecl IsWindows(void);
static int vf=-1,mf=-1,haveHardware,displayChanged,handlers,mode=3,error;
static unsigned oldMode,width,height,fps,frameBytes,rowBytes,xByte,yTop;
static U8 pixels[MAX_FRAME],musicBuffer[MUSIC_CHUNK*RECORD_SIZE];
static unsigned bufferCount,bufferIndex;
static U32 frames,duration,eventCount,recordsRead,previousEvent;
static U32 startTick,lastService,lastFrame=0xffffffffUL;
static U32 playStart,playEnd,prerolled;
static int fullRange;
static U32 rendered,dropped,applied,skipped,maxLate,totalLate,maxGap;
static U32 maxVideoRead,maxMusicRead,maxDraw,psgWrites,elapsedEnd;
static U8 volume[4]={15,15,15,15};
static volatile int interrupted;
static void (_interrupt _far *oldBreak)();
static void (_interrupt _far *oldCritical)();
typedef struct { U32 when; U8 note[4],velocity[4],retrigger; } STATE;
static STATE current,nextState,seed;
static int seedValid;
static int nextValid;
static unsigned wordAt(U8 *p) {return (unsigned)p[0]|((unsigned)p[1]<<8);}
static U32 longAt(U8 *p) {return (U32)wordAt(p)|((U32)wordAt(p+2)<<16);}
static U32 since(U32 first,U32 last) {return last>=first?last-first:DAY_TICKS-first+last;}
static U32 elapsed(void) {return playStart+since(startTick,ClockTick())*54925UL/1000UL;}
static void output(unsigned b) {PsgByte(b);psgWrites++;}
static void mute(void)
{
    unsigned i;if(!haveHardware)return;
    for(i=0;i<4;i++){output(0x9f|(i<<5));volume[i]=15;}
}
static int fail(int n) {error=n;return 0;}
static int readExact(int f,void *p,unsigned n)
{
    unsigned got;return !_dos_read(f,(void _far *)p,n,&got)&&got==n;
}
void _interrupt _far onBreak(void) {interrupted=1;}
void _far onCritical(unsigned deverr,unsigned errcode,unsigned _far *devhdr)
{
    (void)deverr;(void)errcode;(void)devhdr;_hardresume(_HARDERR_FAIL);
}
static void cleanup(void)
{
    mute();
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
    xByte=(320-width)/4;yTop=(200-height)/2;return 1;
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
           (p[4+i]&&((unsigned)p[4+i]<(i==3?35U:45U)||(unsigned)p[4+i]>(i==3?81U:96U))))return fail(25);
    }
    nextState.retrigger=p[12];previousEvent=nextState.when;
    recordsRead++;bufferIndex++;nextValid=1;return 1;
}
static int openMusic(void)
{
    U8 h[MUSIC_HEADER];U32 musicDuration,length;
    if(_dos_open("WEEZER.WZM",O_RDONLY,&mf))return fail(20);
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
static void soundState(STATE *s)
{
    unsigned i,n,period,atten,change;
    for(i=0;i<4;i++){
        change=current.note[i]!=s->note[i]||current.velocity[i]!=s->velocity[i]||
               (s->retrigger&(1<<i));
        if(!change)continue;
        output(0x9f|(i<<5));volume[i]=15;n=s->note[i];if(!n)continue;
        atten=14-(((unsigned)s->velocity[i]-1)*14)/126;
        if(i<3){period=periods[n-45];output(0x80|(i<<5)|(period&15));output(period>>4);}
        else {period=0xe6;if(n==35||n==36)period=0xe2;
              else if(n==38||n==40)period=0xe5;
              else if(n==42||n==44||n==46)period=0xe4;output(period);}
        output(0x90|(i<<5)|atten);volume[i]=(U8)atten;
    }
    current=*s;
}
static int musicService(U32 now)
{
    STATE pending;U32 count=0,late,gap;
    if(now>=playEnd)return 1;
    gap=now-lastService;if(gap>maxGap)maxGap=gap;lastService=now;
    if(!(mode&1))return 1;
    while(nextValid&&nextState.when<=now){pending=nextState;count++;if(!fetchEvent())return 0;}
    if(!count)return 1;
    applied++;skipped+=count-1;late=now-pending.when;totalLate+=late;if(late>maxLate)maxLate=late;
    soundState(&pending);return 1;
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
    for(y=0;y<height;y++){
        screenY=y+yTop;bankOffset=(screenY&3)*8192+(screenY>>2)*160+xByte;
        VideoRow(pixels+y*rowBytes,bankOffset,rowBytes);
    }
    spent=since(t,ClockTick());if(spent>maxDraw)maxDraw=spent;
    lastFrame=frame;rendered++;return 1;
}
static void report(char *reason)
{
    FILE *f=fopen("DOSPLAY.LOG","w");unsigned i,mask=0;
    if(!f)return;
    for(i=0;i<4;i++)if(volume[i]!=15)mask|=1<<i;
    fprintf(f,"%s\nversion=1\nmode=%d\nerror=%d\n",reason,mode,error);
    fprintf(f,"width=%u\nheight=%u\nfps=%u\nduration_ms=%lu\nelapsed_ms=%lu\n",width,height,fps,duration,elapsedEnd);
    fprintf(f,"frames=%lu\ndropped_frames=%lu\nmusic_events_applied=%lu\nmusic_events_skipped=%lu\nmusic_events_read=%lu\n",rendered,dropped,applied,skipped,recordsRead);
    fprintf(f,"music_max_late_ms=%lu\nmusic_total_late_ms=%lu\nservice_max_gap_ms=%lu\n",maxLate,totalLate,maxGap);
    fprintf(f,"video_read_max_ticks=%lu\nmusic_read_max_ticks=%lu\ndraw_max_ticks=%lu\n",maxVideoRead,maxMusicRead,maxDraw);
    fprintf(f,"play_start_ms=%lu\nplay_end_ms=%lu\nrun_elapsed_ms=%lu\nprerolled_records=%lu\nfull_range=%d\n",playStart,playEnd,elapsedEnd>=playStart?elapsedEnd-playStart:0UL,prerolled,fullRange);
    fprintf(f,"sound_mask_after_stop=%u\npsg_writes=%lu\nold_mode=%u\nrestored_mode=%u\n",mask,psgWrites,oldMode,VideoMode());
    fclose(f);
}
int main(int argc,char **argv)
{
    unsigned key;U32 now;char *reason="Complete";int marker,i;union REGS r;
    for(i=1;i<argc;i++){
        if(strlen(argv[i])!=2||argv[i][0]!='/')goto usage;
        switch(argv[i][1]){
        case 'A':case 'a':mode=1;break;
        case 'V':case 'v':mode=2;break;
        case 'B':case 'b':mode=3;break;
        case 'F':case 'f':fullRange=1;break;
        default:goto usage;
        }
    }
    if(IsWindows()){puts("Exit Windows first. DOSPLAY needs exclusive Tandy hardware.");return 1;}
    r.h.ah=0x30;intdos(&r,&r);if(r.h.al<3){puts("DOS 3.0 or later required.");return 1;}
    if(*(U8 _far *)0xfc000000UL!=0x21){puts("Tandy 1000 hardware required.");return 1;}
    oldMode=VideoMode();haveHardware=1;mute();
    oldBreak=_dos_getvect(0x23);oldCritical=_dos_getvect(0x24);
    _dos_setvect(0x23,onBreak);_harderr(onCritical);handlers=1;
    if(((mode&2)&&!openVideo())||((mode&1)&&!openMusic())||!openCue()||!primeMusic()){reason="Error";goto done;}
    if(mode&2){displayChanged=1;SetMode(9);if(VideoMode()!=9){fail(40);reason="Error";goto done;}}
    if(!_dos_creat("DOSSTART.LOG",0,&marker))_dos_close(marker);
    startTick=ClockTick();lastService=playStart;
    if(seedValid){soundState(&seed);applied++;}
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
    puts("DOSPLAY [/F full video] [/A audio | /V video | /B both]\nDefault: song-only. Escape/Space: stop; relaunch to restart.");return 1;
}
