/* Shared owner for legacy direct-PSG Windows consumers. C89 /G0.
 * Raw PSG writes are permitted only while the task holds OpenSound.
 * No fallback if acquisition/capability fails. No system-beep guarantee.
 */
#ifndef TANDY_SNDOWN_H
#define TANDY_SNDOWN_H
#include "PITCLNT.H"
static int soundOwned, pitOption;
static PIT_CLIENT soundPit;
static unsigned soundWrites, soundPitStarts;
static int SoundAcquire(void)
{
    if(soundOwned)return 1;
    if(OpenSound()<1)return 0;
    soundOwned=1;
    if(pitOption && PitClientDiscover(&soundPit)<0) {
        CloseSound();soundOwned=0;return 0;
    }
    return 1;
}
static int SoundPitNote(unsigned note)
{
    if(!pitOption)return 1;
    if(PitClientNote(&soundPit,soundOwned,note)!=0)return 0;
    ++soundPitStarts;return 1;
}
static void SoundRelease(void)
{
    if(soundOwned) {
        if(pitOption)PitClientOff(&soundPit,1);
        StopSound();CloseSound();soundOwned=0;
    }
    PitClientForget(&soundPit);
}
static int soundTest, soundProbe;
static int SoundTestArgs(LPSTR cmd)
{
    if(lstrcmp(cmd,"/test")==0)soundTest=1;
    if(lstrcmp(cmd,"/pittest")==0){soundTest=2;pitOption=1;}
    if(lstrcmp(cmd,"/busytest")==0)soundTest=3;
    if(lstrcmp(cmd,"/pitold")==0){soundTest=4;pitOption=1;}
    if(lstrcmp(cmd,"/pitclose")==0){soundTest=5;pitOption=1;}
    if(lstrcmp(cmd,"/pitfocus")==0){soundTest=6;pitOption=1;}
    if(lstrcmp(cmd,"/pitescape")==0){soundTest=7;pitOption=1;}
    return soundTest;
}
static int SoundAudit(void)
{
    HFILE f;char b[96];int passed;
    if(!soundTest)return 0;
    passed=!soundOwned && !soundPit.note;
    if(soundTest==3 || soundTest==4)passed=passed && !soundWrites;
    else passed=passed && soundWrites>0;
    if(soundTest==2 || soundTest>=5)passed=passed && soundPitStarts>0;
    f=_lcreat("C:\\SNDOWN.LOG",0);
    if(f!=HFILE_ERROR) {
        wsprintf(b,"%s owned=%u pit=%u writes=%u mode=%u\r\n",
            (LPSTR)(passed?"PASS":"FAIL"),soundOwned,soundPit.note,
            soundWrites,soundTest);
        _lwrite(f,b,lstrlen(b));_lclose(f);
    }
    return passed?0:1;
}
#endif
