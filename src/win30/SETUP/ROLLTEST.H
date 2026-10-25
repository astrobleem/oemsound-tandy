/* Real journal paths, redirected only by the host filesystem test shim. */
static void rollbackchecks(const char *root) {
    char source[PLEN],newfile[PLEN],state[PLEN];
    strcpy(app,root); strcat(app,"\\APPFAULT");
    strcpy(win,root); strcat(win,"\\WINDOWS");
    strcpy(host_xtroot,root); strcat(host_xtroot,"\\WINXT");
    check(mkdir("C:\\WINXT")==0&&mkdir("C:\\WINXT\\FILES")==0,
          "isolated mapping for real DOS WINXT journal paths");
    nitems=nclients=0; failafter=-1; validateinstall=0;
    check(prepare(),"prepare fresh journal without installed TWSTATE");
    statepath(state);
    check(!exists(state),"fresh rollback test has no installed state");
    check(recoverytarget("C:\\WINXT\\TP.EXE")&&
          recoverytarget("C:\\WINXT\\FILES\\TP.EXE")&&
          recoverytarget("c:\\winxt\\files\\sound.drv"),
          "exact current client paths and canonical mirror allowed");
    check(!recoverytarget("C:\\WINXT\\FOREIGN.EXE")&&
          !recoverytarget("C:\\WINXT2\\TP.EXE")&&
          !recoverytarget("C:\\WINXT\\FILES\\..\\TP.EXE")&&
          !recoverytarget("C:\\DOS\\COMMAND.COM"),
          "recovery refuses arbitrary same-drive paths and prefix tricks");
    join(source,stage,"GENERATE.TMP");
    check(putfile(source,"new")&&putfile("C:\\WINXT\\TP.EXE","old"),
          "prepare shared client original and replacement");
    check(add("C:\\WINXT\\TP.EXE",source,0)&&
          add("C:\\WINXT\\FILES\\SOUND.DRV",source,0)&&journal(),
          "journal stages actual shared client and canonical mirror paths");
    check(putfile("C:\\WINXT\\TP.EXE","torn")&&
          putfile("C:\\WINXT\\FILES\\SOUND.DRV","torn"),
          "simulate interrupted shared writes");
    check(rollback(),"recover shared paths before TWSTATE is committed");
    check(matches("C:\\WINXT\\TP.EXE",3UL,0x3f5dd4e5UL)&&
          !exists("C:\\WINXT\\FILES\\SOUND.DRV"),
          "shared original restored and newly created mirror removed");
    check(clearstage(),"shared interrupted journal cleaned");
    nitems=0;
    check(prepare(),"prepare failed first-install cleanup regression");
    join(source,stage,"GENERATE.TMP"); join(newfile,app,"NEW.TXT");
    check(putfile(source,"new")&&
          add("C:\\WINXT\\FILES\\SOUND.DRV",source,0)&&
          add(newfile,source,0),"stage shared and app additions");
    failafter=1;
    check(!commit(),"injected first-install shared write rolls back");
    failedinstallcleanup(0);
    check(!exists(app)&&!exists(stage)&&
          !exists("C:\\WINXT\\FILES\\SOUND.DRV"),
          "failed first install leaves no new app/private or mirror files");
    remove("C:\\WINXT\\TP.EXE");
    rmdir("C:\\WINXT\\FILES"); rmdir("C:\\WINXT"); host_xtroot[0]=0;
}
