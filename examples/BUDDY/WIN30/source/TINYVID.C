/* Streamed Buddy Holly player. 8086, Windows 3.0 real mode. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
#include <conio.h>
#include <string.h>
#include "SPEAKER.H"
#include "SPKAPI.H"
#ifdef __WATCOMC__
#define BUD_EXPORT __export
#else
#define BUD_EXPORT
#endif
#include "BQSCHED.C"
/* Supplied by the backend owner after its exact ABI is agreed/tested.
 * BuddyQueueBind adapts existing exports; it must not acquire output. */
#ifdef BUDDY_QUEUE_BRIDGE
#include "BQBRIDGE.H"
#endif

#define VIDEO_HEADER 24
#define MUSIC_HEADER 20
#define RECORD_SIZE 14
#define MUSIC_CHUNK 64
#define WIDTH 64
#define HEIGHT 48
#define FRAME_BYTES 1536
#define MAX_DURATION 600000UL
#define MAX_EVENTS 10000UL

typedef BQSTATE MUSICSTATE;

static volatile DWORD trace[16];
/* Device selection is independent of display. Further backends belong at
   these prepare/apply/clock/stop boundaries; AdLib/GM are not advertised. */
enum {SOUND_AUTO=0,SOUND_SPEAKER=1,SOUND_TANDY=2};
static int soundChoice,legacyUser,serviceOpened,serviceUsed,forceDIB;
static HINSTANCE clockModule;
static SPOPEN clockOpen;static SPLOAD clockLoad;
static SPONE clockStart,clockStop;
static SPCLOCK clockRead;static SPDEBUG clockDebug;
static SPSTATE prepared[SP_CAPACITY];
static unsigned uiFontHeight=5;
static DWORD maxVideoAge,totalVideoAge,maxVideoEarly,totalVideoEarly,videoLead;
static FARPROC windowProcThunk;
static int planarVideo,planeMap[4],bitmapBottomUp,bitmapDirect;
static unsigned bitmapRowStep,bitmapPlaneStep;
static int speaker=1,chorus,holdOpen,videoX=2,videoY=2;
static unsigned speakerNote,speakerChanges;
static DWORD speakerOn,speakerOff;
static HINSTANCE module;
static MIDIPROC midi;
static DEBUGPROC debug;
static HWND mainWindow;
static HDC memoryDC;
static HBITMAP bitmap,oldBitmap;
static HFILE videoFile=HFILE_ERROR,musicFile=HFILE_ERROR;
static int soundOpen,playing,automatic,started,finished,mode=3,lastError,fullPlayback;
static int nextValid,hadFrame;
static char directory[144],status[40]="Space: song";
static BYTE pixels[FRAME_BYTES],bottomUp[FRAME_BYTES];
static BYTE musicBuffer[MUSIC_CHUNK*RECORD_SIZE];
static unsigned bufferCount,bufferIndex;
static DWORD videoFrames,duration,eventCount,recordsRead,previousEvent;
static DWORD startTick,lastTick,lastFrame=0xffffffffUL,finishTick,timelineBase,playEnd,songStart,songEnd,lastElapsed;
static DWORD maxGap,maxLateness,totalLateness,appliedEvents,skippedEvents;
static DWORD renderedFrames,droppedFrames,maxVideoRead,maxMusicRead,maxDraw;
static DWORD musicFailures,videoFailures,cleanupError;
static MUSICSTATE current,nextState;
static BQSCHED queue;
static BQOPS queueOps;
static int queueReady,queueBlocked,paused,usedQueue,seekPaused;
static DWORD resumePosition;
static DWORD movieNow(void);
static void pausePlayback(void);
static void seekTo(DWORD target);
static struct { BITMAPINFOHEADER header; RGBQUAD color[16]; } dibInfo;

