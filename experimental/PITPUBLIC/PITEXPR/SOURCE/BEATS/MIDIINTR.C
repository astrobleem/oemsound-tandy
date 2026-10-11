#define WINVER 0x0300
#include <windows.h>
#include "MIDIAPI.H"
static HFILE log;
static int failures;
static void check(char *name,DWORD value,DWORD want)
{
    char line[90];wsprintf(line,"%s got=%lu want=%lu\r\n",(LPSTR)name,value,want);
    _lwrite(log,line,lstrlen(line));if(value!=want)++failures;
}
int PASCAL WinMain(HINSTANCE h,HINSTANCE p,LPSTR cmd,int show)
{
    HMODULE m;MIDIPROC f;TOPEN desc;DWORD cookie;
    log=_lcreat("C:\\MIDIINTR.LOG",0);
    m=LoadLibrary("C:\\MIDIMAP.DRV");f=(MIDIPROC)GetProcAddress(m,"MIDIMESSAGE");
    desc.hMidi=0;desc.dwCallback=0;desc.dwInstance=0;
    check("Nonowner open",f(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,
        (DWORD)(LPVOID)&desc,0L),4);
    check("Nonowner data",f(0,MODM_DATA,MIDI_COOKIE,0x007f3c90UL,0L),4);
    check("Nonowner reset",f(0,MODM_RESET,MIDI_COOKIE,0L,0L),4);
    check("Nonowner close",f(0,MODM_CLOSE,MIDI_COOKIE,0L,0L),4);
    check("Nonowner SOUND",OpenSound(),(DWORD)-1L);
    FreeLibrary(m);check("TOTAL_FAILURES",failures,0);_lclose(log);return 0;
}
