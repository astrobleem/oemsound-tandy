/* Original MML1 editor/player. Microsoft C6 /G0, Win3 real mode. */
#define WINVER 0x0300
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "MIDIAPI.H"
#include "MMLCORE.H"
#define FILEBOX 101
#define OPEN 102
#define SAVE 103
#define PARTA 104
#define PARTB 105
#define PARTC 106
#define PRESET 107
#define EDITOR 108
#define PLAY 109
#define STOP 110
#define PARTN 111
#define PARTP 112
#define PIT 113
static int pitEnabled;
static unsigned pitDesired,pitSent;
static DWORD pitAttack,pitSentAttack;
#define PULSE 1
static HINSTANCE instance,module;
static HWND window,editor;
static MIDIPROC midi;
static DEBUGPROC debug;
static TOPEN openDesc;
static TCAPS caps;
static DWORD cookie;
static int opened,playing,dirty,changing,selected,timerReady;
static char source[8193],scratch[8193],filename[144],appdir[144];
static char status[64]="Ready: tones+drums",detail[64]="MML1/2 expressive";
static char *names[8]={"Keys","Org","Bass","Pad","Reed","Lead","Bell","Hit"};
static unsigned char programs[3]={0,0,0};
static MML_SCORE score;
static MML_ITER iter[MML_PARTS];
static MML_EVENT events[MML_PARTS];
static MML_ERROR error;
static int pending[MML_PARTS];
static DWORD startTick,beginTick,position,duration,paintTick;
static unsigned sent,completions;
static HFILE logfile=HFILE_ERROR;
static int automatic,stage,failures;
static char *fixtures[4]={"BASIC.MML","DOTTED.MML","CARRIED.MML","PRESETS.MML"};
static DWORD fixtureTimes[4]={1000UL,1250UL,496UL,2000UL};
static char *noiseFixtures[6]={"BEAT40.MML","BEAT120.MML","BEAT240.MML",
    "ALLDRUM.MML","REPEAT.MML","HELD.MML"};
