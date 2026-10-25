/* Contributor-only Win16 shell for the existing PIT Fiddle self-test. */
#define WINVER 0x0300
#include <windows.h>
int PASCAL WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR cmd,int show)
{
    HFILE log;
    UINT launched;
    HMODULE sound;
    char line[180],path[128];
    log=_lcreat("C:\\PITBOOT.LOG",0);
    path[0]=0;
    sound=GetModuleHandle("SOUND");
    GetModuleFileName(sound,path,sizeof(path));
    wsprintf(line,"loaded_sound=%s\r\n",(LPSTR)path);
    if(log!=HFILE_ERROR) _lwrite(log,line,lstrlen(line));
    launched=WinExec("C:\\PIT\\MMLFIDL.EXE /pittest",SW_SHOWNORMAL);
    wsprintf(line,"PITFiddleLaunch got=%u want=1\r\n",launched>31?1:0);
    if(log!=HFILE_ERROR) {
        _lwrite(log,line,lstrlen(line));
        _lclose(log);
    }
    return launched>31?0:1;
}
