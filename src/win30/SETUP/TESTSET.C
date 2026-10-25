#define SETUP_HOST
#define main setup_main
#include "SETUP.C"
#undef main
static unsigned tests;
static void check(int yes,const char *name) {
    ++tests;
    if(!yes) { printf("FAILED: %s\n",name); exit(1); }
}
static void initdir(const char *root) {
    char p[PLEN];
    check(strlen(root)+32<PLEN,"test path bound");
    strcpy(app,root); strcat(app,"\\APP");
    strcpy(win,root); strcat(win,"\\WINDOWS");
    mkdir(app); mkdir(win);
    join(p,win,"SYSTEM"); mkdir(p);
    check(prepare(),"prepare isolated directories");
}
static void put32(unsigned char *p,unsigned long value) {
    unsigned i;
    for(i=0;i<4;i++) { p[i]=(unsigned char)value; value>>=8; }
}
#include "SFXTEST.H"
#include "ROLLTEST.H"
static void launchscriptTests(const char *root) {
    char go[PLEN],state[PLEN],list[PLEN],text[128];
    unsigned scenario;
    for(scenario=0;scenario<3;scenario++) {
        strcpy(app,root);strcat(app,"\\GOAPP");
        strcpy(win,root);strcat(win,"\\WINDOWS");
        nfiles=nitems=nclients=0;validateinstall=0;failafter=-1;
        check(mkdir(app)==0,"prepare current GO uninstall fixture");
        if(scenario==1) app[0]=(char)tolower((unsigned char)app[0]);
        launchscript(edited,app,win);
        join(go,app,"GO.BAT");statepath(state);join(list,app,"PAYLOAD.LST");
        check(putfile(go,edited)&&writestate(state)&&putfile(list,"current\r\n"),
              "write generated GO and current ownership state");
        if(scenario==1) app[0]=(char)toupper((unsigned char)app[0]);
        if(scenario==2)
            check(putfile(go,"@ECHO OFF\r\nECHO CUSTOM\r\n"),
                  "customize current GO without changing installer state");
        check(uninstall(),"current Uninstall completes with generated or changed GO");
        check(!exists(state)&&!exists(list)&&!exists(stage),
              "Uninstall removes ownership/private files");
        if(scenario==2) {
            check(readtext(go,text,sizeof(text))&&
                  !strcmp(text,"@ECHO OFF\r\nECHO CUSTOM\r\n"),
                  "Uninstall preserves customized GO byte for byte");
            remove(go);rmdir(app);
        } else {
            check(!exists(go)&&!exists(app),
                  "Uninstall removes unchanged GO using recorded path spelling");
        }
    }
}
int main(int argc,char **argv) {
    char prev[128],a[PLEN],b[PLEN],out[PLEN];
    const char *input="; keep\r\n[boot]\r\nsystem.drv=SYSTEM.DRV\r\n"
        "sound.drv=TSOUND.DRV ; old\r\ndisplay.drv=XRSYNC.DRV\r\n"
        "shell=PROGMAN.EXE\r\n[other]\r\nfoo = bar\r\n";
    if(argc!=2) return 2;
    sfxchecks();
    check(changeini(input,edited,"SOUND.DRV",prev,1),"change INI");
    check(equal(prev,"TSOUND.DRV"),"old selection captured");
    check(strstr(edited,"sound.drv=SOUND.DRV\r\n")!=0,"canonical selection");
    check(strstr(edited,"shell=PROGMAN.EXE\r\n[other]\r\nfoo = bar\r\n")!=0,
          "unrelated byte preservation");
    strcpy(ini,"[boot]\nsystem.drv=SYSTEM.DRV\ndisplay.drv=CGA.DRV\n"
               "sound.drv=ONE.DRV\nsound.drv=TWO.DRV\n[more]\nx=y\n");
    check(changeini(ini,edited,"SOUND.DRV",prev,1),"coalesce duplicate key");
    check(strstr(strstr(edited,"sound.drv=")+1,"sound.drv=")==0,
          "only one sound key");
    check(equal(prev,"ONE.DRV"),"duplicate old first selection");
    check(changeini("[boot]\nsystem.drv=SYSTEM.DRV\ndisplay.drv=CGA.DRV",
                    edited,"SOUND.DRV",prev,1),"append missing key");
    check(strstr(edited,"CGA.DRV\r\nsound.drv=SOUND.DRV\r\n")!=0,
          "repair missing final newline");
    check(changeini(input,edited,0,prev,1),"remove originally missing key");
    check(strstr(edited,"sound.drv=")==0,"key removed on uninstall");
    check(!changeini("[boot]\nsystem.drv=SYSTEM.DRV\ndisplay.drv=BAD.DRV\n",
                    edited,"SOUND.DRV",prev,1),"unknown display refused");
    check(!changeini("[boot]\nsystem.drv=OTHER.DRV\ndisplay.drv=CGA.DRV\n",
                    edited,"SOUND.DRV",prev,1),"wrong system refused");
    check(!changeini("[boot]\n[boot]\n",edited,"SOUND.DRV",prev,1),
          "duplicate boot refused");
    check(!changeini("[boot]garbage\n",edited,"SOUND.DRV",prev,0),
          "malformed section refused");
    check(absolute("C:\\WINDOWS")&&absolute("D:\\APPS\\WINAMP"),
          "DOS paths accepted");
    check(!absolute("C:\\WINDOWS\\")&&!absolute("C:\\..\\WINDOWS")&&
          !absolute("C:\\BAD NAME")&&!absolute("C:\\LONGNAMEX"),
          "ambiguous paths refused");
    check(basename83("RAWCLK.DLL")&&!basename83("..")&&
          !basename83("A\\B.DRV"),"flat package paths");
    check(!basename83("CON")&&!basename83("COM1.DRV")&&
          !basename83("LPT9.TXT")&&!basename83("CLOCK$"),
          "DOS device files refused");
    check(join(a,"C:\\","WINDOWS")&&equal(a,"C:\\WINDOWS"),
          "root discovery path has one separator");
    files[0].kind=xtclients[0].kind;
    strcpy(files[0].name,xtclients[0].name);
    strcpy(files[0].source,xtclients[0].name);
    files[0].bytes=xtclients[0].bytes; files[0].crc=xtclients[0].crc;
    check(clientpin(&files[0])==1,"current client pin accepted");
    files[0].crc^=1;
    check(!clientpin(&files[0]),"changed current client pin refused");
    files[0].crc^=1;
    strcpy(win,"D:\\WINDOWS");
    check(clientpath(&files[0],a,0)&&equal(a,"D:\\WINDOWS\\TSHELL.EXE"),
          "Windowsroot client path uses selected Windows");
    check(clientpath(&files[0],a,1)&&equal(a,"D:\\WINXT\\FILES\\TSHELL.EXE"),
          "client mirror path follows selected drive");
    strcpy(files[1].name,"TSINPUT.DLL"); files[1].kind='W';
    check(inputpolicy(&files[1],0,1)==1,
          "current TSHELL requires missing paired TSINPUT");
    check(inputpolicy(&files[1],0,0)==-1,
          "stock Windows skips unrequested global TSINPUT");
    check(inputpolicy(&files[1],1,1)==0,
          "TSINPUT mirror uses existing target policy");
    check(mirrorsoundpolicy(0,1,0)==0,
          "stock Windows leaves unrelated sound mirror namespace alone");
    check(mirrorsoundpolicy(1,0,0)==1&&mirrorsoundpolicy(1,1,1)==1,
          "recognized current FIXXT receives canonical sound mirror");
    check(mirrorsoundpolicy(1,1,0)<0,
          "first current kit install preserves foreign sound mirror");
    check(mirrorsoundpolicy(2,0,0)==1&&mirrorsoundpolicy(2,1,0)==1,
          "recorded current kit repairs missing or corrupt sound mirror");
    initdir(argv[1]);
    sprintf(edited,"twstage1\r\n%s\r\n",app);
    join(a,stage,"OWNER.DAT");
    check(putfile(a,edited)&&stageowned(),"DOS case-insensitive stage marker");
    join(a,app,"OLD.TXT"); join(b,stage,"GENERATE.TMP");
    check(putfile(a,"old"),"write previous installed file");
    check(putfile(b,"new"),"write new staging input");
    check(add(a,b,0),"stage replacement and recovery copy");
    join(out,app,"NEW.TXT");
    check(add(out,b,0),"stage new file");
    failafter=1;
    check(!commit(),"injected failure returns failure");
    check(matches(a,3UL,0x3f5dd4e5UL),"replacement rolled back");
    check(!exists(out),"new file absent after rollback");
    check(!exists(stage),"rollback staging cleaned");
    nitems=0; failafter=-1;
    check(prepare(),"prepare successful retry");
    join(b,stage,"GENERATE.TMP"); check(putfile(b,"new"),"retry input");
    check(add(a,b,0)&&add(out,b,0),"retry staged");
    check(commit(),"retry committed");
    check(matches(a,3UL,0x6be34445UL)&&matches(out,3UL,0x6be34445UL),
          "committed identities");
    check(!exists(stage),"commit journal cleaned");
    join(b,app,"TWBACK");
    check(!exists(b),"successful current operation has no permanent backup directory");
    nitems=0; check(prepare(),"prepare interrupted write");
    join(b,stage,"GENERATE.TMP"); check(putfile(b,"next"),"interrupted input");
    check(add(a,b,0)&&journal(),"durable journal before write");
    check(putfile(a,"torn partial"),"simulate interrupted write");
    win[0]=0;
    check(rollback(),"recover without readable Windows INI or state");
    check(matches(a,3UL,0x6be34445UL),"interrupted file restored");
    check(clearstage(),"recovery staging removed");
    check(prepare(),"prepare foreign staging preservation test");
    join(b,stage,"NOTES.TXT"); check(putfile(b,"foreign"),"foreign fixture");
    check(!clearstage()&&exists(b)&&stageowned(),
          "unknown staging data and ownership marker preserved");
    check(remove(b)==0&&clearstage(),"remove fixture and retry cleanup");
    check(mkdir(stage)==0,"create foreign unmarked stage");
    join(b,stage,"NOTES.TXT"); check(putfile(b,"foreign"),"unmarked fixture");
    check(!prepare()&&!clearstage()&&exists(b),
          "unmarked nonempty staging refused without deletion");
    remove(b); rmdir(stage);
    nfiles=1;
    nclients=2;
    clientowned[0].which=clientowned[1].which=0;
    clientowned[0].mirror=0; clientowned[1].mirror=1;
    clientowned[0].bytes=clientowned[1].bytes=xtclients[0].bytes;
    clientowned[0].crc=clientowned[1].crc=xtclients[0].crc;
    statepath(out); check(writestate(out),"persistent state written");
    check(readstate()==1&&instbytes==SOUND_BYTES&&
          instcrc==SOUND_CRC&&nowned==1&&
          equal(ownedfiles[0].name,"TSHELL.EXE"),
          "state reads only current driver and owned app identities");
    check(nclients==2&&clientrecord(0,0)==1&&clientrecord(0,1)==2&&
          clientowned[0].bytes==xtclients[0].bytes,
          "only recorded current client targets roundtrip");
    clientowned[1].mirror=0;
    check(writestate(out)&&readstate()<0,"duplicate client target state refused");
    remove(out);
    join(out,app,"NEW.TXT");
    remove(a); remove(out); rmdir(app);
    join(a,win,"SYSTEM"); rmdir(a); rmdir(win);
    rollbackchecks(argv[1]);
    launchscriptTests(argv[1]);
    printf("PASS: %u installer core checks\n",tests); return 0;
}