static DWORD noiseTimes[6]={6000UL,2000UL,1000UL,11750UL,1250UL,1000UL};
static const char demo[]="MML2\r\n[A]\r\nT120 O4 L8 @KEYS C E G > C4 < G8 E C4\r\n[B]\r\nO3 @PAD C1\r\n[C]\r\nO4 @BELL G2 R2\r\n[N]\r\nT120 V9 [N35/16. R32 N42/16. R32 N38/16. R32 N42/16. R32]2 R8\r\n";
static void textcopy(char *d,const char FAR *s,unsigned cap) {
    unsigned i=0;while(i+1<cap&&s[i]){d[i]=s[i];++i;}d[i]=0;
}
static void logtext(char *s) {
    if(logfile!=HFILE_ERROR){_lwrite(logfile,s,lstrlen(s));
        _lwrite(logfile,"\r\n",2);}
}
static void check(char *s,int ok) {
    char b[96];wsprintf(b,"%s %s",(LPSTR)(ok?"PASS":"FAIL"),(LPSTR)s);
    logtext(b);if(!ok)++failures;
}
static void redraw(void) {if(window)InvalidateRect(window,NULL,FALSE);}
static void setstatus(char *s) {textcopy(status,s,sizeof(status));redraw();}
static DWORD message(WORD m,DWORD v) {
    return midi?midi(0,m,MIDI_COOKIE,v,0L):MMSYSERR_ERROR;
}
static void release(void) {
    if(opened){
        check("reset success",message(MODM_RESET,0L)==0L);
        check("voices silent",debug&&debug(0)==0);
        check("close success",message(MODM_CLOSE,0L)==0L);
        opened=0;check("owner released",debug&&debug(1)==0);
    }
}
static void stop(void) {
    int i;playing=0;for(i=0;i<MML_PARTS;++i)pending[i]=0;
    pitDesired=pitSent=0;pitAttack=pitSentAttack=0;release();
    setstatus("Stopped / muted");
}
static int acquire(void) {
    DWORD r;cookie=0L;
    if(!midi){setstatus("Driver missing");return 0;}
    if(midi(0,MODM_GETDEVCAPS,0L,(DWORD)(LPVOID)&caps,sizeof(caps))||
        caps.wVoices!=3||caps.vDriverVersion!=0x0102){
        setstatus("Need WININST12");return 0;
    }
    if(score.present[MML_PIT_PART] && !GetProcAddress(GetModuleHandle("SOUND"),
        "TANDYPITVOICE")){setstatus("PIT unavailable");return 0;}
    openDesc.hMidi=0;openDesc.dwCallback=openDesc.dwInstance=0L;
    r=midi(0,MODM_OPEN,(DWORD)(LPVOID)&cookie,(DWORD)(LPVOID)&openDesc,0L);
    if(r){setstatus(r==MMSYSERR_ALLOCATED?"Sound is busy":"Open failed");return 0;}
    opened=1;
    if(cookie!=MIDI_COOKIE||message(MODM_SYNTHCONFIG,3L)){
        release();setstatus("Synth setup failed");return 0;
    }
    if(score.present[MML_PIT_PART] &&
       midi(0,TANDY_PIT_CONFIG,MIDI_COOKIE,1L,1L)){
        release();setstatus("PIT unavailable");return 0;
    }
    return 1;
}
static void showerror(void) {
    wsprintf(status,"Line %u col %u",error.line,error.column);
    textcopy(detail,error.message,sizeof(detail));redraw();
    SendMessage(editor,EM_SETSEL,0,MAKELONG(error.offset,error.offset+1));
    SetFocus(editor);
}
static void fileerror(unsigned length) {
    mml_validate(&score,scratch,length,programs,&error);
    wsprintf(status,"Line %u col %u",error.line,error.column);
    textcopy(detail,error.message,sizeof(detail));redraw();
}
static void pulltext(void) {GetWindowText(editor,source,sizeof(source));}
static void puttext(char *s) {
    changing=1;SetWindowText(editor,s);changing=0;
}
static int nextpart(unsigned part) {
    pending[part]=mml_next(&iter[part],&events[part],&error);
    if(pending[part]<0){stop();showerror();return 0;}return 1;
}
static int emit(MML_EVENT *e) {
    unsigned s=0,a=0,b=0,c,i,oldNotes[3],oldPeriods[3];DWORD r;char line[96];
    if(e->part==MML_PIT_PART){
        if(e->kind==MML_NOTEON){pitDesired=e->pitch;pitAttack=e->when_ms;}
        if(e->kind==MML_NOTEOFF)pitDesired=0;
        return 1;
    }
    c=e->part==3?9:e->part;
    if(e->kind==MML_PROGRAM){s=0xc0+c;a=e->program;}
    if(e->kind==MML_NOTEON){s=0x90+c;a=e->pitch;b=e->velocity;}
    if(e->kind==MML_NOTEOFF){s=0x80+c;a=e->pitch;}
    if(!s)return 1;
    if(automatic==6&&e->part==3&&debug)
        for(i=0;i<3;++i){oldNotes[i]=debug(4+i);oldPeriods[i]=debug(8+i);}
    r=message(MODM_DATA,(DWORD)s|((DWORD)a<<8)|((DWORD)b<<16));
    if(r){stop();setstatus("Sound send failed");return 0;}++sent;
    if(automatic==6&&e->part==3&&debug){
        if(e->kind==MML_NOTEON){
            check("noise attack mapped",debug(7)==e->pitch);
            i=(e->pitch==35||e->pitch==36)?8:
              (e->pitch==42||e->pitch==44||e->pitch==46)?10:9;
            check("noise envelope snapshot",debug(51)==i);
        }
        if(e->kind==MML_NOTEOFF)check("noise off immediate",debug(7)==0);
        check("noise preserves all tones",
            debug(4)==oldNotes[0]&&debug(5)==oldNotes[1]&&
            debug(6)==oldNotes[2]&&debug(8)==oldPeriods[0]&&
            debug(9)==oldPeriods[1]&&debug(10)==oldPeriods[2]);
    }
    if(automatic){wsprintf(line,"EVENT %lu %u %u %u",e->when_ms,s,a,b);logtext(line);}
    return 1;
}
static void pump(DWORD now) {
    int i,best,count=0;DWORD elapsed;
    if(!playing)return;
    elapsed=now-startTick;position=elapsed<duration?elapsed:duration;
    while(playing&&count<64){
        best=-1;
        for(i=0;i<(int)score.part_count;++i)if(pending[i]>0&&
            (best<0||events[i].when_ms<events[best].when_ms||
            (events[i].when_ms==events[best].when_ms&&
            events[i].kind==MML_NOTEOFF&&events[best].kind!=MML_NOTEOFF)))best=i;
        if(best<0){playing=0;release();++completions;
            position=duration;setstatus("Finished / muted");break;}
        if(events[best].when_ms>elapsed)break;
        if(elapsed-events[best].when_ms>1000UL){
            stop();setstatus("Timing stalled");break;
        }
        if(!emit(&events[best])||!nextpart(best))break;
        ++count;
    }
    /* Emit P only after evaluating all states due now. Never play an
       expired attack burst while a cooperative catch-up is incomplete. */
    if(playing && score.present[MML_PIT_PART]){
        int duePending=0;DWORD value;
        for(i=0;i<(int)score.part_count;++i)
            if(pending[i]>0 && events[i].when_ms<=elapsed)duePending=1;
        if(duePending || !pitDesired || pitDesired!=pitSent ||
           pitAttack!=pitSentAttack){
            if(pitSent){value=0x8fUL|((DWORD)pitSent<<8);
                if(message(MODM_DATA,value)){stop();return;}}
            pitSent=0;
            if(!duePending && pitDesired){
                value=0x9fUL|((DWORD)pitDesired<<8)|(1UL<<16);
                if(message(MODM_DATA,value)){stop();return;}
                pitSent=pitDesired;pitSentAttack=pitAttack;
            }
        }
    }
    if(playing&&message(MODM_SYNTHUPDATE,0L)){
        stop();setstatus("Synth update failed");
    }
    if(now-paintTick>=500UL){paintTick=now;redraw();}
}
static void play(void) {
    int i;stop();pulltext();sent=0;position=duration=0;
    if(!mml_validate(&score,source,lstrlen(source),programs,&error)){
        showerror();return;
    }
    if(score.present[MML_PIT_PART] && !pitEnabled){
        setstatus("Enable PIT for [P]");return;
    }
    duration=score.duration_ms;
    for(i=0;i<(int)score.part_count;++i){mml_reset(&iter[i],&score,i);if(!nextpart(i))return;}
    if(!timerReady){setstatus("Timer unavailable");return;}
    /* Finish pending edit redraws before owning sound or starting the clock. */
    UpdateWindow(editor);UpdateWindow(window);
    if(!acquire())return;
    lstrcpy(detail,"Esc = Panic");setstatus("Playing MML");
    UpdateWindow(window);
    startTick=GetTickCount();paintTick=startTick;playing=1;pump(startTick);
}
static int discard(void) {
    if(!dirty)return 1;
    return MessageBox(window,"Discard unsaved edits?","MML Fiddle",
        MB_YESNO|MB_ICONQUESTION)==IDYES;
}
static int filenameok(void) {
    unsigned n;GetDlgItemText(window,FILEBOX,filename,sizeof(filename));
    n=lstrlen(filename);
    if(n<5||n>134||lstrcmpi(filename+n-4,".MML")){
        setstatus("Use a .MML name");return 0;
    }
    return 1;
}
static int readfile(int ask) {
    HFILE f;OFSTRUCT of;LONG length;unsigned n,i;
    stop();if(ask&&!discard())return 0;if(!filenameok())return 0;
    f=OpenFile(filename,&of,OF_READ|OF_SHARE_DENY_WRITE);
    if(f==HFILE_ERROR){setstatus("Cannot open file");return 0;}
    length=_llseek(f,0L,2);_llseek(f,0L,0);
    if(length<0){_lclose(f);setstatus("Cannot read size");return 0;}
    if(length>8192L){
        n=_lread(f,scratch,8192);_lclose(f);
        if(n!=8192){setstatus("Read failed");return 0;}
        scratch[8192]=0;fileerror(8193);return 0;
    }
    n=_lread(f,scratch,(unsigned)length);_lclose(f);
    if(n!=(unsigned)length){setstatus("Read failed");return 0;}
    for(i=0;i<n;++i)if(!scratch[i]||(unsigned char)scratch[i]>127){
        fileerror(n);return 0;
    }
    scratch[n]=0;puttext(scratch);dirty=0;
    setstatus("Opened .MML");lstrcpy(detail,"Edit, then Play");redraw();return 1;
}
static int savefile(int ask) {
    HFILE f;OFSTRUCT of;unsigned n;int exists;char temp[144],backup[144];
    stop();if(!filenameok())return 0;pulltext();n=lstrlen(source);
    exists=OpenFile(filename,&of,OF_EXIST)!=HFILE_ERROR;
    if(exists&&ask&&MessageBox(window,"Replace this .MML file?",
        "MML Fiddle",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;
    textcopy(temp,filename,sizeof(temp));textcopy(backup,filename,sizeof(backup));
    temp[lstrlen(temp)-4]=0;lstrcat(temp,".$ML");
    backup[lstrlen(backup)-4]=0;lstrcat(backup,".$BK");
    if(OpenFile(temp,&of,OF_EXIST)!=HFILE_ERROR||
        OpenFile(backup,&of,OF_EXIST)!=HFILE_ERROR){
        setstatus("Temp file exists");return 0;
    }
    f=_lcreat(temp,0);
    if(f==HFILE_ERROR){setstatus("Cannot create file");return 0;}
    if(_lwrite(f,source,n)!=n){_lclose(f);remove(temp);setstatus("Save failed");return 0;}
    if(_lclose(f)){remove(temp);setstatus("Close failed");return 0;}
    if(exists&&rename(filename,backup)){
        remove(temp);setstatus("Cannot replace");return 0;
    }
    if(rename(temp,filename)){
        if(exists)rename(backup,filename);remove(temp);
        setstatus("Rename failed");return 0;
    }
    if(exists&&remove(backup)){setstatus("Saved; backup kept");dirty=0;return 1;}
    dirty=0;setstatus("Saved .MML");return 1;
}
static void selectpart(int p) {
    unsigned i,n;int line=1,target,lineCount;char c,wanted;selected=p;
    wanted=(char)(p==4?'P':p==3?'N':'A'+p);
    SetDlgItemText(window,PARTA,p==0?"*A":"A");
    SetDlgItemText(window,PARTB,p==1?"*B":"B");
    SetDlgItemText(window,PARTC,p==2?"*C":"C");
    SetDlgItemText(window,PARTN,p==3?"*N":"N");
    SetDlgItemText(window,PARTP,p==4?"*P":"P");
    if(p>=3){
        if(SendDlgItemMessage(window,PRESET,CB_GETCOUNT,0,0L)==9)
            SendDlgItemMessage(window,PRESET,CB_DELETESTRING,8,0L);
        if(SendDlgItemMessage(window,PRESET,CB_GETCOUNT,0,0L)==8)
            SendDlgItemMessage(window,PRESET,CB_ADDSTRING,0,(LPARAM)(LPSTR)(p==4?"PIT":"Drum"));
        SendDlgItemMessage(window,PRESET,CB_SETCURSEL,8,0L);
        EnableWindow(GetDlgItem(window,PRESET),FALSE);
    }else{
        if(SendDlgItemMessage(window,PRESET,CB_GETCOUNT,0,0L)==9)
            SendDlgItemMessage(window,PRESET,CB_DELETESTRING,8,0L);
        EnableWindow(GetDlgItem(window,PRESET),TRUE);
        SendDlgItemMessage(window,PRESET,CB_SETCURSEL,programs[p]/16,0L);
    }
    pulltext();n=lstrlen(source);
    for(i=0;i<n;++i){
        c=source[i];if(line&&(c==' '||c=='\t'||c=='\r'))continue;
        if(line&&c=='['&&i+2<n&&
            (source[i+1]==wanted||source[i+1]==(char)(wanted+32))&&source[i+2]==']'){
            SendMessage(editor,WM_SETREDRAW,FALSE,0L);
            SendMessage(editor,EM_SETSEL,0,MAKELONG(i,i+3));
            target=(int)SendMessage(editor,EM_LINEFROMCHAR,i,0L);
            /* Win3.0 has no EM_GETFIRSTVISIBLELINE. Clamp to the top,
               then scroll by the wrapped target line. Win16 WINDOWS X
               Edit_Scroll packs vertical delta in LOWORD, horizontal HIGH. */
            lineCount=(int)SendMessage(editor,EM_GETLINECOUNT,0,0L);
            SendMessage(editor,EM_LINESCROLL,0,MAKELONG(-lineCount,0));
            SendMessage(editor,EM_LINESCROLL,0,MAKELONG(target,0));
            SendMessage(editor,WM_SETREDRAW,TRUE,0L);
            InvalidateRect(editor,NULL,TRUE);SetFocus(editor);
            if(automatic==7)check("selected section line valid",
                target>=0&&SendMessage(editor,EM_GETLINECOUNT,0,0L)>(LONG)target);
            return;
        }
        line=c=='\n';
    }
    setstatus("Part not in source");
}
static HWND control(char *kind,char *label,DWORD style,int x,int y,
    int width,int height,int id) {
    HWND h=CreateWindow(kind,label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|style,
        x,y,width,height,window,(HMENU)id,instance,NULL);
    if(h)SendMessage(h,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);
    return h;
}
static void paint(HWND w) {
    PAINTSTRUCT ps;HDC dc;char b[20];unsigned i,j;
    dc=BeginPaint(w,&ps);SelectObject(dc,GetStockObject(SYSTEM_FIXED_FONT));
    SetBkColor(dc,RGB(255,255,255));SetTextColor(dc,RGB(0,0,0));
    for(j=0;j<2;++j){textcopy(b,j?detail:status,19);
        i=lstrlen(b);while(i<18)b[i++]=' ';b[18]=0;
        TextOut(dc,3,149+j*12,b,18);}
    EndPaint(w,&ps);
}
static void pitTestTick(DWORD now)
{
    int before;
    static char scoreP[]="MML3\r\n[A]\r\nC2\r\n[B]\r\nE2\r\n"
        "[C]\r\nG2\r\n[N]\r\nN38/4 R4\r\n[P]\r\nC8 C8 D4\r\n";
    if(!stage && now-beginTick>500UL){
        puttext(automatic==9 || automatic>=16?"MML3\r\n[P]\r\nR4\r\n":scoreP);
        pitEnabled=automatic!=9;
        SetDlgItemText(window,PIT,pitEnabled?"*PIT":"PIT");
        if(automatic==8){
            SetDlgItemText(window,FILEBOX,"C:\\PIT.MML");
            check("MML3 native save",savefile(0));
            check("MML3 native reopen",readfile(0));
            pulltext();check("MML3 bytes preserved",!lstrcmp(source,scoreP));
        }
        before=debug?debug(2):0;
        if(automatic==12){
            check("P busy guard lease",OpenSound()>=0);play();
            check("P busy refusal",!playing&&!opened);
            check("P busy no PSG writes",debug&&debug(2)==before);
            CloseSound();stage=2;return;
        }
        play();
        if(automatic==9 || automatic==10 || automatic==17){
            check("P required refusal before ownership",!playing&&!opened);
            check("P refusal no PSG writes",debug&&debug(2)==before);
            stage=2;return;
        }
        check(automatic==16?"all-rest P owns capability":"all five generators",
            playing&&opened&&debug&&debug(0)==(automatic==16?0:31));
        play();check("P replay rearms",playing&&debug&&debug(0)==(automatic==16?0:31));
        selectpart(4);check("P no preset",!IsWindowEnabled(GetDlgItem(window,PRESET)));
        selectpart(0);stage=1;return;
    }
    if(stage==1 && now-startTick>175UL){
        if(automatic==13){
            PostMessage(editor,WM_KEYDOWN,VK_ESCAPE,0L);stage=4;return;
        }
        if(automatic==15){dirty=0;PostMessage(window,WM_CLOSE,0,0L);stage=3;return;}
        if(automatic==11)SendMessage(window,WM_ACTIVATE,0,0L);
        else if(automatic==14)pump(startTick+2000UL);
        else stop();
        check("P interrupted cleanup",!playing&&!opened&&debug&&debug(0)==0);
        stop();check("P repeated cleanup",!opened&&debug&&debug(1)==0);
        stage=2;return;
    }
    if(stage==4){
        check("P Escape queue cleanup",!playing&&!opened&&debug&&debug(0)==0);
        stage=2;return;
    }
    if(stage==2){dirty=0;PostMessage(window,WM_CLOSE,0,0L);stage=3;}
}
static void testtick(DWORD now) {
    HWND dialog;char **testFiles;DWORD *testTimes;int total,i;
    if(automatic>=8){pitTestTick(now);return;}
    if(automatic==5&&(stage==10||stage==12||stage==14)){
        dialog=FindWindow("#32770","MML Fiddle");
        if(dialog){++stage;PostMessage(dialog,WM_COMMAND,IDNO,0L);}
        return;
    }
    if(automatic==5&&stage==0&&now-beginTick>=500UL){
        SetDlgItemText(window,FILEBOX,"C:\\DIALOG.MML");
        check("prepare dialog file",savefile(0));dirty=1;
        stage=10;SendMessage(window,WM_CLOSE,0,0L);
        check("cancel close preserves edits",IsWindow(window)&&dirty);
        stage=12;check("cancel open",!readfile(1)&&dirty);
        stage=14;check("cancel replace",!savefile(1)&&dirty);
        dirty=0;stage=16;PostMessage(window,WM_CLOSE,0,0L);return;
    }
    if(automatic==5)return;
    if(automatic==4||automatic==6){
        testFiles=automatic==4?fixtures:noiseFixtures;
        testTimes=automatic==4?fixtureTimes:noiseTimes;
        total=automatic==4?4:6;
        if(!stage&&now-beginTick>=500UL){
            lstrcpy(filename,appdir);lstrcat(filename,testFiles[0]);
            SetDlgItemText(window,FILEBOX,filename);
            check("open fixture",readfile(0));play();
            check("fixture exact duration",duration==testTimes[0]);stage=1;
        }else if(stage>=1&&stage<=total&&completions){
            check("fixture completed",!opened);completions=0;
            if(stage==total){stage=total+1;PostMessage(window,WM_CLOSE,0,0L);return;}
            lstrcpy(filename,appdir);lstrcat(filename,testFiles[stage]);
            SetDlgItemText(window,FILEBOX,filename);
            check("open fixture",readfile(0));play();
            check("fixture exact duration",duration==testTimes[stage]);++stage;
        }
        return;
    }
    if(!automatic||now-beginTick<500UL)return;
    if(!stage){
        if(automatic==7)stage=1;
        check("editor created",editor!=0);
        check("eight presets",SendDlgItemMessage(window,PRESET,CB_GETCOUNT,0,0L)==8);
        check("timer ready",timerReady);
        if(automatic==2){
            SetDlgItemText(window,FILEBOX,"C:\\ROUND.MML");
            check("save score",savefile(0));puttext("[A]\r\nR4\r\n");
            check("open score",readfile(0));pulltext();
            check("exact text roundtrip",!lstrcmp(source,demo));
            check("repeat save",savefile(0));check("repeat open",readfile(0));
            selectpart(1);check("select B",selected==1);selectpart(2);
            check("select C",selected==2);selectpart(3);
            check("select N",selected==3);
            check("noise has no preset",!IsWindowEnabled(GetDlgItem(window,PRESET)));
            selectpart(0);check("tone preset restored",
                IsWindowEnabled(GetDlgItem(window,PRESET))&&
                SendDlgItemMessage(window,PRESET,CB_GETCOUNT,0,0L)==8);
            puttext("[A]\r\nO1 C\r\n");play();
            check("error before sound ownership",!opened&&!playing&&error.line==2);
            SetDlgItemText(window,FILEBOX,"C:\\ASCII.MML");
            check("non-ASCII file rejected",!readfile(0)&&error.line==2);
            SetDlgItemText(window,FILEBOX,"C:\\OVERSIZE.MML");
            check("oversize file rejected",!readfile(0)&&error.offset==8192);
            SetDlgItemText(window,FILEBOX,"C:\\MISSING.MML");
            check("missing file rejected",!readfile(0)&&!opened);
            puttext((char *)demo);play();play();
            check("repeat play",opened&&playing);stop();stop();
            check("repeat stop silent",!opened&&debug&&debug(0)==0);
            play();stop();check("Panic releases",!opened&&debug&&debug(1)==0);
            check("SOUND lease acquired",OpenSound()>=0);
            i=debug?debug(2):0;play();
            check("busy makes no PSG writes",debug&&debug(2)==(unsigned)i);
            check("busy refuses",!opened&&!playing);CloseSound();play();
            check("retry after busy",opened&&playing);
        }else{if(automatic==7)selectpart(3);play();}
        stage=1;return;
    }
    if(automatic==3&&stage==1&&playing){
        SendMessage(window,WM_ACTIVATE,0,0L);
        check("deactivate releases",!opened&&!playing);play();
        check("close during playback",opened&&playing);dirty=0;
        PostMessage(window,WM_CLOSE,0,0L);stage=2;return;
    }
    if(automatic==7&&stage==1&&!playing&&!completions){
        check("preview playback completed",0);dirty=0;stage=2;
        PostMessage(window,WM_CLOSE,0,0L);return;
    }
    if(completions){check("playback completed",sent>0&&!opened);
        dirty=0;PostMessage(window,WM_CLOSE,0,0L);automatic=0;}
}
LONG FAR PASCAL WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp) {
    int i;DWORD now;
    switch(m){
    case WM_CREATE:
        window=w;
        if(!control("EDIT",filename,WS_BORDER|ES_AUTOHSCROLL,3,2,142,18,FILEBOX))return -1;
        SendDlgItemMessage(w,FILEBOX,EM_LIMITTEXT,134,0L);
        if(!control("BUTTON","Open",0,3,23,40,18,OPEN)||
            !control("BUTTON","Save",0,45,23,40,18,SAVE))return -1;
        for(i=0;i<5;++i)if(!control("BUTTON",i==0?"*A":i==1?"B":i==2?"C":i==3?"N":"P",0,
            3+i*26,44,26,18,i==4?PARTP:i==3?PARTN:PARTA+i))return -1;
        if(!control("COMBOBOX","",CBS_DROPDOWNLIST|WS_VSCROLL,
            89,23,56,100,PRESET))return -1;
        for(i=0;i<8;++i)SendDlgItemMessage(w,PRESET,CB_ADDSTRING,0,(LPARAM)(LPSTR)names[i]);
        SendDlgItemMessage(w,PRESET,CB_SETCURSEL,0,0L);
        editor=control("EDIT","",WS_BORDER|WS_VSCROLL|ES_MULTILINE|
            ES_AUTOVSCROLL,3,66,142,58,EDITOR);
        if(!editor)return -1;SendMessage(editor,EM_LIMITTEXT,8192,0L);puttext((char *)demo);
        if(!control("BUTTON","Play",0,3,128,46,18,PLAY)||
            !control("BUTTON","Stop",0,51,128,46,18,STOP)||
            !control("BUTTON","PIT",0,99,128,46,18,PIT))return -1;
        timerReady=SetTimer(w,PULSE,55,NULL)!=0;if(!timerReady)return -1;
        beginTick=GetTickCount();return 0;
    case WM_COMMAND:
        if(wp==EDITOR&&HIWORD(lp)==EN_CHANGE&&!changing){
            if(playing)stop();dirty=1;setstatus("Edited / stopped");
        }
        if(wp==EDITOR&&HIWORD(lp)==EN_ERRSPACE)setstatus("Editor memory full");
        if(wp==OPEN)readfile(1);if(wp==SAVE)savefile(1);
        if(wp==PLAY)play();if(wp==STOP)stop();
        if(wp>=PARTA&&wp<=PARTC)selectpart(wp-PARTA);
        if(wp==PARTN)selectpart(3);
        if(wp==PARTP)selectpart(4);
        if(wp==PIT){stop();pitEnabled=!pitEnabled;
            SetDlgItemText(w,PIT,pitEnabled?"*PIT":"PIT");
            setstatus(pitEnabled?"PIT enabled":"PIT disabled");}
        if(wp==PRESET&&HIWORD(lp)==CBN_SELCHANGE&&selected<3){
            stop();i=(int)SendDlgItemMessage(w,PRESET,CB_GETCURSEL,0,0L);
            if(i>=0&&i<8)programs[selected]=(unsigned char)(i*16);
            setstatus("Initial preset set");lstrcpy(detail,"@ commands override");
        }
        return 0;
    case WM_TIMER:if(wp==PULSE){now=GetTickCount();pump(now);testtick(now);}return 0;
    case WM_ACTIVATE:if(!wp&&playing)stop();return 0;
    case WM_PAINT:paint(w);return 0;
    case WM_QUERYENDSESSION:stop();return discard();
    case WM_ENDSESSION:if(wp)stop();return 0;
    case WM_CLOSE:stop();if(discard())DestroyWindow(w);return 0;
    case WM_DESTROY:KillTimer(w,PULSE);timerReady=0;stop();window=0;PostQuitMessage(0);return 0;
    }
    return DefWindowProc(w,m,wp,lp);
}
int PASCAL WinMain(HINSTANCE inst,HINSTANCE previous,LPSTR cmd,int show) {
    WNDCLASS wc;MSG msg;HWND w;OFSTRUCT of;char path[144],line[96];int i,result=0,testmode;
    if(previous){w=FindWindow("TandyMMLFiddle",NULL);
        if(w){ShowWindow(w,SW_RESTORE);BringWindowToTop(w);}return 1;}
    if(GetWinFlags()&WF_PMODE)return 2;instance=inst;
    if(!lstrcmp(cmd,"/pittest"))automatic=8;
    if(!lstrcmp(cmd,"/pitoff"))automatic=9;
    if(!lstrcmp(cmd,"/pitold"))automatic=10;
    if(!lstrcmp(cmd,"/pitfocus"))automatic=11;
    if(!lstrcmp(cmd,"/pitbusy"))automatic=12;
    if(!lstrcmp(cmd,"/pitescape"))automatic=13;
    if(!lstrcmp(cmd,"/pitstall"))automatic=14;
    if(!lstrcmp(cmd,"/pitclose"))automatic=15;
    if(!lstrcmp(cmd,"/pitrest"))automatic=16;
    if(!lstrcmp(cmd,"/pitrestold"))automatic=17;
    if(!lstrcmp(cmd,"/test"))automatic=1;
    if(!lstrcmp(cmd,"/uitest"))automatic=2;
    if(!lstrcmp(cmd,"/closetest"))automatic=3;
    if(!lstrcmp(cmd,"/fixtures"))automatic=4;
    if(!lstrcmp(cmd,"/dialogs"))automatic=5;
    if(!lstrcmp(cmd,"/noise"))automatic=6;
    if(!lstrcmp(cmd,"/noisepreview"))automatic=7;testmode=automatic;
    if(automatic)logfile=_lcreat("C:\\MMLFIDL.LOG",0);
    i=GetModuleFileName(inst,appdir,sizeof(appdir));appdir[143]=0;
    if(i<=0||i>=143){result=3;goto cleanup;}
    for(i=lstrlen(appdir)-1;i>=0;--i)if(appdir[i]=='\\')break;
    if(i<0||i>130){result=3;goto cleanup;}appdir[i+1]=0;
    lstrcpy(filename,appdir);lstrcat(filename,"SONG.MML");
    lstrcpy(path,appdir);lstrcat(path,"MIDIMAP.DRV");
    if(OpenFile(path,&of,OF_EXIST)!=HFILE_ERROR)module=LoadLibrary(path);
    if(module>=32){midi=(MIDIPROC)GetProcAddress(module,"MIDIMESSAGE");
        debug=(DEBUGPROC)GetProcAddress(module,"DEBUGSTATE");}
    wc.style=CS_HREDRAW|CS_VREDRAW;wc.lpfnWndProc=WndProc;
    wc.cbClsExtra=wc.cbWndExtra=0;wc.hInstance=inst;wc.hIcon=NULL;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName=NULL;wc.lpszClassName="TandyMMLFiddle";
    if(!RegisterClass(&wc)){result=4;goto cleanup;}
    w=CreateWindow("TandyMMLFiddle","MML Fiddle",WS_OVERLAPPED|WS_CAPTION|
        WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN,0,0,156,200,NULL,NULL,inst,NULL);
    if(!w){result=5;goto cleanup;}ShowWindow(w,show);UpdateWindow(w);
    if(!testmode&&cmd[0]){
        textcopy(filename,cmd,sizeof(filename));
        if(filename[0]=='"'){
            i=lstrlen(filename);
            if(i>1&&filename[i-1]=='"'){
                filename[i-1]=0;
                for(i=0;filename[i+1];++i)filename[i]=filename[i+1];
                filename[i]=0;
            }
        }
        SetDlgItemText(w,FILEBOX,filename);readfile(0);
    }
    while(GetMessage(&msg,NULL,0,0)){
        if(msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){stop();continue;}
        if(msg.message==WM_KEYDOWN&&msg.wParam==VK_TAB){
            HWND next=GetNextDlgTabItem(w,GetFocus(),GetKeyState(VK_SHIFT)<0);
            if(next)SetFocus(next);continue;
        }
        TranslateMessage(&msg);DispatchMessage(&msg);
    }
cleanup:
    stop();check("exit has no owner",!opened);
    if(module>=32)FreeLibrary(module);
    wsprintf(line,"TOTAL_FAILURES=%d SENT=%u",failures,sent);logtext(line);
    if(logfile!=HFILE_ERROR)_lclose(logfile);
    if(testmode)ExitWindows(0L,0);return result?result:(failures?6:0);
}