static WORD wordAt(BYTE *p) { return (WORD)p[0]|((WORD)p[1]<<8); }
static DWORD longAt(BYTE *p) { return (DWORD)wordAt(p)|((DWORD)wordAt(p+2)<<16); }
static void pathFor(char *p,char *name) { lstrcpy(p,directory);lstrcat(p,name); }
static void statistic(HFILE f,char *key,DWORD value)
{
    char row[96],digits[16];unsigned i=0,n=0;
    while(*key)row[i++]=*key++;row[i++]='=';
    do {digits[n++]=(char)('0'+(unsigned)(value%10));value/=10;}while(value);
    while(n)row[i++]=digits[--n];row[i++]='\r';row[i++]='\n';_lwrite(f,row,i);
}
static void speakerSilence(void)
{
    outp(0x61,inp(0x61)&0xfc);speakerNote=0;speakerOff++;
}
static void speakerPlay(unsigned note)
{
    unsigned divisor;
    if(note==speakerNote)return;
    speakerSilence();
    if(!note)return;
    divisor=speakerPeriods[note];
    outp(0x43,0xb6);outp(0x42,divisor&255);
    outp(0x42,divisor>>8);outp(0x61,inp(0x61)|3);
    speakerNote=note;speakerChanges++;speakerOn++;
}
static void releaseSound(void)
{
    if(serviceOpened) {
        if(clockStop()){cleanupError|=4;lastError=51;return;}
        serviceOpened=0;soundOpen=0;speakerNote=0;return;
    }
    if(speaker) {
        if(soundOpen){speakerSilence();CloseSound();}
        soundOpen=0;return;
    }
    if(soundOpen) {
        if(queueReady&&BqStop(&queue)!=BQ_OK) {
            cleanupError|=4;lastError=38;return;
        }
        if(midi(0,MODM_RESET,MIDI_COOKIE,0L,0L))cleanupError|=1;
        if(midi(0,MODM_CLOSE,MIDI_COOKIE,0L,0L)) {
            cleanupError|=2;lastError=38;return;
        }
        if(cleanupError&&!lastError)lastError=38;
        soundOpen=0;
    }
    queueReady=0;
}
static void closeStreams(void)
{
    if(videoFile!=HFILE_ERROR){_lclose(videoFile);videoFile=HFILE_ERROR;}
    if(musicFile!=HFILE_ERROR){_lclose(musicFile);musicFile=HFILE_ERROR;}
}
static void report(char *reason,DWORD elapsed)
{
    char path[160];HFILE f;pathFor(path,"TINYVID.LOG");f=_lcreat(path,0);
    if(f==HFILE_ERROR)return;
    _lwrite(f,reason,lstrlen(reason));_lwrite(f,"\r\n",2);
    statistic(f,"version",5);statistic(f,"timestamped_queue",usedQueue);statistic(f,"sound_selection",soundChoice);
    statistic(f,"system_timer_service",serviceUsed);
    if(serviceUsed&&clockDebug){statistic(f,"sound_timer_backend",clockDebug(10));
        statistic(f,"broker_hook_fault",clockDebug(18));
        statistic(f,"broker_hooks_after_stop",clockDebug(19));
        statistic(f,"broker_dos_suspends",clockDebug(20));
        statistic(f,"system_events",clockDebug(3));
        statistic(f,"system_max_late_ms",clockDebug(4));
        statistic(f,"system_max_gap_ms",clockDebug(5));
        statistic(f,"system_owner_after_stop",clockDebug(0));
        statistic(f,"system_timer_after_stop",clockDebug(1));
        statistic(f,"system_finished",clockDebug(7));
        statistic(f,"system_irq0_unchanged",clockDebug(8));}
    statistic(f,"pc_speaker",speaker);statistic(f,"native_planar",planarVideo);statistic(f,"direct_bitmap",planarVideo==1&&bitmapDirect);
    statistic(f,"speaker_changes",serviceUsed?clockDebug(15):speakerChanges);
    statistic(f,"speaker_on",serviceUsed?clockDebug(16):speakerOn);statistic(f,"speaker_off",serviceUsed?clockDebug(17):speakerOff);
    statistic(f,"speaker_gate_after_stop",speaker?(inp(0x61)&3):65535);statistic(f,"mode",mode);
    statistic(f,"duration_ms",duration);statistic(f,"elapsed_ms",elapsed);
    statistic(f,"clip_start_ms",timelineBase);statistic(f,"clip_end_ms",playEnd);
    statistic(f,"music_events_applied",serviceUsed?clockDebug(3):appliedEvents);
    statistic(f,"music_events_skipped",serviceUsed?clockDebug(12):skippedEvents);
    statistic(f,"music_events_read",recordsRead);
    statistic(f,"music_max_late_ms",serviceUsed?clockDebug(4):maxLateness);
    statistic(f,"music_total_late_ms",serviceUsed?((DWORD)clockDebug(14)<<16)|clockDebug(13):totalLateness);
    statistic(f,"timer_max_gap_ms",maxGap);
    statistic(f,"frames",renderedFrames);statistic(f,"dropped_frames",droppedFrames);
    statistic(f,"video_read_max_ms",maxVideoRead);
    statistic(f,"music_read_max_ms",maxMusicRead);statistic(f,"draw_max_ms",maxDraw);statistic(f,"video_max_age_ms",maxVideoAge);statistic(f,"video_total_age_ms",totalVideoAge);statistic(f,"video_max_early_ms",maxVideoEarly);statistic(f,"video_total_early_ms",totalVideoEarly);statistic(f,"video_lead_ms",videoLead);
    statistic(f,"sound_mask_after_stop",debug?debug(0):65535);
    statistic(f,"owner_after_stop",debug?debug(1):65535);
    statistic(f,"psg_lease_after_stop",debug?debug(15):65535);
    statistic(f,"psg_release_error",debug?debug(17):65535);
    statistic(f,"error",lastError);statistic(f,"cleanup_error_bits",cleanupError);
    statistic(f,"video_errors",videoFailures);
    statistic(f,"music_errors",musicFailures);_lclose(f);
}
static void stop(char *reason)
{
    DWORD position=playing?movieNow():timelineBase+lastElapsed;
    DWORD elapsed=position>=timelineBase?position-timelineBase:0L;
    if(elapsed>playEnd-timelineBase)elapsed=playEnd-timelineBase;
    lastElapsed=elapsed;playing=paused=0;trace[1]=2;trace[3]=elapsed;
    releaseSound();
    trace[4]=lastFrame;trace[5]=renderedFrames;trace[6]=droppedFrames;
    trace[10]=serviceUsed?clockDebug(3):appliedEvents;
    trace[11]=serviceUsed?clockDebug(12):skippedEvents;
    trace[12]=speaker?(inp(0x61)&3):0;
    closeStreams();finished=1;finishTick=GetTickCount();
    lstrcpy(status,cleanupError?"Cleanup error":reason);report(reason,elapsed);InvalidateRect(mainWindow,NULL,TRUE);
}
static int fail(int code,int music)
{
    lastError=code;if(music)musicFailures++;else videoFailures++;return 0;
}
static int nativePlanar(void)
{
    BITMAP bm;unsigned i,k,c,row,j,base,src,layout,flip;
    int copied,got,good,found,used;char path[160];HFILE f;
    static BYTE pattern[4][2]={{15,15},{51,51},{85,85},{0,255}};
    copied=GetObject(bitmap,sizeof(bm),(LPSTR)&bm);
    PatBlt(memoryDC,0,0,WIDTH,HEIGHT,BLACKNESS);
    /* RGBI brown is a monitor substitution for digital dark yellow.
       SetPixel's RGB threshold maps (170,85,0) to red on CGA100; use
       digital RGB levels so the probe tests all sixteen native indices. */
    for(i=0;i<16;i++)SetPixel(memoryDC,i,0,
        RGB((i&4?170:0)+(i&8?85:0),(i&2?170:0)+(i&8?85:0),
            (i&1?170:0)+(i&8?85:0)));
    SelectObject(memoryDC,oldBitmap);
    got=(int)GetBitmapBits(bitmap,FRAME_BYTES,pixels);
    SelectObject(memoryDC,bitmap);
    pathFor(path,"BDINFO.LOG");f=_lcreat(path,0);
    if(f!=HFILE_ERROR){statistic(f,"object_bytes",copied);
        statistic(f,"planes",bm.bmPlanes);
        statistic(f,"bits_pixel",bm.bmBitsPixel);
        statistic(f,"width_bytes",bm.bmWidthBytes);
        statistic(f,"bitmap_bytes",got);_lclose(f);}
    pathFor(path,"BDPROBE.BIN");f=_lcreat(path,0);
    if(f!=HFILE_ERROR){_lwrite(f,pixels,FRAME_BYTES);_lclose(f);}
    PatBlt(memoryDC,0,0,WIDTH,HEIGHT,BLACKNESS);
    if(copied<10||got!=FRAME_BYTES)return 0;
    /* Some Win16 GDI implementations expose packed DDB bits. */
    if(bm.bmPlanes==1&&bm.bmBitsPixel==4&&bm.bmWidthBytes==32) {
        for(flip=0;flip<2;flip++) {
            good=1;base=flip?(HEIGHT-1)*32:0;
            for(i=0;i<FRAME_BYTES;i++) {
                unsigned value=0;
                if(i>=base&&i<base+8)value=(i-base)*34+1;
                if(pixels[i]!=value)good=0;
            }
            if(good){bitmapBottomUp=flip;return 2;}
        }
    }
    if(bm.bmPlanes!=4||bm.bmBitsPixel!=1||bm.bmWidthBytes!=8)return 0;
    /* Detect row/plane interleaving, row direction and RGBI plane order. */
    for(layout=0;layout<2;layout++)for(flip=0;flip<2;flip++) {
        bitmapRowStep=layout?8:32;
        bitmapPlaneStep=layout?HEIGHT*8:8;used=0;good=1;
        for(k=0;k<4;k++) {
            found=-1;
            for(c=0;c<4;c++) {
                int match=1;
                for(row=0;row<HEIGHT;row++)for(j=0;j<8;j++) {
                    BYTE value=0;
                    if(row==(flip?HEIGHT-1:0)&&j<2)value=pattern[c][j];
                    src=k*bitmapPlaneStep+row*bitmapRowStep+j;
                    if(pixels[src]!=value)match=0;
                }
                if(match)found=c;
            }
            if(found<0||(used&(1<<found)))good=0;
            else {planeMap[found]=k;used|=1<<found;}
        }
        if(good){bitmapBottomUp=flip;
            bitmapDirect=!flip&&bitmapRowStep==32&&bitmapPlaneStep==8;
            for(c=0;c<4;c++)if(planeMap[c]!=c)bitmapDirect=0;
            return 1;}
    }
    return 0;
}
static int openVideo(void)
{
    char path[160];BYTE h[VIDEO_HEADER];DWORD length,expected;
    planarVideo=forceDIB?0:nativePlanar();
    pathFor(path,planarVideo==1?"WEEZER.WZP":"WEEZER.WZV");
    videoFile=_lopen(path,OF_READ);
    if(planarVideo&&videoFile==HFILE_ERROR){planarVideo=0;
        pathFor(path,"WEEZER.WZV");videoFile=_lopen(path,OF_READ);}
    if(videoFile==HFILE_ERROR)return fail(10,0);
    if(_lread(videoFile,h,VIDEO_HEADER)!=VIDEO_HEADER)return fail(11,0);
    if(h[0]!='W'||h[1]!='Z'||h[2]!=(planarVideo==1?'P':'V')||
       h[3]!=(planarVideo==1?'1':'2')||wordAt(h+4)!=WIDTH||
       wordAt(h+6)!=HEIGHT||wordAt(h+8)!=4||wordAt(h+10)!=VIDEO_HEADER||
       longAt(h+20)!=FRAME_BYTES)return fail(12,0);
    videoFrames=longAt(h+12);duration=longAt(h+16);
    if(!videoFrames||videoFrames>2400UL||!duration||duration>MAX_DURATION||
       duration>videoFrames*250UL||duration<=(videoFrames-1)*250UL)return fail(13,0);
    expected=VIDEO_HEADER+videoFrames*(DWORD)FRAME_BYTES;
    length=(DWORD)_llseek(videoFile,0L,2);
    if(length!=expected)return fail(14,0);
    return 1;
}
static int fetchEvent(void)
{
    BYTE *p;unsigned i,wanted;DWORD n,t,spent;
    if(recordsRead==eventCount){nextValid=0;return 1;}
    if(bufferIndex==bufferCount) {
        n=eventCount-recordsRead;if(n>MUSIC_CHUNK)n=MUSIC_CHUNK;
        wanted=(unsigned)n*RECORD_SIZE;t=GetTickCount();
        if(_lread(musicFile,musicBuffer,wanted)!=wanted)return fail(23,1);
        spent=GetTickCount()-t;if(spent>maxMusicRead)maxMusicRead=spent;
        bufferCount=(unsigned)n;bufferIndex=0;
    }
    p=musicBuffer+bufferIndex*RECORD_SIZE;
    nextState.when=longAt(p);
    if(nextState.when>duration||(recordsRead&&nextState.when<=previousEvent)||
       (p[12]&0xf0)||p[13])return fail(24,1);
    for(i=0;i<4;i++) {
        nextState.note[i]=p[4+i];nextState.velocity[i]=p[8+i];
        if(p[8+i]>127||(!p[4+i]&&p[8+i])||(p[4+i]&&!p[8+i])||
           (p[4+i]&&((unsigned)p[4+i]<(speaker?24U:(i==3?35U:45U))||(unsigned)p[4+i]>(i==3?81U:96U))))
            return fail(25,1);
    }
    nextState.retrigger=p[12];previousEvent=nextState.when;
    recordsRead++;bufferIndex++;nextValid=1;return 1;
}
static int openMusic(void)
{
    char path[160];BYTE h[MUSIC_HEADER];DWORD musicDuration,length;
    pathFor(path,speaker?"SPEAKER.WZM":"WEEZER.WZM");musicFile=_lopen(path,OF_READ);
    if(musicFile==HFILE_ERROR)return fail(20,1);
    if(_lread(musicFile,h,MUSIC_HEADER)!=MUSIC_HEADER)return fail(21,1);
    if(h[0]!='W'||h[1]!='Z'||h[2]!='M'||h[3]!='1'||wordAt(h+4)!=MUSIC_HEADER||
       wordAt(h+6)!=RECORD_SIZE||longAt(h+16))return fail(22,1);
    eventCount=longAt(h+8);musicDuration=longAt(h+12);
    if(!eventCount||eventCount>MAX_EVENTS||!musicDuration||musicDuration>MAX_DURATION)
        return fail(22,1);
    if((mode&2)&&musicDuration!=duration)return fail(26,1);
    duration=musicDuration;length=(DWORD)_llseek(musicFile,0L,2);
    if(length!=MUSIC_HEADER+eventCount*RECORD_SIZE)return fail(27,1);
    if(_llseek(musicFile,MUSIC_HEADER,0)==-1L)return fail(23,1);
    /* Validate the entire bounded stream before acquiring any output. */
    if(!fetchEvent())return 0;
    while(nextValid)if(!fetchEvent())return 0;
    recordsRead=previousEvent=0;bufferCount=bufferIndex=0;
    if(_llseek(musicFile,MUSIC_HEADER,0)==-1L)return fail(23,1);
    return fetchEvent();
}
static int openCues(void)
{
    char path[160];BYTE h[16];HFILE f;unsigned got;LONG length;
    pathFor(path,"WEEZER.CUE");f=_lopen(path,OF_READ);
    if(f==HFILE_ERROR)return fail(35,1);
    got=_lread(f,h,16);length=_llseek(f,0L,2);_lclose(f);
    if(got!=16||length!=16L||h[0]!='W'||h[1]!='Z'||h[2]!='C'||h[3]!='1'||
       longAt(h+4)!=duration)return fail(36,1);
    songStart=longAt(h+8);songEnd=longAt(h+12);
    if(songStart>=songEnd||songEnd>duration)return fail(37,1);
    timelineBase=fullPlayback?0L:songStart;playEnd=fullPlayback?duration:songEnd;return 1;
}
static int acquireSound(void)
{
    TOPEN description;TCAPS caps;DWORD cookie=0;
    if(speaker) {
        if(clockModule<32&&!legacyUser)return fail(47,1);
        if(clockModule>=32&&!legacyUser) {
            if(clockOpen())return fail(47,1);
            serviceOpened=serviceUsed=1;
        } else {if(OpenSound()<0)return fail(46,1);StopSound();speakerSilence();}
        soundOpen=1;return 1;
    }
    if(!midi)return fail(30,1);
    if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||
       caps.wVoices!=3||caps.vDriverVersion<0x0102)return fail(31,1);
    description.hMidi=0;description.dwCallback=0L;
    description.dwInstance=soundChoice==SOUND_TANDY?0x30475350UL:0L;
    if(midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&description,0L))
        return fail(32,1);
    soundOpen=1;if(cookie!=MIDI_COOKIE)return fail(33,1);
