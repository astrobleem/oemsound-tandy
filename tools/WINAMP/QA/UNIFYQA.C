/* Native production-driver checks, then unchanged Winamp/GDI exercise.
   Build with MSC6 /G0. No SOUND_TEST exports or alternate driver loading. */
#include "QACOMMON.H"
extern unsigned _cdecl PitBits(void);
/* This DOSBox-X fixture stores lower-nibble controls only.
   Bits4/5 read dynamic status here; see PORT61.MD for physical EX mapping. */
#define QA_P61_STORED 0x0cU
static HMODULE soundModule;
static TSGGET psg;
static TANDYPITPROC pit;
static TPGET lease;
static QBEEP beep;
static HWND player,window;
static WORD phase,frames,rounds;
static DWORD started,wholeRun;
static char soundFile[128],displayFile[128],child[160];
static char expectedSound[128],expectedDisplay[32],application[128];
static void waitms(unsigned ms)
{
    DWORD at=GetTickCount();
    while(GetTickCount()-at<ms) Yield();
}
static int loghas(char *path,char *needle)
{
    HFILE f;
    int n;
    char b[2048];
    f=_lopen(path,OF_READ);
    if(f==HFILE_ERROR) return 0;
    n=_lread(f,b,sizeof(b)-1);
    _lclose(f);
    if(n<0) return 0;
    b[n]=0;
    return strstr(b,needle)!=0;
}
static int childrun(int free)
{
    DWORD at;
    HFILE fresh;
    char *path=free?"C:\\UNFREE.LOG":"C:\\UNBUSY.LOG";
    fresh=_lcreat(path,0);
    if(fresh==HFILE_ERROR) return 0;
    _lclose(fresh);
    wsprintf(child,"%s\\UNINTR.EXE %s",(LPSTR)application,
             (LPSTR)(free?"/FREE":"/BUSY"));
    check(free?"HandoffLaunch":"IntruderLaunch",
          WinExec(child,SW_SHOWNORMAL)>31,1);
    at=GetTickCount();
    while(GetTickCount()-at<4000UL) {
        if(loghas(path,"TOTAL_FAILURES got=0 want=0")) return 1;
        if(loghas(path,"TOTAL_FAILURES")) return 0;
        Yield();
    }
    return 0;
}
static void sameSound(char *name)
{
    char path[128];
    HMODULE sound=GetModuleHandle("SOUND");
    check(name,sound==soundModule,1);
    GetModuleFileName(sound,path,sizeof(path));
    check("SameInstalledSoundPath",lstrcmpi(path,soundFile)==0,1);
}
static int exports(void)
{
    char *base;
    WORD i;
    soundModule=GetModuleHandle("SOUND");
    check("SoundModule",soundModule!=0,1);
    if(!soundModule) return 0;
    GetModuleFileName(soundModule,soundFile,sizeof(soundFile));
    text("loaded_sound",soundFile);
    GetProfileString("UNIFYQA","sound",
                     "C:\\WINDOWS\\SYSTEM\\SOUND.DRV",expectedSound,
                     sizeof(expectedSound));
    check("CanonicalInstalledSound",lstrcmpi(soundFile,expectedSound)==0,1);
    check("RealMode",(GetWinFlags()&WF_PMODE)==0,1);
    GetProfileString("UNIFYQA","display","XRSYNC.DRV",expectedDisplay,
                     sizeof(expectedDisplay));
    GetModuleFileName(GetModuleHandle("DISPLAY"),displayFile,
                      sizeof(displayFile));
    text("loaded_display",displayFile);
    base=displayFile;
    for(i=0;displayFile[i];++i)
        if(displayFile[i]=='\\') base=displayFile+i+1;
    check("ExpectedDisplay",lstrcmpi(base,expectedDisplay)==0,1);
    psg=(TSGGET)GetProcAddress(soundModule,"TANDYPSGLEASE");
    pit=(TANDYPITPROC)GetProcAddress(soundModule,"TANDYPITVOICE");
    lease=(TPGET)GetProcAddress(soundModule,"TANDYPITLEASE");
    beep=(QBEEP)GetProcAddress(soundModule,"DOBEEP");
    check("UnifiedLeaseExports",psg&&pit&&lease&&beep,1);
    check("ProductionNoTestHook",
          GetProcAddress(soundModule,"DEBUGTESTTICK")==0,1);
    return psg&&pit&&lease&&beep;
}
static void legacy(void)
{
    unsigned n;
    DWORD at;
    TPINFO pending;
    check("IdleAvailable",psg(1,TSG_QUERY),0);
    at=GetTickCount();
    check("IdleAlert",beep(0),1);
    check("IdleAlertBounded",GetTickCount()-at<1000UL,1);
    check("IdleAlertReleases",psg(1,TSG_QUERY),0);
    check("LegacyOpen",OpenSound(),3);
    check("LegacySecondOpen",OpenSound(),-1);
    check("LegacyAccent",SetVoiceAccent(1,120,65535U,0,0),0);
    check("LegacyThreshold",SetVoiceThreshold(1,0),0);
    check("LegacyC",SetVoiceSound(1,440UL<<16,800),0);
    check("LegacyE",SetVoiceSound(2,660UL<<16,800),0);
    check("LegacyG",SetVoiceSound(3,880UL<<16,800),0);
    check("PITRefusesPendingQueue",lease(TP_VERSION,&pending,
                                        sizeof(pending),TP_CLAIM),-3);
    check("LegacyStart",StartSound(),0);
    check("RawRefusesManagedQueues",psg(1,TSG_CLAIM),-3);
    check("ManagedBusyAlert",beep(0),1);
    check("ManagedQueuesPreserved",CountVoiceNotes(1)>0,1);
    check("ManagedWait",WaitSoundState(S_QUEUEEMPTY),0);
    check("ManagedQueuesEnd",CountVoiceNotes(1),0);
    check("ManagedStop",StopSound(),0);
    CloseSound();
    for(n=0;n<3;++n) {
        check("LegacyReopen",OpenSound(),3);
        check("LegacyShortNote",SetVoiceNote(1,25,16,0),0);
        check("LegacyRestart",StartSound(),0);
        check("LegacyWait",WaitSoundState(S_QUEUEEMPTY),0);
        check("LegacyStop",StopSound(),0);
        CloseSound();
    }
    check("LegacyReleased",psg(1,TSG_QUERY),0);
    sameSound("LegacySameModule");
}
static void leases(void)
{
    TPINFO info;
    TPFAST fast;
    unsigned token,bits;
    check("RawOpen",OpenSound(),3);
    check("RawStop",StopSound(),0);
    check("PlainOwnerAlertRefused",beep(0),0);
    check("RawClaim",psg(1,TSG_CLAIM),0);
    check("RawClaimIdempotent",psg(1,TSG_CLAIM),0);
    check("RawAlertRefused",beep(0),0);
    check("RawStartRefused",StartSound(),-3);
    check("RawQueueRefused",SetVoiceSound(1,440UL<<16,40),-3);
    check("IntruderAllRefused",childrun(0),1);
    check("OwnerSurvivesNonownerClose",psg(1,TSG_CLAIM),0);
    check("PITQuery",lease(TP_VERSION,&info,sizeof(info),TP_QUERY),0);
    check("PITCapabilities",info.size==sizeof(info)&&
          info.version==TP_VERSION&&info.features==
          (TP_FIXED|TP_EXCLUSIVE|TP_SUSPEND_SAFE)&&
          info.minDivisor==TP_MIN_DIVISOR&&info.maxDivisor==65535U&&
          info.token==0&&info.fast!=0,1);
    bits=PitBits()&QA_P61_STORED;
    check("PITRawCoexistClaim",lease(TP_VERSION,&info,sizeof(info),
                                   TP_CLAIM),0);
    token=info.token;
    fast=info.fast;
    check("PITToken",token!=0&&fast!=0,1);
    if(token&&fast) {
        check("PITSound",fast(token,2712,TP_SET),0);
        waitms(120);
        check("PITGateSet",PitBits()&3U,3);
        check("PITEmulatedControlBits",PitBits()&QA_P61_STORED,bits);
        check("PITSimpleRefusesFast",pit(1,440,1),-3);
        check("PITSuspend",fast(token,0,TP_SUSPEND),0);
        check("PITSuspendMuted",PitBits()&3U,0);
        check("PITSuspendedSetRefused",fast(token,2712,TP_SET),-3);
        check("PITReenter",fast(token,0,TP_REENTER),0);
        check("PITResumeSound",fast(token,2280,TP_SET),0);
        check("PITMute",fast(token,0,TP_MUTE),0);
        check("PITRelease",lease(TP_VERSION,&info,sizeof(info),
                                 TP_RELEASE),0);
        check("PITReleasedGate",PitBits()&3U,0);
        check("PITStaleTokenRefused",fast(token,2712,TP_SET),-1);
    }
    check("RawRelease",psg(1,TSG_RELEASE),0);
    check("PITSimpleStart",pit(1,440,1),0);
    waitms(120);
    check("PITFastRefusesSimple",lease(TP_VERSION,&info,sizeof(info),
                                      TP_CLAIM),-3);
    check("PITSimpleStop",pit(1,0,0),0);
    check("PITSimpleRestart",pit(1,523,1),0);
    check("StopRevokesPIT",StopSound(),0);
    check("StopPITMuted",PitBits()&3U,0);
    check("StopRevokesRaw",psg(1,TSG_RELEASE),-1);
    CloseSound();
    check("HandoffAllAccepted",childrun(1),1);
    check("HandoffReleased",psg(1,TSG_QUERY),0);
    sameSound("LeasesSameModule");
}
static void emptylog(char *path)
{
    HFILE f=_lcreat(path,0);
    if(f!=HFILE_ERROR) _lclose(f);
}
static int copylog(char *source,char *destination)
{
    HFILE in,out;
    int n,okay=1;
    char block[256];
    in=_lopen(source,OF_READ);
    if(in==HFILE_ERROR) return 0;
    out=_lcreat(destination,0);
    if(out==HFILE_ERROR) { _lclose(in);return 0; }
    while((n=_lread(in,block,sizeof(block)))>0)
        if(_lwrite(out,block,n)!=n) { okay=0;break; }
    if(n<0) okay=0;
    _lclose(in);_lclose(out);
    return okay;
}
static void client(char *exe,char *argument,char *module,char *cls,
                   unsigned timeout,unsigned minimum,int busy)
{
    DWORD at;
    HWND focus;
    int saw=0,owned=0,ended=0;
    char label[80];
    wsprintf(child,"%s\\%s %s",(LPSTR)application,(LPSTR)exe,
             (LPSTR)argument);
    text(busy?"busy_client":"client",child);
    wsprintf(label,"%sLaunch",(LPSTR)module);
    check(label,WinExec(child,SW_SHOWNORMAL)>31,1);
    at=GetTickCount();
    while(GetTickCount()-at<timeout) {
        if(GetModuleHandle(module)) saw=1;
        else if(saw||busy) { ended=1;break; }
        if(!busy&&psg(1,TSG_QUERY)==-3) owned=1;
        if(cls) {
            focus=FindWindow(cls,0);
            if(focus) { SetActiveWindow(focus);SetFocus(focus); }
        }
        Yield();
    }
    wsprintf(label,"%sExited",(LPSTR)module);check(label,ended,1);
    if(!busy) {
        wsprintf(label,"%sUsedInstalledSound",(LPSTR)module);
        check(label,owned,1);
        wsprintf(label,"%sMinimumDuration",(LPSTR)module);
        check(label,GetTickCount()-at>=minimum,1);
        wsprintf(label,"%sReleasedSound",(LPSTR)module);
        check(label,psg(1,TSG_QUERY),0);
    } else {
        wsprintf(label,"%sPreservedOtherOwner",(LPSTR)module);
        check(label,psg(1,TSG_CLAIM),0);
    }
    sameSound("ClientSameSoundModule");
}
static void actualclients(void)
{
    client("TCHIME.EXE","","TCHIME",0,3000,650,0);
    client("XPCHIME.EXE","","XPCHIME",0,4500,2000,0);
    emptylog("C:\\TEXIT.LOG");
    client("TEXIT.EXE","/testpreview","TEXIT",0,3000,650,0);
    check("ExitPreviewStarted",loghas("C:\\TEXIT.LOG","START"),1);
    check("ExitPreviewCleaned",
          loghas("C:\\TEXIT.LOG","MUTED WINDOW GONE"),1);
    check("ExitPreviewStayedInWindows",
          loghas("C:\\TEXIT.LOG","NO EXIT REQUEST"),1);
    check("ExitPreviewNoLateAbort",!loghas("C:\\TEXIT.LOG",
                                        "LATE ABORT"),1);
    check("ExitPreviewReceipt",copylog("C:\\TEXIT.LOG",
                                      "C:\\TEXGOOD.LOG"),1);
    emptylog("C:\\TPTEST.LOG");
    client("TP.EXE","/TEST","TP","WindowsXTToiletPaper",6500,3500,0);
    check("TPComplete",loghas("C:\\TPTEST.LOG","COMPLETE=1"),1);
    check("TPFunctionalPass",loghas("C:\\TPTEST.LOG","FAIL_COUNT=0"),1);
    check("TPFlushOnce",loghas("C:\\TPTEST.LOG","FLUSH_TRANSITIONS=1"),1);
    check("TPSoundSupported",loghas("C:\\TPTEST.LOG","SOUND_SUPPORTED=1"),1);
    check("TPSoundStarted",loghas("C:\\TPTEST.LOG","SOUND_STARTED=1"),1);
    check("TPSoundCompleted",loghas("C:\\TPTEST.LOG","SOUND_COMPLETED=1"),1);
    check("TPNoBusySkip",loghas("C:\\TPTEST.LOG","SOUND_BUSY_SKIPS=0"),1);
    check("TPGoodReceipt",copylog("C:\\TPTEST.LOG","C:\\TPGOOD.LOG"),1);
    check("BusyClientsOpen",OpenSound(),3);
    check("BusyClientsStop",StopSound(),0);
    check("BusyClientsClaim",psg(1,TSG_CLAIM),0);
    client("TCHIME.EXE","","TCHIME",0,3000,0,1);
    client("XPCHIME.EXE","","XPCHIME",0,4500,0,1);
    emptylog("C:\\TEXIT.LOG");
    client("TEXIT.EXE","/testpreview","TEXIT",0,3000,0,1);
    check("BusyExitDeclines",loghas("C:\\TEXIT.LOG",
                                   "BUSY: NO PSG OR EXIT"),1);
    check("BusyExitReceipt",copylog("C:\\TEXIT.LOG","C:\\TEXBUSY.LOG"),1);
    emptylog("C:\\TPTEST.LOG");
    client("TP.EXE","/TEST","TP","WindowsXTToiletPaper",6500,0,1);
    check("BusyTPComplete",loghas("C:\\TPTEST.LOG","COMPLETE=1"),1);
    check("BusyTPFunctionalPass",loghas("C:\\TPTEST.LOG","FAIL_COUNT=0"),1);
    check("BusyTPNoSoundStart",loghas("C:\\TPTEST.LOG","SOUND_STARTED=0"),1);
    check("BusyTPSkipped",loghas("C:\\TPTEST.LOG","SOUND_BUSY_SKIPS=1"),1);
    check("BusyTPReceipt",copylog("C:\\TPTEST.LOG","C:\\TPBUSY.LOG"),1);
    check("BusyClientsRelease",psg(1,TSG_RELEASE),0);
    CloseSound();
    check("ClientsFinalAvailable",psg(1,TSG_QUERY),0);
}
static void shellcase(int busy)
{
    char dir[128],command[160],configuration[144],selected[32];
    char loaded[128];
    HWND bar=0;
    WORD launched;
    DWORD at;
    int sawbar=0,sawchime=0,sawbusy=0,finished=0;
    check("TSHELLInitiallyAbsent",GetModuleHandle("TSHELL")==0,1);
    if(GetModuleHandle("TSHELL")) return;
    check("TSHELLWindowInitiallyAbsent",FindWindow("TandyStart",0)==0,1);
    if(FindWindow("TandyStart",0)) return;
    GetWindowsDirectory(dir,sizeof(dir));
    lstrcpy(configuration,dir);lstrcat(configuration,"\\SYSTEM.INI");
    GetPrivateProfileString("boot","shell","",selected,sizeof(selected),
                            configuration);
    check("TSHELLConfiguredPrimary",lstrcmpi(selected,"TSHELL.EXE")==0,1);
    wsprintf(command,"%s\\TSHELL.EXE",(LPSTR)dir);
    launched=WinExec(command,SW_SHOWNORMAL);
    check(busy?"BusyTSHELLLaunch":"TSHELLLaunch",launched>31,1);
    if(launched<32) return;
    at=GetTickCount();
    while(GetTickCount()-at<4000UL) {
        if(!bar) bar=FindWindow("TandyStart",0);
        if(bar) sawbar=1;
        if(GetModuleHandle("TCHIME")) {
            if(!sawchime) {
                GetModuleFileName(GetModuleHandle("TCHIME"),loaded,
                                  sizeof(loaded));
                text("loaded_startup_chime",loaded);
                check("TSHELLInstalledStartupPath",
                      lstrcmpi(loaded,"C:\\WINXT\\TCHIME.EXE")==0,1);
            }
            sawchime=1;
        }
        if(!busy&&psg(1,TSG_QUERY)==-3) sawbusy=1;
        if(sawbar&&((busy&&GetTickCount()-at>=1400UL)||
           (!busy&&sawchime&&!GetModuleHandle("TCHIME")&&
            psg(1,TSG_QUERY)==0))) { finished=1;break; }
        Yield();
    }
    check(busy?"BusyTSHELLWindow":"TSHELLWindow",sawbar,1);
    check(busy?"BusyTSHELLStartupSettled":"TSHELLStartupSettled",finished,1);
    if(GetModuleHandle("TSHELL")) {
        GetModuleFileName(GetModuleHandle("TSHELL"),loaded,sizeof(loaded));
        text("loaded_shell",loaded);
        check("TSHELLInstalledPath",lstrcmpi(loaded,command)==0,1);
    }
    if(busy) {
        check("BusyTSHELLNoChime",sawchime,0);
        check("BusyTSHELLPreservedOwner",psg(1,TSG_CLAIM),0);
    } else {
        check("TSHELLStartedChime",sawchime,1);
        check("TSHELLChimeOwnedSound",sawbusy,1);
        check("TSHELLChimeReleasedSound",psg(1,TSG_QUERY),0);
    }
    /* Normal shutdown notification tests cleanup, without requesting exit. */
    check("TSHELLShutdownWindow",bar&&IsWindow(bar),1);
    if(bar&&IsWindow(bar)) {
        check("TSHELLOwnedWindow",
              GetWindowWord(bar,GWW_HINSTANCE)==launched,1);
        GetModuleFileName((HMODULE)GetWindowWord(bar,GWW_HINSTANCE),
                          loaded,sizeof(loaded));
        check("TSHELLWindowInstalledPath",lstrcmpi(loaded,command)==0,1);
        if(GetWindowWord(bar,GWW_HINSTANCE)==launched&&
           lstrcmpi(loaded,command)==0)
            SendMessage(bar,WM_ENDSESSION,TRUE,0L);
    }
    at=GetTickCount();
    while(GetModuleHandle("TSHELL")&&GetTickCount()-at<3000UL) Yield();
    check(busy?"BusyTSHELLUnloaded":"TSHELLUnloaded",
          GetModuleHandle("TSHELL")==0,1);
    check("TSHELLWindowGone",!bar||!IsWindow(bar),1);
    if(busy) check("BusyTSHELLCleanupPreservedOwner",psg(1,TSG_CLAIM),0);
    else check("TSHELLCleanupSoundAvailable",psg(1,TSG_QUERY),0);
    sameSound("TSHELLSameInstalledSound");
}
static void shellstartup(void)
{
    shellcase(0);
    check("BusyTSHELLOpen",OpenSound(),3);
    check("BusyTSHELLStop",StopSound(),0);
    check("BusyTSHELLClaim",psg(1,TSG_CLAIM),0);
    shellcase(1);
    check("BusyTSHELLRelease",psg(1,TSG_RELEASE),0);
    CloseSound();
    check("TSHELLFinalSoundAvailable",psg(1,TSG_QUERY),0);
}
static void stoprun(void)
{
    check("AllPass",failures==0,1);
    finishlog();
    if(window) KillTimer(window,1);
    ExitWindows(0L,0);
}
LONG FAR PASCAL WndProc(HWND w,WORD m,WORD a,LONG b)
{
    if(m==WM_TIMER) {
        if(GetTickCount()-wholeRun>75000UL) {
            check("WholeRunTimeout",0,1);
            stoprun();
            return 0L;
        }
        if(!phase) {
            player=FindWindow("MiniMCIPlayer",0);
            if(!player) {
                if(GetTickCount()-started>10000UL) {
                    check("PlayerFound",0,1);stoprun();
                }
                return 0L;
            }
            check("PlayerFound",1,1);
            SendMessage(player,WM_COMMAND,101,0L);
            check("PlayerPlay",SendMessage(player,WM_USER+47,0,0L)!=0,1);
            check("PlayerRawAlertRefused",beep(0),0);
            check("PlayerOtherOpenRefused",OpenSound(),-1);
            /* Nonowner CloseSound must preserve the actual player owner. */
            CloseSound();
            check("PlayerSurvivesOtherClose",
                  SendMessage(player,WM_USER+47,0,0L)!=0,1);
            phase=1;started=GetTickCount();frames=0;
        } else if(phase==1) {
            RECT r;
            HDC dc;
            HBRUSH brush;
            MoveWindow(player,(frames&1)?2:14,2,304,192,TRUE);
            InvalidateRect(player,0,TRUE);UpdateWindow(player);
            dc=GetDC(w);SetRect(&r,0,0,24,24);
            brush=CreateSolidBrush((frames&1)?RGB(255,0,0):RGB(0,255,0));
            FillRect(dc,&r,brush);DeleteObject(brush);ReleaseDC(w,dc);
            ++frames;
            if(GetTickCount()-started>3000UL) {
                check("GraphicsUpdates",frames>=8,1);
                check("ForegroundService",
                      SendMessage(player,WM_USER+42,0,0L)>0,1);
                check("PlayingDuringGraphics",
                      SendMessage(player,WM_USER+47,0,0L)!=0,1);
                SendMessage(player,WM_COMMAND,102,0L);
                check("PlayerStopped",
                      SendMessage(player,WM_USER+47,0,0L)==0,1);
                check("PlayerStopReleases",OpenSound(),3);
                CloseSound();
                check("PlayerStopAvailable",psg(1,TSG_QUERY),0);
                sameSound("PlayerSameModule");
                if(++rounds<2) { phase=0;started=GetTickCount(); }
                else {
                    PostMessage(player,WM_CLOSE,0,0L);
                    phase=2;started=GetTickCount();
                }
            }
        } else if(phase==2) {
            if(!IsWindow(player)) {
                check("PlayerClosed",1,1);
                check("PlayerCloseAvailable",psg(1,TSG_QUERY),0);
                check("FinalIdleAlert",beep(0),1);
                sameSound("FinalSameModule");
                stoprun();
            } else if(GetTickCount()-started>5000UL) {
                check("PlayerCloseTimeout",0,1);stoprun();
            }
        }
        return 0L;
    }
    return DefWindowProc(w,m,a,b);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE prev,LPSTR cmd,int show)
{
    WNDCLASS wc;
    MSG msg;
    WORD i;
    char *p=(char *)&wc;
    qlog=_lcreat("C:\\UNIFYQA.LOG",0);
    if(qlog==HFILE_ERROR) {
        /* No evidence means no qualification run. Exit normally and fail. */
        ExitWindows(0L,0);
        return 20;
    }
    GetProfileString("UNIFYQA","app","C:\\MMLAB",application,
                     sizeof(application));
    wholeRun=GetTickCount();
    if(!exports()) { finishlog();ExitWindows(0L,0);return 1; }
    legacy();leases();actualclients();shellstartup();
    for(i=0;i<sizeof(wc);++i) p[i]=0;
    wc.lpfnWndProc=WndProc;wc.hInstance=inst;wc.lpszClassName="UNIFYQA";
    wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    check("RegisterObserver",RegisterClass(&wc)!=0,1);
    window=CreateWindow("UNIFYQA","Unified sound graphics check",
           WS_OVERLAPPEDWINDOW,350,20,120,90,0,0,inst,0L);
    check("CreateObserver",window!=0,1);
    ShowWindow(window,SW_SHOW);
    check("ObserverTimer",SetTimer(window,1,200,0)!=0,1);
    started=GetTickCount();
    wsprintf(child,"%s\\MINIPLAY.EXE",(LPSTR)application);
    check("PlayerLaunch",WinExec(child,SW_SHOWNORMAL)>31,1);
    while(GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
    return failures;
}
