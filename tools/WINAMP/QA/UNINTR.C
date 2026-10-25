/* Separate native Win16 task tests actual SOUND task ownership. */
#include "QACOMMON.H"
int PASCAL WinMain(HINSTANCE h,HINSTANCE old,LPSTR cmd,int show)
{
    HMODULE sound;
    TSGGET psg;
    TANDYPITPROC pit;
    TPGET lease;
    TPINFO info;
    QBEEP beep;
    int busy;
    WORD i;
    char *zero=(char *)&info;
    char path[128];
    for(i=0;i<sizeof(info);++i) zero[i]=0;
    busy=lstrcmpi(cmd,"/FREE")!=0;
    qlog=_lcreat(busy?"C:\\UNBUSY.LOG":"C:\\UNFREE.LOG",0);
    sound=GetModuleHandle("SOUND");
    check("SoundModule",sound!=0,1);
    GetModuleFileName(sound,path,sizeof(path));
    text("loaded_sound",path);
    psg=(TSGGET)GetProcAddress(sound,"TANDYPSGLEASE");
    pit=(TANDYPITPROC)GetProcAddress(sound,"TANDYPITVOICE");
    lease=(TPGET)GetProcAddress(sound,"TANDYPITLEASE");
    beep=(QBEEP)GetProcAddress(sound,"DOBEEP");
    check("LeaseExports",psg&&pit&&lease&&beep,1);
    if(!psg||!pit||!lease||!beep) goto done;
    if(busy) {
        check("NonownerOpen",OpenSound(),-1);
        check("NonownerStart",StartSound(),-1);
        check("NonownerStop",StopSound(),-1);
        check("NonownerQueue",SetVoiceNote(1,25,4,0),-1);
        check("NonownerPSGClaim",psg(TSG_VERSION,TSG_CLAIM),-1);
        check("NonownerPSGRelease",psg(TSG_VERSION,TSG_RELEASE),-1);
        check("NonownerPITVoice",pit(1,440,1),-1);
        check("NonownerPITClaim",lease(TP_VERSION,&info,
                                      sizeof(info),TP_CLAIM),-1);
        check("BusyAlertSuppressed",beep(0),0);
        CloseSound();
    } else {
        check("HandoffOpen",OpenSound(),3);
        check("HandoffStop",StopSound(),0);
        check("HandoffPSGClaim",psg(TSG_VERSION,TSG_CLAIM),0);
        check("HandoffPITClaim",lease(TP_VERSION,&info,
                                     sizeof(info),TP_CLAIM),0);
        if(info.token&&info.fast) {
            check("HandoffPITSet",info.fast(info.token,2712,TP_SET),0);
            check("HandoffPITRelease",lease(TP_VERSION,&info,
                                           sizeof(info),TP_RELEASE),0);
        }
        check("HandoffPSGRelease",psg(TSG_VERSION,TSG_RELEASE),0);
        CloseSound();
        check("HandoffAvailable",psg(TSG_VERSION,TSG_QUERY),0);
    }
done:
    finishlog();
    return failures;
}
