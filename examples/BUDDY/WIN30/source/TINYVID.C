/* Private full-length streamed PSG video player. MSC6 /G0, Windows 3.0. */
#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"

#define VIDEO_HEADER 24
#define MUSIC_HEADER 20
#define RECORD_SIZE 14
#define MUSIC_CHUNK 64
#define WIDTH 64
#define HEIGHT 48
#define FRAME_BYTES 1536
#define MAX_DURATION 600000UL
#define MAX_EVENTS 10000UL

typedef struct {
    DWORD when;
    BYTE note[4], velocity[4], retrigger;
} MUSICSTATE;

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
static DWORD data(unsigned channel,unsigned note,unsigned velocity,int on)
{
    return midi(0,MODM_DATA,MIDI_COOKIE,
        (DWORD)((on?0x90:0x80)|(channel==3?9:channel))|
        ((DWORD)note<<8)|((DWORD)velocity<<16),0L);
}
static void releaseSound(void)
{
    if(soundOpen) {
        if(midi(0,MODM_RESET,MIDI_COOKIE,0L,0L))cleanupError|=1;
        if(midi(0,MODM_CLOSE,MIDI_COOKIE,0L,0L))cleanupError|=2;
        if(cleanupError&&!lastError)lastError=38;
        soundOpen=0;
    }
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
    statistic(f,"version",2);statistic(f,"mode",mode);
    statistic(f,"duration_ms",duration);statistic(f,"elapsed_ms",elapsed);
    statistic(f,"clip_start_ms",timelineBase);statistic(f,"clip_end_ms",playEnd);
    statistic(f,"music_events_applied",appliedEvents);
    statistic(f,"music_events_skipped",skippedEvents);
    statistic(f,"music_events_read",recordsRead);
    statistic(f,"music_max_late_ms",maxLateness);
    statistic(f,"music_total_late_ms",totalLateness);
    statistic(f,"timer_max_gap_ms",maxGap);
    statistic(f,"frames",renderedFrames);statistic(f,"dropped_frames",droppedFrames);
    statistic(f,"video_read_max_ms",maxVideoRead);
    statistic(f,"music_read_max_ms",maxMusicRead);statistic(f,"draw_max_ms",maxDraw);
    statistic(f,"sound_mask_after_stop",debug?debug(0):65535);
    statistic(f,"owner_after_stop",debug?debug(1):65535);
    statistic(f,"error",lastError);statistic(f,"cleanup_error_bits",cleanupError);
    statistic(f,"video_errors",videoFailures);
    statistic(f,"music_errors",musicFailures);_lclose(f);
}
static void stop(char *reason)
{
    DWORD elapsed=playing?GetTickCount()-startTick:0L;
    lastElapsed=elapsed;playing=0;releaseSound();closeStreams();finished=1;finishTick=GetTickCount();
    lstrcpy(status,cleanupError?"Cleanup error":reason);report(reason,elapsed);InvalidateRect(mainWindow,NULL,TRUE);
}
static int fail(int code,int music)
{
    lastError=code;if(music)musicFailures++;else videoFailures++;return 0;
}
static int openVideo(void)
{
    char path[160];BYTE h[VIDEO_HEADER];DWORD length,expected;
    pathFor(path,"WEEZER.WZV");videoFile=_lopen(path,OF_READ);
    if(videoFile==HFILE_ERROR)return fail(10,0);
    if(_lread(videoFile,h,VIDEO_HEADER)!=VIDEO_HEADER)return fail(11,0);
    if(h[0]!='W'||h[1]!='Z'||h[2]!='V'||h[3]!='2'||wordAt(h+4)!=WIDTH||
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
           (p[4+i]&&((unsigned)p[4+i]<(i==3?35U:45U)||(unsigned)p[4+i]>(i==3?81U:96U))))
            return fail(25,1);
    }
    nextState.retrigger=p[12];previousEvent=nextState.when;
    recordsRead++;bufferIndex++;nextValid=1;return 1;
}
static int openMusic(void)
{
    char path[160];BYTE h[MUSIC_HEADER];DWORD musicDuration,length;
    pathFor(path,"WEEZER.WZM");musicFile=_lopen(path,OF_READ);
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
    if(!midi)return fail(30,1);
    if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||
       caps.wVoices!=3||caps.vDriverVersion<0x0101)return fail(31,1);
    description.hMidi=0;description.dwCallback=description.dwInstance=0L;
    if(midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&description,0L))
        return fail(32,1);
    soundOpen=1;if(cookie!=MIDI_COOKIE)return fail(33,1);return 1;
}
static int updateMusic(DWORD elapsed)
{
    MUSICSTATE pending;DWORD count=0,late;unsigned i;int change;
    if(!(mode&1))return 1;
    while(nextValid&&nextState.when<=elapsed) {
        pending=nextState;count++;
        if(!fetchEvent())return 0;
    }
    if(!count)return 1;
    appliedEvents++;skippedEvents+=count-1;
    late=elapsed-pending.when;totalLateness+=late;if(late>maxLateness)maxLateness=late;
    for(i=0;i<4;i++) {
        change=current.note[i]!=pending.note[i]||current.velocity[i]!=pending.velocity[i]||
               (pending.retrigger&(1<<i));
        if(change&&current.note[i]&&data(i,current.note[i],0,0))return fail(34,1);
    }
    for(i=0;i<4;i++) {
        change=current.note[i]!=pending.note[i]||current.velocity[i]!=pending.velocity[i]||
               (pending.retrigger&(1<<i));
        if(change&&pending.note[i]&&data(i,pending.note[i],pending.velocity[i],1))
            return fail(34,1);
    }
    current=pending;return 1;
}
static int updateVideo(DWORD elapsed)
{
    DWORD frame,t,spent,offset;unsigned x,y;HDC dc;int ok;
    if(!(mode&2))return 1;
    frame=elapsed/250UL;if(frame>=videoFrames)frame=videoFrames-1;
    if(frame==lastFrame)return 1;
    if(lastFrame!=0xffffffffUL&&frame>lastFrame+1)droppedFrames+=frame-lastFrame-1;
    t=GetTickCount();offset=VIDEO_HEADER+frame*(DWORD)FRAME_BYTES;
    if(_llseek(videoFile,(LONG)offset,0)!=(LONG)offset||
       _lread(videoFile,pixels,FRAME_BYTES)!=FRAME_BYTES)return fail(15,0);
    spent=GetTickCount()-t;if(spent>maxVideoRead)maxVideoRead=spent;
    for(y=0;y<HEIGHT;y++)for(x=0;x<WIDTH/2;x++)
        bottomUp[(HEIGHT-1-y)*(WIDTH/2)+x]=pixels[y*(WIDTH/2)+x];
    t=GetTickCount();dc=GetDC(mainWindow);if(!dc)return fail(40,0);
    if(SelectObject(memoryDC,oldBitmap)!=bitmap){ReleaseDC(mainWindow,dc);return fail(42,0);}
    ok=SetDIBits(dc,bitmap,0,HEIGHT,bottomUp,(BITMAPINFO FAR *)&dibInfo,DIB_RGB_COLORS);
    if(SelectObject(memoryDC,bitmap)!=oldBitmap){ReleaseDC(mainWindow,dc);return fail(43,0);}
    if(ok!=HEIGHT){ReleaseDC(mainWindow,dc);return fail(44,0);}
    ok=BitBlt(dc,8,8,WIDTH,HEIGHT,memoryDC,0,0,SRCCOPY);
    ReleaseDC(mainWindow,dc);if(!ok)return fail(41,0);
    spent=GetTickCount()-t;if(spent>maxDraw)maxDraw=spent;
    lastFrame=frame;hadFrame=1;renderedFrames++;return 1;
}
static void play(int full)
{
    unsigned i;HFILE marker;char path[160];
    if(playing)return;fullPlayback=full;releaseSound();closeStreams();lastError=0;finished=0;
    maxGap=maxLateness=totalLateness=appliedEvents=skippedEvents=0;
    renderedFrames=droppedFrames=maxVideoRead=maxMusicRead=maxDraw=0;
    musicFailures=videoFailures=cleanupError=0;recordsRead=previousEvent=0;
    bufferCount=bufferIndex=0;nextValid=0;lastFrame=0xffffffffUL;
    duration=videoFrames=eventCount=timelineBase=playEnd=lastElapsed=0;
    for(i=0;i<4;i++){current.note[i]=0;current.velocity[i]=0;}
    if(((mode&2)&&!openVideo())||((mode&1)&&!openMusic())||!openCues()||
       ((mode&1)&&!acquireSound())) {
        stop("Error: see log");return;
    }
    pathFor(path,"TVSTART.LOG");marker=_lcreat(path,0);if(marker!=HFILE_ERROR)_lclose(marker);
    startTick=GetTickCount();playing=1;lstrcpy(status,"Playing - Esc");
    InvalidateRect(mainWindow,NULL,TRUE);UpdateWindow(mainWindow);
    startTick=lastTick=GetTickCount();
}
static void timeText(char *text,DWORD ms)
{
    unsigned sec=(unsigned)(ms/1000UL),i=0;
    if(sec>=600)text[i++]=(char)('0'+sec/600);
    text[i++]=(char)('0'+(sec/60)%10);text[i++]=':';
    text[i++]=(char)('0'+(sec%60)/10);text[i++]=(char)('0'+sec%10);text[i]=0;
}
long FAR PASCAL WndProc(HWND window,unsigned message,WORD wp,LONG lp)
{
    PAINTSTRUCT paint;HDC dc;DWORD now,elapsed,gap;char time[16],total[8];
    switch(message) {
    case WM_CREATE:
        mainWindow=window;if(!SetTimer(window,1,55,NULL))return -1;return 0;
    case WM_TIMER:
        if(automatic&&!started&&GetActiveWindow()==window&&GetFocus()==window&&!IsIconic(window))
            {started=1;play(fullPlayback);}
        if(playing) {
            now=GetTickCount();gap=now-lastTick;if(gap>maxGap)maxGap=gap;lastTick=now;
            elapsed=now-startTick+timelineBase;
            if(elapsed>=playEnd){stop("Done: Space plays");return 0;}
            if(!updateMusic(elapsed)){stop("Media/PSG error");return 0;}
            elapsed=GetTickCount()-startTick+timelineBase;
            if(elapsed>=playEnd){stop("Done: Space plays");return 0;}
            if(!updateVideo(elapsed)){stop("Media/PSG error");return 0;}
            elapsed=GetTickCount()-startTick+timelineBase;
            if(elapsed>=playEnd){stop("Done: Space plays");return 0;}
            if(!updateMusic(elapsed)){stop("Media/PSG error");return 0;}
        } else if(automatic&&finished&&GetTickCount()-finishTick>=2000UL)DestroyWindow(window);
        return 0;
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE){if(playing)stop("Stopped: Space");}
        else if(wp==VK_SPACE){if(playing)stop("Stopped: Space");else play(0);}
        else if(wp==VK_RETURN){if(playing)stop("Restarting full");play(1);}
        return 0;
    case WM_SIZE:if(wp==SIZE_MINIMIZED&&playing)stop("Paused: Space");return 0;
    case WM_KILLFOCUS:if(playing)stop("Paused: Space");return 0;
    case WM_SYSKEYDOWN:if(playing)stop("Paused: Space");break;
    case WM_ACTIVATE:if(!wp&&playing)stop("Paused: Space");return 0;
    case WM_PAINT:
        dc=BeginPaint(window,&paint);
        if(hadFrame)BitBlt(dc,8,8,WIDTH,HEIGHT,memoryDC,0,0,SRCCOPY);
        elapsed=playing?GetTickCount()-startTick:lastElapsed;if(elapsed>duration)elapsed=duration;
        lstrcpy(time,"Length: ");timeText(total,playEnd>timelineBase?playEnd-timelineBase:0);lstrcat(time,total);
        TextOut(dc,8,62,time,lstrlen(time));TextOut(dc,8,78,status,lstrlen(status));
        TextOut(dc,8,94,playing?"Esc stops":"Enter: full video",playing?9:17);
        EndPaint(window,&paint);return 0;
    case WM_CLOSE:if(playing)stop("Closed");DestroyWindow(window);return 0;
    case WM_DESTROY:KillTimer(window,1);releaseSound();closeStreams();PostQuitMessage(0);return 0;
    }
    return DefWindowProc(window,message,wp,lp);
}
int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show)
{
    WNDCLASS wc;MSG message;HDC dc;unsigned i;UINT oldErrorMode;HFILE ready,probe;char path[160];
    while(*command) {
        if(*command=='M'||*command=='m'){mode=1;automatic=1;}
        if(*command=='V'||*command=='v'){mode=2;automatic=1;}
        if(*command=='C'||*command=='c'){mode=3;automatic=1;}
        if(*command=='F'||*command=='f'){fullPlayback=1;automatic=1;}command++;
    }
    GetModuleFileName(instance,directory,sizeof(directory));
    for(i=lstrlen(directory);i&&directory[i-1]!='\\';i--);directory[i]=0;
    oldErrorMode=SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    SetErrorMode(oldErrorMode|SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    pathFor(path,"MIDIMAP.DRV");probe=_lopen(path,OF_READ);
    if(probe!=HFILE_ERROR){_lclose(probe);module=LoadLibrary(path);}
    if(module>=32) {
        midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
        debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    }
    if(!previous) {
        wc.style=0;wc.lpfnWndProc=WndProc;wc.cbClsExtra=wc.cbWndExtra=0;
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
    mainWindow=CreateWindow("TinyFull","Buddy Holly PSG",WS_OVERLAPPEDWINDOW,
                           0,0,GetSystemMetrics(SM_CXSCREEN)<196?GetSystemMetrics(SM_CXSCREEN):196,142,NULL,NULL,instance,NULL);
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
    while(GetMessage(&message,NULL,0,0)){TranslateMessage(&message);DispatchMessage(&message);}
    SelectObject(memoryDC,oldBitmap);DeleteDC(memoryDC);DeleteObject(bitmap);
    releaseSound();closeStreams();if(module>=32)FreeLibrary(module);SetErrorMode(oldErrorMode);return lastError;
}
