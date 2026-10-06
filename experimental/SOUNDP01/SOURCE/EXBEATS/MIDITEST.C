#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
static MIDIPROC midi;
static DEBUGPROC debug;
static HFILE log;
static int failures,checks;
static TOPEN desc;
static DWORD cookie;
static void result(char *name,DWORD value,DWORD expected)
{
    char s[96];++checks;
    wsprintf(s,"%s got=%lu want=%lu\r\n",(LPSTR)name,value,expected);
    _lwrite(log,s,lstrlen(s));if(value!=expected)++failures;
}
static DWORD call(WORD m,DWORD p)
{
    return midi(0,m,MIDI_COOKIE,p,0L);
}
static DWORD data(WORD s,WORD a,WORD b)
{
    return call(MODM_DATA,(DWORD)s|((DWORD)a<<8)|((DWORD)b<<16));
}
static DWORD openDriver(void)
{
    cookie=0;return midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,
        (DWORD)(LPVOID)&desc,0L);
}
static void waitms(WORD duration)
{
    DWORD start=GetTickCount();while(GetTickCount()-start<duration)Yield();
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
    HMODULE module;WORD i,initial;int soundVoices;HFILE f;
    struct {TCAPS caps;BYTE guard[8];} bounded;
    log=_lcreat("C:\\MIDITEST.LOG",0);
    module=LoadLibrary("C:\\MIDIMAP.DRV");result("Load driver",module>=32,1);
    if(module<32)goto done;
    midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
    debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    result("Entry points",midi && debug,1);if(!midi || !debug)goto done;
    result("Device count",midi(0,MODM_GETNUMDEVS,0L,0L,0L),1);
    result("Bad device",midi(1,MODM_GETNUMDEVS,0L,0L,0L),2);
    for(i=0;i<8;++i)bounded.guard[i]=0x5a;
    result("Get caps",midi(0,MODM_GETDEVCAPS,0L,
        (DWORD)(LPVOID)&bounded.caps,sizeof(bounded.caps)),0);
    result("Three voices",bounded.caps.wVoices,3);
    result("Version",bounded.caps.vDriverVersion,0x0102);
    result("Caps guard",bounded.guard[0],0x5a);
    bounded.caps.wPid=0x5a5a;
    result("Short caps",midi(0,MODM_GETDEVCAPS,0L,
        (DWORD)(LPVOID)&bounded.caps,2L),0);
    result("Short caps bounded",bounded.caps.wPid,0x5a5a);
    result("Closed DATA blocked",data(0x90,60,96),4);
    initial=debug(2);
    soundVoices=OpenSound();result("OpenSound first",soundVoices>=0,1);
    result("SOUND owner blocks MIDI",openDriver(),4);
    result("Failed open writes nothing",debug(2),initial);
    CloseSound();
    desc.dwCallback=1;result("Callbacks rejected",openDriver(),8);
    desc.dwCallback=0;result("Open",openDriver(),0);
    result("Cookie",cookie,MIDI_COOKIE);
    result("Exclusive",openDriver(),4);
    result("Sound locked",OpenSound(),(DWORD)-1L);
    result("Wrong cookie rejected",midi(0,MODM_RESET,0L,0L,0L),4);
    result("C4 note on",data(0x90,60,127),0);
    result("C4 period",debug(8),428);
    result("Max velocity",debug(12),0);
    result("E4 note on",data(0x91,64,64),0);
    result("G4 note on",data(0x92,67,1),0);
    result("Polyphony mask",debug(0),7);
    result("Min velocity audible",debug(14),14);
    waitms(550);
    result("Fourth tone steals",data(0x93,72,96),0);
    result("Oldest stolen",debug(4),72);
    result("Steal count",debug(3),1);
    result("Old note off ignored",data(0x80,60,0),0);
    result("New note remains",debug(4),72);
    result("Zero velocity off",data(0x93,72,0),0);
    result("Voice off",debug(4),0);
    result("Low note rejected",data(0x90,44,96),11);
    result("High note rejected",data(0x90,97,96),11);
    result("Bad data rejected",data(0x90,60,128),11);
    result("High byte rejected",call(MODM_DATA,0x01903c90UL),11);
    result("Program supported",data(0xc0,1,0),0);
    result("Pitch bend unsupported",data(0xe0,0,64),8);
    result("Long data unsupported",call(MODM_LONGDATA,0L),8);
    result("CC unsupported",data(0xb0,7,127),8);
    result("Hat",data(0x99,42,90),0);result("Noise active",debug(7),42);
    waitms(180);
    result("Wrong drum off",data(0x89,38,0),0);result("Hat remains",debug(7),42);
    result("Hat off",data(0x89,42,0),0);result("Noise off",debug(7),0);
    result("Snare",data(0x99,38,100),0);waitms(180);
    result("Channel10 all notes off",data(0xb9,123,0),0);result("Noise CC off",debug(7),0);
    result("Channel2 all sound off",data(0xb1,120,0),0);result("Channel2 off",debug(5),0);
    result("Channel3 remains",debug(6),67);
    result("Reset",call(MODM_RESET,0L),0);result("Reset silent",debug(0),0);
    result("Repeat reset",call(MODM_RESET,0L),0);
    result("Owner note",data(0x90,69,96),0);
    result("Intruder launch",WinExec("C:\\MIDIINTR.EXE",SW_SHOWNORMAL)>31,1);
    waitms(700);f=_lopen("C:\\MIDIINTR.LOG",OF_READ);
    result("Intruder ran",f!=HFILE_ERROR,1);if(f!=HFILE_ERROR)_lclose(f);
    result("Owner note survived",debug(4),69);
    result("Close active",call(MODM_CLOSE,0L),0);
    result("Close silent",debug(0),0);result("Owner released",debug(1),0);
    result("Repeat close rejected",call(MODM_CLOSE,0L),4);
    for(i=0;i<12;++i) {
        result("Cycle open",openDriver(),0);result("Cycle note",data(0x90,60,96),0);
        waitms(55);result("Cycle close",call(MODM_CLOSE,0L),0);
        result("Cycle silent",debug(0),0);
    }
    result("Sound released",OpenSound(),soundVoices);CloseSound();
    FreeLibrary(module);module=LoadLibrary("C:\\MIDIMAP.DRV");
    result("Reload driver",module>=32,1);
    midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
    debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");
    result("Reload no owner",debug(1),0);
    result("Reload open",openDriver(),0);result("Reload close",call(MODM_CLOSE,0L),0);
    FreeLibrary(module);waitms(700);
done:
    result("TOTAL_FAILURES",failures,0);result("CHECKS",checks,checks);
    _lclose(log);ExitWindows(0L,0);return failures;
}