#ifdef BUDDY_QUEUE_BRIDGE
    if(!BuddyQueueBind(module,&queueOps))return fail(45,1);
    queueReady=1;
#endif
    return 1;
}
static int applyState(MUSICSTATE *pending)
{
    DWORD messages[BQ_MAXMSG];unsigned i,n;
    if(speaker){speakerPlay(pending->note[1]);current=*pending;return 1;}
    n=BqMessages(&current,pending,messages);
    for(i=0;i<n;i++)
        if(midi(0,MODM_DATA,MIDI_COOKIE,messages[i],0L))
            return fail(34,1);
    current=*pending;return 1;
}
/* Reconstruct only current state, then leave the next future snapshot ready.
 * The caller must acknowledge reset before any reconstructed note is sent. */
static int musicAt(DWORD target,MUSICSTATE *seed)
{
    DWORD lo=0,hi=eventCount,mid;BYTE bytes[4];unsigned i;
    for(i=0;i<4;i++)seed->note[i]=seed->velocity[i]=0;
    seed->when=target;seed->retrigger=15;
    while(lo<hi) {
        mid=lo+(hi-lo)/2;
        if(_llseek(musicFile,MUSIC_HEADER+mid*RECORD_SIZE,0)==-1L||
           _lread(musicFile,bytes,4)!=4)return fail(23,1);
        if(longAt(bytes)<=target)lo=mid+1;else hi=mid;
    }
    recordsRead=lo?lo-1:0;previousEvent=0;
    bufferCount=bufferIndex=0;nextValid=0;
    if(_llseek(musicFile,MUSIC_HEADER+recordsRead*RECORD_SIZE,0)==-1L||
       !fetchEvent())return fail(23,1);
    if(lo) {
        *seed=nextState;seed->when=target;seed->retrigger=15;
        if(!fetchEvent())return 0;
    }
    return 1;
}
static DWORD movieNow(void)
{
    DWORD position;
    if(!playing)return timelineBase;
    if(queueReady) {
        if(BqPosition(&queue,&position)!=BQ_OK) {
            fail(46,1);return timelineBase+lastElapsed;
        }
        return position;
    }
    position=serviceOpened?clockRead():GetTickCount()-startTick;
    if(serviceOpened&&clockDebug(7)==2)fail(52,1);
    if(position>=playEnd-timelineBase)return playEnd;
    return timelineBase+position;
}
static int updateMusic(DWORD elapsed)
{
    unsigned count=0;DWORD late;int result;
    queueBlocked=0;
    if(!(mode&1)||serviceOpened)return 1;
    if(queueReady) {
        result=BqSeed(&queue);
        if(result==BQ_ERROR)return fail(46,1);
        if(result==BQ_FULL){queueBlocked=1;return 1;}
        while(nextValid&&nextState.when<=playEnd&&count<16) {
            result=BqOffer(&queue,&nextState);
            if(result==BQ_ERROR)return fail(46,1);
            if(result==BQ_LATER)return 1;
            if(result==BQ_FULL){queueBlocked=1;return 1;}
            appliedEvents++;count++;
            if(!fetchEvent())return 0;
        }
        if(nextValid&&nextState.when<=playEnd) {
            queueBlocked=1;return 1;
        }
        result=BqEnd(&queue);
        if(result==BQ_ERROR)return fail(46,1);
        if(result==BQ_FULL)queueBlocked=1;
        if(result==BQ_DONE)finished=1;
        return 1;
    }
    /* Bound foreground work, preserving every attack in source order. */
    while(nextValid&&nextState.when<=elapsed&&count<64) {
        late=elapsed-nextState.when;totalLateness+=late;
        if(late>maxLateness)maxLateness=late;
        if(!applyState(&nextState))return 0;
        appliedEvents++;count++;
        if(!fetchEvent())return 0;
    }
    queueBlocked=nextValid&&nextState.when<=elapsed;
    return 1;
}
static int updateVideo(DWORD elapsed)
{
    DWORD frame,t,spent,offset,workStart,target;LONG age;unsigned x,y,p;HDC dc;int ok;
    if(!(mode&2))return 1;
    /* Predict completion of the measured read/upload/blit work. Audio is
       the clock; preparation of a frame must start before its display time. */
    target=elapsed+videoLead;if(target>=playEnd)target=playEnd-1;
    frame=target/250UL;if(frame>=videoFrames)frame=videoFrames-1;
    if(lastFrame!=0xffffffffUL&&frame<=lastFrame)return 1;
    if(lastFrame!=0xffffffffUL&&frame>lastFrame+1)droppedFrames+=frame-lastFrame-1;
    workStart=movieNow();t=GetTickCount();offset=VIDEO_HEADER+frame*(DWORD)FRAME_BYTES;
    if(((lastFrame==0xffffffffUL||frame!=lastFrame+1)&&_llseek(videoFile,(LONG)offset,0)!=(LONG)offset)||
       _lread(videoFile,pixels,FRAME_BYTES)!=FRAME_BYTES)return fail(15,0);
    spent=GetTickCount()-t;if(spent>maxVideoRead)maxVideoRead=spent;
    if(!planarVideo)for(y=0;y<HEIGHT;y++)for(x=0;x<WIDTH/2;x++)
        bottomUp[(HEIGHT-1-y)*(WIDTH/2)+x]=pixels[y*(WIDTH/2)+x];
    t=GetTickCount();dc=GetDC(mainWindow);if(!dc)return fail(40,0);
    if(SelectObject(memoryDC,oldBitmap)!=bitmap){ReleaseDC(mainWindow,dc);return fail(42,0);}
    if(planarVideo) {
        if(planarVideo==1&&!bitmapDirect)for(y=0;y<HEIGHT;y++)for(p=0;p<4;p++)
            memcpy(bottomUp+(bitmapBottomUp?HEIGHT-1-y:y)*bitmapRowStep+
                   planeMap[p]*bitmapPlaneStep,pixels+y*32+p*8,8);
        else if(bitmapBottomUp)for(y=0;y<HEIGHT;y++)
            memcpy(bottomUp+(HEIGHT-1-y)*32,pixels+y*32,32);
        ok=SetBitmapBits(bitmap,FRAME_BYTES,
            (planarVideo==1&&!bitmapDirect)||bitmapBottomUp?bottomUp:pixels)==FRAME_BYTES?HEIGHT:0;
    }
    else ok=SetDIBits(dc,bitmap,0,HEIGHT,bottomUp,
                   (BITMAPINFO FAR *)&dibInfo,DIB_RGB_COLORS);
    if(SelectObject(memoryDC,bitmap)!=oldBitmap){ReleaseDC(mainWindow,dc);return fail(43,0);}
    if(ok!=HEIGHT){ReleaseDC(mainWindow,dc);return fail(44,0);}
    ok=BitBlt(dc,videoX,videoY,WIDTH,HEIGHT,memoryDC,0,0,SRCCOPY);
    ReleaseDC(mainWindow,dc);if(!ok)return fail(41,0);
    spent=GetTickCount()-t;if(spent>maxDraw)maxDraw=spent;
    spent=movieNow()-frame*250UL;
    age=(LONG)spent;
    if(age>=0){if(spent>maxVideoAge)maxVideoAge=spent;totalVideoAge+=spent;}
    else {spent=(DWORD)(-age);if(spent>maxVideoEarly)maxVideoEarly=spent;totalVideoEarly+=spent;}
    spent=movieNow()-workStart;if(spent>500UL)spent=500UL;
    videoLead=videoLead?(videoLead*3UL+spent)/4UL:spent;
    lastFrame=frame;hadFrame=1;renderedFrames++;return 1;
}
static int prepareSpeaker(MUSICSTATE *seed)
{
    WORD n=1;DWORD at;
    prepared[0].at=0;
    prepared[0].divisor=seed->note[1]?speakerPeriods[seed->note[1]]:0;
    while(nextValid) {
        at=nextState.when;
        if(at<playEnd) {
            if(n>=SP_CAPACITY-1)return fail(48,1);
            prepared[n].at=at-timelineBase;
            prepared[n++].divisor=nextState.note[1]?
                speakerPeriods[nextState.note[1]]:0;
        }
        if(!fetchEvent())return 0;
    }
    prepared[n].at=playEnd-timelineBase;prepared[n++].divisor=0;
    if(clockLoad(prepared,n,playEnd-timelineBase))return fail(49,1);
    _lclose(musicFile);musicFile=HFILE_ERROR;return 1;
}
static void play(int full)
{
    unsigned i;HFILE marker;char path[160];MUSICSTATE seed;
    DWORD target=resumePosition;int resume=paused;
    if(playing)return;
    fullPlayback=full;releaseSound();
    if(soundOpen){lstrcpy(status,"Cleanup error");return;}
    closeStreams();lastError=0;
    finished=paused=queueBlocked=usedQueue=0;serviceUsed=0;
    maxVideoAge=totalVideoAge=maxVideoEarly=totalVideoEarly=videoLead=0;
    maxGap=maxLateness=totalLateness=appliedEvents=skippedEvents=0;
    renderedFrames=droppedFrames=maxVideoRead=maxMusicRead=maxDraw=0;
    musicFailures=videoFailures=cleanupError=0;recordsRead=previousEvent=0;
    bufferCount=bufferIndex=0;nextValid=0;lastFrame=0xffffffffUL;
    duration=videoFrames=eventCount=timelineBase=playEnd=lastElapsed=0;
    for(i=0;i<4;i++){current.note[i]=0;current.velocity[i]=0;}
    if(((mode&2)&&!openVideo())||((mode&1)&&!openMusic())||!openCues()||
       ((mode&1)&&!seekPaused&&!acquireSound())) {
        stop("Error: see log");return;
    }
    if(!resume||target<timelineBase||target>=playEnd)target=timelineBase;
    timelineBase=target;
    usedQueue=queueReady;
    if(mode&1) {
        if(!musicAt(target,&seed)) {stop("Seek/music error");return;}
        if(serviceOpened&&!prepareSpeaker(&seed)){stop("Score error");return;}
    }
    if(seekPaused) {
        startTick=GetTickCount();
        if(!updateVideo(target)){stop("Video error");return;}
        closeStreams();playing=0;paused=1;resumePosition=target;lastElapsed=0;
        trace[1]=2;trace[3]=0;trace[14]=target;trace[15]=playEnd;
        lstrcpy(status,"Paused: Space");report("Paused seek",0L);
        InvalidateRect(mainWindow,NULL,TRUE);return;
    }
    /* Complete the expensive first bitmap and window repaint before the
       audio epoch. On an XT, UpdateWindow alone can exceed several notes. */
    lstrcpy(status,"Space: pause");
    if(!updateVideo(target)){stop("Video error");return;}
    InvalidateRect(mainWindow,NULL,TRUE);UpdateWindow(mainWindow);
    if(GetActiveWindow()!=mainWindow||GetFocus()!=mainWindow||IsIconic(mainWindow)) {
        releaseSound();closeStreams();paused=1;resumePosition=target;
        lstrcpy(status,"Paused: Space");report("Paused during load",0L);return;
    }
    pathFor(path,"TVSTART.LOG");marker=_lcreat(path,0);if(marker!=HFILE_ERROR)_lclose(marker);
    startTick=lastTick=GetTickCount();
    if((mode&1)&&!serviceOpened) {
        if(queueReady) {
            if(BqStart(&queue,&queueOps,target,playEnd,&seed)!=BQ_OK) {
                fail(46,1);stop("Queue start error");return;
            }
        } else if((!speaker&&midi(0,MODM_RESET,MIDI_COOKIE,0L,0L))||
                  !applyState(&seed)) {
            fail(34,1);stop("Seek/music error");return;
        }
    }
    playing=1;
    if(serviceOpened&&clockStart()){fail(50,1);stop("Clock error");return;}
    trace[0]=0x54435633UL;trace[1]=1;trace[2]=startTick;
    trace[14]=timelineBase;trace[15]=playEnd;
    if(!updateMusic(movieNow())){stop("Queue/media error");return;}

}
static void pausePlayback(void)
{
    DWORD position;
    if(!playing)return;
    position=movieNow();
    if(queueReady&&BqPause(&queue,&position)!=BQ_OK) {
        fail(46,1);stop("Pause flush error");return;
    }
    /* stop closes the same owner after the acknowledged queue flush. */
    if(queueReady)queueReady=0;
    lastElapsed=position>=timelineBase?position-timelineBase:0L;
    playing=0;stop("Paused: Space");
    resumePosition=position;paused=1;finished=0;
}
static void seekTo(DWORD target)
{
    int resume=playing,full=fullPlayback;
    DWORD lower=full?0L:songStart;
    if(!playing&&!paused)return;
    if(target<lower)target=lower;
    if(target>=playEnd)target=playEnd-1;
    stop("Seeking");paused=1;resumePosition=target;
    seekPaused=!resume;play(full);seekPaused=0;
}
static void chooseSound(void);
long FAR PASCAL BUD_EXPORT WndProc(HWND window,unsigned message,WPARAM wp,LONG lp)
{
    PAINTSTRUCT paint;HDC dc;DWORD now,elapsed,gap;
    switch(message) {
    case WM_CREATE:
        mainWindow=window;if(!SetTimer(window,1,55,NULL))return -1;return 0;
    case WM_TIMER:
        if(automatic&&!started&&GetActiveWindow()==window&&GetFocus()==window&&!IsIconic(window))
            {started=1;play(fullPlayback);}
        if(playing) {
            now=GetTickCount();gap=now-lastTick;if(gap>maxGap)maxGap=gap;lastTick=now;
            elapsed=movieNow();
            if(lastError){stop("Clock error");return 0;}
            if(!updateMusic(elapsed)){stop("Media/PSG error");return 0;}
            if(queueReady?finished:
               (elapsed>=playEnd&&!queueBlocked)) {
                stop("Done: Space plays");return 0;
            }
            elapsed=movieNow();
            if(lastError){stop("Clock error");return 0;}
            if(!queueBlocked&&!updateVideo(elapsed)) {
                stop("Media/PSG error");return 0;
            }
            elapsed=movieNow();
            if(lastError){stop("Clock error");return 0;}
            if(!updateMusic(elapsed)){stop("Media/PSG error");return 0;}
            lastElapsed=elapsed-timelineBase;
        } else if(automatic&&!holdOpen&&finished&&!soundOpen&&GetTickCount()-finishTick>=2000UL)DestroyWindow(window);
        trace[3]=playing?movieNow()-timelineBase:lastElapsed;
        trace[4]=lastFrame;trace[5]=renderedFrames;trace[6]=droppedFrames;
        trace[7]=speakerNote;trace[8]=maxGap;trace[9]=maxDraw;
        trace[10]=serviceUsed?clockDebug(3):appliedEvents;
        trace[11]=serviceUsed?clockDebug(12):skippedEvents;
        trace[12]=speaker?(inp(0x61)&3):0;
        trace[13]=serviceUsed?clockDebug(15):speakerChanges;
        return 0;
    case WM_KEYDOWN:
        if(wp=='B'||wp=='P'||wp=='A') {
            if(playing||paused)stop("Device changed");
            if(soundOpen)return 0;
            soundChoice=wp=='B'?SOUND_SPEAKER:wp=='P'?SOUND_TANDY:SOUND_AUTO;
            chooseSound();lstrcpy(status,speaker?"PC: Space plays":"PSG: Space plays");
            InvalidateRect(window,NULL,TRUE);return 0;
        }
        if(wp==VK_ESCAPE){if(playing||paused)stop("Stopped: Space");}
        else if(wp==VK_SPACE) {
            if(playing)pausePlayback();
            else play(paused?fullPlayback:0);
        } else if(wp==VK_RETURN) {
            if(playing||paused)stop("Restarting full");
            paused=0;play(1);
        } else if(wp==VK_LEFT||wp==VK_RIGHT) {
            elapsed=playing?movieNow():resumePosition;
            seekTo(wp==VK_LEFT?(elapsed>5000UL?elapsed-5000UL:0L):
                   elapsed+5000UL);
        }
        return 0;
    case WM_SIZE:if(wp==SIZE_MINIMIZED&&playing)pausePlayback();return 0;
    case WM_KILLFOCUS:if(playing)pausePlayback();return 0;
    case WM_SYSKEYDOWN:if(playing)pausePlayback();break;
    case WM_ACTIVATE:if(!wp&&playing)pausePlayback();return 0;
    case WM_PAINT:
        dc=BeginPaint(window,&paint);
        if(hadFrame)BitBlt(dc,videoX,videoY,WIDTH,HEIGHT,memoryDC,0,0,SRCCOPY);
        SelectObject(dc,GetStockObject(SYSTEM_FONT));
        TextOut(dc,2,HEIGHT+4,status,lstrlen(status));
        TextOut(dc,2,HEIGHT+6+uiFontHeight,"A:Auto B:PC P:PSG",17);
        EndPaint(window,&paint);return 0;
    case WM_CLOSE:if(playing||paused)stop("Closed");
        else {releaseSound();report("Closed",lastElapsed);}
        if(!soundOpen)DestroyWindow(window);return 0;
    case WM_DESTROY:KillTimer(window,1);releaseSound();closeStreams();PostQuitMessage(0);return 0;
    }
    return DefWindowProc(window,message,wp,lp);
}
static void chooseSound(void)
{
    HFILE f;char path[160];TCAPS caps;
    int declared=0;
    if(soundChoice==SOUND_SPEAKER){speaker=1;return;}
    /* PSG_QUERY reports ownership availability, not physical hardware.
       A configured C0 card needs explicit P; never infer it from a lease API. */
    if(!(GetWinFlags()&WF_PMODE)&&*(BYTE FAR *)0xfc000000UL==0x21)declared=1;
    speaker=soundChoice==SOUND_AUTO&&!declared;
    if(speaker)return;
    if(module<32){pathFor(path,"MIDIMAP.DRV");f=_lopen(path,OF_READ);
        if(f!=HFILE_ERROR){_lclose(f);module=LoadLibrary(path);}}
    if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
        debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}
    if(!midi||midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||
       caps.wVoices!=3||caps.vDriverVersion<0x0101) {
        if(soundChoice==SOUND_AUTO)speaker=1;
    }
}
int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show)
{
    WNDCLASS wc;MSG message;HDC dc;unsigned i;UINT oldErrorMode;HFILE ready;char path[160];RECT box;TEXTMETRIC tm;DWORD textWidth;int winX,winY;
    union {LPVOID p;struct {WORD off,seg;} w;} traceAddress;
    while(*command) {
        if(*command=='P'||*command=='p')soundChoice=SOUND_TANDY;
        if(*command=='B'||*command=='b')soundChoice=SOUND_SPEAKER;
        if(*command=='U'||*command=='u')legacyUser=1;
        if(*command=='D'||*command=='d')forceDIB=1;
        if(*command=='N'||*command=='n')holdOpen=1;
        if(*command=='H'||*command=='h'){chorus=1;automatic=1;}
        if(*command=='M'||*command=='m'){mode=1;automatic=1;}
        if(*command=='V'||*command=='v'){mode=2;automatic=1;}
        if(*command=='C'||*command=='c'){mode=3;automatic=1;}
        if(*command=='F'||*command=='f'){fullPlayback=1;automatic=1;}command++;
    }
    GetModuleFileName(instance,directory,sizeof(directory));
    for(i=lstrlen(directory);i&&directory[i-1]!='\\';i--);directory[i]=0;
    if(previous){HWND other=FindWindow("TinyFull",NULL);
        if(other){ShowWindow(other,SW_RESTORE);SetActiveWindow(other);SetFocus(other);}
        return 0;}
    oldErrorMode=SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    SetErrorMode(oldErrorMode|SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    chooseSound();
    /* A configured library broker is optional. Stock SYSTEM remains the fallback. */
    pathFor(path,"SPKFAST.DLL");clockModule=LoadLibrary(path);
    if(clockModule>=32){SPONE ready=(SPONE)GetProcAddress(clockModule,"SCREADY");
        if(!ready||!ready()){FreeLibrary(clockModule);clockModule=0;}}
    if(clockModule<32){pathFor(path,"SPKCLK.DLL");clockModule=LoadLibrary(path);}
    if(clockModule>=32) {
        SPONE version=(SPONE)GetProcAddress(clockModule,"SCVERSION");
        clockOpen=(SPOPEN)GetProcAddress(clockModule,"SCOPEN");
        clockLoad=(SPLOAD)GetProcAddress(clockModule,"SCLOAD");
        clockStart=(SPONE)GetProcAddress(clockModule,"SCSTART");
        clockStop=(SPONE)GetProcAddress(clockModule,"SCSTOP");
        clockRead=(SPCLOCK)GetProcAddress(clockModule,"SCCLOCK");
        clockDebug=(SPDEBUG)GetProcAddress(clockModule,"SCDEBUG");
        if(!version||version()!=0x0100||!clockOpen||!clockLoad||!clockStart||
           !clockStop||!clockRead||!clockDebug){FreeLibrary(clockModule);clockModule=0;}
    }
    if(!previous) {
        windowProcThunk=MakeProcInstance((FARPROC)WndProc,instance);
        if(!windowProcThunk)return 1;
        wc.style=0;wc.lpfnWndProc=(WNDPROC)windowProcThunk;wc.cbClsExtra=wc.cbWndExtra=0;
        wc.hInstance=instance;wc.hIcon=NULL;wc.hCursor=LoadCursor(NULL,IDC_ARROW);
        wc.hbrBackground=GetStockObject(WHITE_BRUSH);wc.lpszMenuName=NULL;wc.lpszClassName="TinyFull";
        if(!RegisterClass(&wc)){if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return 1;}
    }
    dibInfo.header.biSize=40;dibInfo.header.biWidth=WIDTH;dibInfo.header.biHeight=HEIGHT;
    dibInfo.header.biPlanes=1;dibInfo.header.biBitCount=4;dibInfo.header.biCompression=BI_RGB;
    dibInfo.header.biSizeImage=FRAME_BYTES;dibInfo.header.biClrUsed=16;
    for(i=0;i<16;i++) {
        dibInfo.color[i].rgbBlue=(BYTE)((i&1?170:0)+(i&8?85:0));
        dibInfo.color[i].rgbGreen=(BYTE)((i&2?170:0)+(i&8?85:0));
        dibInfo.color[i].rgbRed=(BYTE)((i&4?170:0)+(i&8?85:0));
    }
    dibInfo.color[6].rgbGreen=85;
    dc=GetDC(NULL);SelectObject(dc,GetStockObject(SYSTEM_FONT));
    GetTextMetrics(dc,&tm);textWidth=GetTextExtent(dc,"A:Auto B:PC P:PSG",17);
    ReleaseDC(NULL,dc);
    box.left=box.top=0;box.right=WIDTH+4;
    if(LOWORD(textWidth)+4>box.right)box.right=LOWORD(textWidth)+4;
    uiFontHeight=tm.tmHeight;box.bottom=HEIGHT+9+tm.tmHeight*2;
    AdjustWindowRect(&box,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
    winX=GetSystemMetrics(SM_CXSCREEN)-(box.right-box.left)-2;
    winY=GetSystemMetrics(SM_CYSCREEN)-(box.bottom-box.top)-2;
    if(winX>42)winX=42;if(winX<0)winX=0;
    if(winY>12)winY=12;if(winY<0)winY=0;
    mainWindow=CreateWindow("TinyFull","Buddy Holly",
                           WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
                           winX,winY,
                           box.right-box.left,box.bottom-box.top,
                           NULL,NULL,instance,NULL);
    if(!mainWindow){if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return 2;}
    dc=GetDC(mainWindow);
    if(!dc){DestroyWindow(mainWindow);if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return 3;}
    memoryDC=CreateCompatibleDC(dc);bitmap=CreateCompatibleBitmap(dc,WIDTH,HEIGHT);
    ReleaseDC(mainWindow,dc);
    if(!memoryDC||!bitmap) {
        if(memoryDC)DeleteDC(memoryDC);if(bitmap)DeleteObject(bitmap);
        DestroyWindow(mainWindow);if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return 3;
    }
    oldBitmap=SelectObject(memoryDC,bitmap);
    if(!oldBitmap){DeleteDC(memoryDC);DeleteObject(bitmap);DestroyWindow(mainWindow);
        if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return 4;}
    ShowWindow(mainWindow,show);SetFocus(mainWindow);UpdateWindow(mainWindow);
    pathFor(path,"TVREADY.LOG");ready=_lcreat(path,0);if(ready!=HFILE_ERROR)_lclose(ready);
    pathFor(path,"TVMARK.LOG");ready=_lcreat(path,0);
    traceAddress.p=(LPVOID)trace;
    if(ready!=HFILE_ERROR){statistic(ready,"seg",traceAddress.w.seg);
        statistic(ready,"offset",traceAddress.w.off);_lclose(ready);}
    while(GetMessage(&message,NULL,0,0)){TranslateMessage(&message);DispatchMessage(&message);}
    SelectObject(memoryDC,oldBitmap);DeleteDC(memoryDC);DeleteObject(bitmap);
    releaseSound();closeStreams();if(windowProcThunk)FreeProcInstance(windowProcThunk);
    if(module>=32&&!soundOpen)FreeLibrary(module);if(clockModule>=32&&!serviceOpened)FreeLibrary(clockModule);
    SetErrorMode(oldErrorMode);return lastError;
}
