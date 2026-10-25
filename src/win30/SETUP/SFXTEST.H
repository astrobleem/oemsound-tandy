static void sfxchecks(void) {
    unsigned char raw[32]; SXARCHIVE a; SXENTRY entry[1];
    memset(raw,0,sizeof(raw));raw[0]='M';raw[1]='Z';raw[4]=2;
    check(SXImage(raw)==1024UL,"SFX MZ image length");
    raw[2]=1;raw[3]=2;
    check(!SXImage(raw),"SFX MZ last-page length bounded");
    raw[2]=raw[3]=raw[4]=0;
    check(!SXImage(raw),"SFX zero-page image refused");
    memset(raw,0,sizeof(raw)); memcpy(raw,"TWINSFX1",8);
    put32(raw+8,1024UL);put32(raw+12,35UL);put32(raw+16,1UL);
    check(SXFooter(raw,1091UL,&a)&&a.image==1024UL&&a.count==1UL,
          "SFX footer accepted");
    check(!SXFooter(raw,1092UL,&a),"SFX trailing/size mismatch refused");
    put32(raw+16,0UL);
    check(!SXFooter(raw,1091UL,&a),"SFX zero entries refused");
    put32(raw+16,65UL);
    check(!SXFooter(raw,1091UL,&a),"SFX too many entries refused");
    put32(raw+16,1UL);put32(raw+12,4194305UL);
    check(!SXFooter(raw,4195361UL,&a),"SFX oversized bundle refused");
    put32(raw+12,35UL);put32(raw+28,1UL);
    check(!SXFooter(raw,1091UL,&a),"SFX unknown flags refused");
    put32(raw+28,0UL);put32(raw+8,0xfffffff0UL);
    check(!SXFooter(raw,1091UL,&a),"SFX wrapped offset refused");
    memset(raw,0,sizeof(raw));strcpy((char *)raw,"SETUP.EXE");
    put32(raw+16,32UL);put32(raw+20,3UL);put32(raw+24,0x6be34445UL);
    check(SXDecode(raw,32UL,35UL,&entry[0])&&
          equal(entry[0].name,"SETUP.EXE"),"SFX flat entry accepted");
    check(!SXDecode(raw,31UL,35UL,&entry[0]),"SFX overlap/gap refused");
    check(!SXDecode(raw,32UL,34UL,&entry[0]),"SFX truncated file refused");
    check(SXUnique(entry,0,"SETUP.EXE")&&
          !SXUnique(entry,1,"setup.exe"),"SFX duplicate names case-insensitive");
    raw[13]=1;
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX nonzero padding refused");
    raw[13]=0;raw[10]='X';
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX hidden name tail refused");
    memset(raw,0,16);strcpy((char *)raw,"COM1.EXE");
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX DOS device refused");
    memset(raw,0,16);strcpy((char *)raw,"..\\BAD.EXE");
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX traversal refused");
    memset(raw,0,16);strcpy((char *)raw,"SFXOWN.DAT");
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX ownership marker collision refused");
    memset(raw,0,16);memcpy(raw,"ABCDEFGHIJKLM",13);
    check(!SXDecode(raw,32UL,35UL,&entry[0]),"SFX missing name terminator refused");
    memset(raw,0,16);strcpy((char *)raw,"BIG.BIN");put32(raw+20,1048577UL);
    check(!SXDecode(raw,32UL,2097152UL,&entry[0]),"SFX oversized member refused");
}
