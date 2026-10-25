/* Native DOS installer. Microsoft C 6.0, small model, 8086. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SETUP_HOST
#include "DOSHOST.H"
#else
#include <dos.h>
#endif
#include "SOUNDID.H"
#include "TPLAYPOL.H"
#include "SFXCORE.H"
struct XTClient {
    const char *name;
    char kind;
    unsigned long old_bytes,old_crc,bytes,crc;
};
#include "XTCLIENT.H"

#define PLEN 128
#define MAXFILES 64
#define MAXITEMS 96
#define MAXINI 8192
#define MAXCLIENTS 14
#define FP(s,o) ((void far *)(((unsigned long)(s)<<16)|(unsigned)(o)))
typedef struct {
    char kind, source[13], name[13];
    unsigned long bytes, crc;
} PAYLOAD;
typedef struct {
    char path[PLEN];
    unsigned long bytes, crc;
    int existed, remove;
} ITEM;
typedef struct {
    unsigned which,mirror;
    unsigned long bytes,crc;
} CLIENTOWN;
static PAYLOAD files[MAXFILES];
static PAYLOAD ownedfiles[MAXFILES];
static ITEM items[MAXITEMS];
static CLIENTOWN clientowned[MAXCLIENTS];
static unsigned nfiles, nitems, nowned;
static unsigned nclients;
static char home[PLEN], win[PLEN], app[PLEN];
static char stage[PLEN], path[PLEN], temp[PLEN];
static char ini[MAXINI+1], edited[MAXINI+256];
static char selectedsound[128];
static unsigned long instbytes,instcrc,gobytes,gocrc;
static char candidates[16][PLEN];
static char newdirs[2][PLEN];
static unsigned nnewdirs;
static SXENTRY sfxentries[SXMAX];
static char sfxdir[PLEN];
static unsigned sfxmade;
static unsigned ncandidates, scanned;
static int quiet, failafter=-1, validateinstall, soundmirror;

static int equal(const char *a,const char *b) {
    return TPNameEqual(a,b);
}
static int fail(const char *why) {
    printf("SETUP: %s\n",why);
    return 0;
}
static int join(char *out,const char *a,const char *b) {
    if(strlen(a)+strlen(b)+2>=PLEN) return 0;
    strcpy(out,a);
    if(*a&&a[strlen(a)-1]!='\\') strcat(out,"\\");
    strcat(out,b); return 1;
}
static int basename83(const char *p) {
    unsigned n=0,dot=0,i=0; char base[9]; const char *s=p;
    if(!*p) return 0;
    while(*s&&*s!='.'&&i<8) base[i++]=(char)toupper((unsigned char)*s++);
    base[i]=0;
    if(equal(base,"CON")||equal(base,"PRN")||equal(base,"AUX")||
       equal(base,"NUL")||equal(base,"CLOCK$")||
       ((i==4)&&(base[3]>='1'&&base[3]<='9')&&
        ((!strncmp(base,"COM",3))||(!strncmp(base,"LPT",3))))) return 0;
    while(*p) {
        unsigned c=(unsigned char)*p++;
        if(c=='.') {
            if(!n||dot) return 0;
            dot=1; n=0; continue;
        }
        if(!isalnum(c)&&c!='_'&&c!='-'&&c!='$'&&c!='~') return 0;
        if(++n>(dot?3U:8U)) return 0;
    }
    return n!=0;
}
static int absolute(const char *p) {
    char part[13]; unsigned n=0;
    if(!isalpha((unsigned char)p[0])||p[1]!=':'||p[2]!='\\'||!p[3])
        return 0;
    p+=3;
    for(;;p++) {
        if(!*p||*p=='\\') {
            part[n]=0; if(!basename83(part)) return 0;
            n=0; if(!*p) return 1;
        } else {
            if(n>=12) return 0;
            part[n++]=*p;
        }
    }
}
static int exists(const char *p) {
    return access(p,0)==0;
}
static unsigned long crcstep(unsigned long crc,unsigned value) {
    unsigned bit;
    crc^=value;
    for(bit=0;bit<8;bit++)
        crc=(crc>>1)^((crc&1)?0xedb88320UL:0UL);
    return crc;
}
static int identity(const char *p,unsigned long *size,unsigned long *sum) {
    FILE *f; int c; unsigned long n=0,crc=0xffffffffUL;
    f=fopen(p,"rb"); if(!f) return 0;
    while((c=fgetc(f))!=EOF) { ++n; crc=crcstep(crc,(unsigned)c); }
    c=!ferror(f); if(fclose(f)) c=0;
    *size=n; *sum=crc^0xffffffffUL; return c;
}
static int matches(const char *p,unsigned long size,unsigned long sum) {
    unsigned long n,c;
    return identity(p,&n,&c)&&n==size&&c==sum;
}
static int copyfile(const char *src,const char *dst) {
    FILE *a,*b; unsigned n; int ok=1; char buf[1024];
    a=fopen(src,"rb"); if(!a) return 0;
    b=fopen(dst,"wb"); if(!b) { fclose(a); return 0; }
    while((n=fread(buf,1,sizeof(buf),a))!=0)
        if(fwrite(buf,1,n,b)!=n) { ok=0; break; }
    if(ferror(a)||ferror(b)) ok=0;
    if(fclose(a)) ok=0;
    if(fclose(b)) ok=0;
    return ok;
}
static int putfile(const char *p,const char *s) {
    FILE *f; int ok;
    f=fopen(p,"wb"); if(!f) return 0;
    ok=fwrite(s,1,strlen(s),f)==strlen(s);
    if(fclose(f)) ok=0;
    return ok;
}
static char *trim(char *p) {
    char *q;
    while(*p==' '||*p=='\t') p++;
    q=p+strlen(p);
    while(q>p&&(q[-1]==' '||q[-1]=='\t'||q[-1]=='\r'||q[-1]=='\n'))
        *--q=0;
    return p;
}
static void trimself(char *p) {
    char *first=trim(p); memmove(p,first,strlen(first)+1);
}
static int readtext(const char *p,char *out,unsigned capacity) {
    FILE *f; unsigned n; int c;
    f=fopen(p,"rb"); if(!f) return 0;
    n=fread(out,1,capacity-1,f); c=fgetc(f);
    if(ferror(f)||c!=EOF) { fclose(f); return 0; }
    fclose(f); out[n]=0;
    return strlen(out)==n;
}
/* Preserve every unrelated byte. Multiple sound entries become one entry;
   repeated [boot] sections are ambiguous and refused. No profile APIs. */
static int changeini(const char *input,char *output,const char *value,
                     char *selection,int validate) {
    const char *line,*end,*next;
    char work[256],*p,*q; unsigned n,used=0;
    unsigned boot=0,boots=0,sounds=0,system=0,display=0;
    int inserted=0;
    output[0]=0; selection[0]=0;
    for(line=input;*line;line=next) {
        end=strchr(line,'\n'); next=end?end+1:line+strlen(line);
        n=(unsigned)(next-line); if(n>=sizeof(work)) return 0;
        memcpy(work,line,n); work[n]=0;
        q=strchr(work,';'); if(q) *q=0; p=trim(work);
        if(*p=='[') {
            if(boot&&!inserted&&value) {
                if(used&&output[used-1]!='\n') {
                    strcpy(output+used,"\r\n"); used+=2;
                }
                if(used+strlen(value)+14>=MAXINI+256) return 0;
                sprintf(output+used,"sound.drv=%s\r\n",value);
                used+=(unsigned)strlen(output+used); inserted=1;
            }
            q=strchr(p,']'); if(!q||*trim(q+1)) return 0;
            *q=0; boot=equal(trim(p+1),"boot");
            if(boot&&++boots!=1) return 0;
        } else if(boot&&*p) {
            q=strchr(p,'='); if(!q) return 0;
            *q=0; p=trim(p); q=trim(q+1);
            if(equal(p,"sound.drv")) {
                if(!sounds&&strlen(q)<128) strcpy(selection,q);
                if(++sounds>64||strlen(q)>=128) return 0;
                if(!inserted&&value) {
                    if(used+strlen(value)+14>=MAXINI+256) return 0;
                    sprintf(output+used,"sound.drv=%s\r\n",value);
                    used+=(unsigned)strlen(output+used); inserted=1;
                }
                continue;
            }
            if(equal(p,"system.drv")) {
                if(++system!=1||!equal(q,"SYSTEM.DRV")) return 0;
            }
            if(equal(p,"display.drv")) {
                if(++display!=1||TPDisplay(q)<0) return 0;
            }
        }
        if(used+n>=MAXINI+256) return 0;
        memcpy(output+used,line,n); used+=n; output[used]=0;
    }
    if(boot&&!inserted&&value) {
        if(used&&output[used-1]!='\n') {
            strcpy(output+used,"\r\n"); used+=2;
        }
        if(used+strlen(value)+14>=MAXINI+256) return 0;
        sprintf(output+used,"sound.drv=%s\r\n",value);
    }
    return strlen(output)<=MAXINI&&boots==1&&
           (!validate||(system==1&&display==1));
}
static int inwindows(void) {
    union REGS r;
    r.x.ax=0x1600; int86(0x2f,&r,&r);
    if(r.h.al!=0&&r.h.al!=0x80) return 1;
    r.x.ax=0x4680; int86(0x2f,&r,&r); return r.x.ax==0;
}
static int plainDOS(void) {
    union REGS r;
    r.h.ah=0x30; int86(0x21,&r,&r);
    if(r.h.al<3) return fail("Requires DOS 3.0 or newer.");
    if(inwindows()) return fail("Exit Windows before running Setup.");
    if(!TPBoard(*(unsigned char far *)FP(0xfc00,0),
                *(unsigned char far *)FP(0xffff,0x0e)))
        return fail("Requires the supported Tandy EX/HX hardware.");
    r.x.ax=0xd7a0; r.x.bx=0x5451; int86(0x2f,&r,&r);
    if(r.x.ax==0xd7af) return fail("Exit the active timer application first.");
    return 1;
}
static void sfxcleanup(void) {
    unsigned i; char p[PLEN];
    if(!*sfxdir) return;
    for(i=0;i<sfxmade;i++)
        if(join(p,sfxdir,sfxentries[i].name)) remove(p);
    if(join(p,sfxdir,"SFXOWN.DAT")) remove(p);
    rmdir(sfxdir); sfxdir[0]=0;
}
static int sfxload(const char *exe) {
    FILE *f,*dest; SXARCHIVE a;
    unsigned char raw[SXFOOT],mz[6];
    char p[PLEN],folder[13],buf[1024];
    unsigned i,j,n,seed; int ok=1; long length;
    unsigned long expected,crc,tablecrc,remaining,body;
    union REGS r;
    f=fopen(exe,"rb"); if(!f) return fail("Cannot read Setup.");
    if(fseek(f,0L,SEEK_END)||(length=ftell(f))<SXFOOT||
       fseek(f,0L,SEEK_SET)||fread(mz,1,sizeof(mz),f)!=sizeof(mz)||
       !(body=SXImage(mz))||body>(unsigned long)length) goto bad;
    /* Plain cached installers have exactly the MZ-declared file length.
       Any overlay requires the complete valid footer, even beside a list. */
    if(body==(unsigned long)length) { fclose(f); return 1; }
    if((unsigned long)length-body<SXFOOT||
       fseek(f,length-SXFOOT,SEEK_SET)||
       fread(raw,1,sizeof(raw),f)!=sizeof(raw)||
       !SXFooter(raw,(unsigned long)length,&a)||body!=a.image||
       fseek(f,(long)a.image,SEEK_SET)) goto bad;
    expected=a.count*SXENTRYBYTES; tablecrc=0xffffffffUL;
    for(i=0;i<(unsigned)a.count;i++) {
        if(fread(raw,1,SXENTRYBYTES,f)!=SXENTRYBYTES||
           !SXDecode(raw,expected,a.bundle,&sfxentries[i])||
           !SXUnique(sfxentries,i,sfxentries[i].name)) goto bad;
        expected+=sfxentries[i].bytes;
        for(j=0;j<SXENTRYBYTES;j++) tablecrc=crcstep(tablecrc,raw[j]);
    }
    if(expected!=a.bundle||(tablecrc^0xffffffffUL)!=a.tablecrc||
       fseek(f,(long)a.image,SEEK_SET)) goto bad;
    remaining=a.bundle; crc=0xffffffffUL;
    while(remaining) {
        n=(unsigned)(remaining>sizeof(buf)?sizeof(buf):remaining);
        if(fread(buf,1,n,f)!=n) goto bad;
        for(j=0;j<n;j++) crc=crcstep(crc,(unsigned char)buf[j]);
        remaining-=n;
    }
    if((crc^0xffffffffUL)!=a.bundlecrc) goto bad;
    /* Exclusive directory creation is the ownership proof. Never adopt an
       existing name, even one containing a plausible old marker. */
    r.h.ah=0x2c; int86(0x21,&r,&r); seed=(r.x.cx^r.x.dx)%10000U;
    for(i=0;i<128;i++) {
        sprintf(folder,"TWI%04u",(seed+i)%10000U);
        if(!join(p,"C:\\",folder)) goto bad;
        if(!mkdir(p)) { strcpy(sfxdir,p); break; }
    }
    if(!*sfxdir) { fclose(f); return fail("Cannot create a temporary Setup directory."); }
    if(atexit(sfxcleanup)) { fclose(f); sfxcleanup(); return 0; }
    if(!join(p,sfxdir,"SFXOWN.DAT")||!putfile(p,"TWINSFX1\r\n"))
        { fclose(f); return fail("Cannot mark the temporary Setup directory."); }
    puts("Preparing native Setup...");
    for(i=0;i<(unsigned)a.count;i++) {
        if(!join(p,sfxdir,sfxentries[i].name)||
           fseek(f,(long)(a.image+sfxentries[i].offset),SEEK_SET)) goto bad;
        sfxmade=i+1; dest=fopen(p,"wb"); if(!dest) goto bad;
        remaining=sfxentries[i].bytes; crc=0xffffffffUL;
        while(remaining) {
            n=(unsigned)(remaining>sizeof(buf)?sizeof(buf):remaining);
            if(fread(buf,1,n,f)!=n||fwrite(buf,1,n,dest)!=n) { ok=0;break; }
            for(j=0;j<n;j++) crc=crcstep(crc,(unsigned char)buf[j]);
            remaining-=n;
        }
        if(fclose(dest)) ok=0;
        if(!ok||(crc^0xffffffffUL)!=sfxentries[i].crc) goto bad;
    }
    fclose(f); strcpy(home,sfxdir);
    if(equal(home,app)) return fail("Choose a permanent application directory.");
    return 1;
bad:
    fclose(f); return fail("The native installer download is incomplete or damaged.");
}
static int windowsdir(const char *root) {
    FILE *f; long bytes; char p[PLEN];
    if(!absolute(root)||!join(p,root,"WIN.COM")) return 0;
    f=fopen(p,"rb"); if(!f) return 0;
    bytes=fseek(f,0L,SEEK_END)?-1L:ftell(f); fclose(f);
    if(!TPBootstrapLength(bytes)) return 0;
    if(!join(p,root,"SYSTEM\\SYSTEM.DRV")||
       !matches(p,2784UL,0x3ff05fecUL)) return 0;
    if(!join(p,root,"SYSTEM.DRV")||exists(p)) return 0;
    if(!join(p,root,"SYSTEM.INI")||!readtext(p,ini,sizeof(ini)))
        return 0;
    return changeini(ini,edited,"SOUND.DRV",selectedsound,1);
}
static void candidate(const char *root) {
    unsigned i;
    if(ncandidates==16) return;
    for(i=0;i<ncandidates;i++) if(equal(root,candidates[i])) return;
    if(windowsdir(root)) strcpy(candidates[ncandidates++],root);
}
static void scan(const char *root,unsigned depth) {
    struct find_t find; char pattern[PLEN],child[PLEN]; unsigned result;
    if(scanned++>=4096||depth>4) return;
    candidate(root);
    if(!join(pattern,root,"*.*")) return;
    result=_dos_findfirst(pattern,_A_SUBDIR,&find);
    while(!result) {
        if((find.attrib&_A_SUBDIR)&&find.name[0]!='.'&&
           basename83(find.name)&&join(child,root,find.name))
            scan(child,depth+1);
        if(scanned>=4096) break;
        result=_dos_findnext(&find);
    }
}
static int choosewindows(void) {
    char *env,*part,*next; char roots[512],root[4],answer[32];
    unsigned d,i; union REGS r;
    env=getenv("WINDIR"); if(env&&strlen(env)<PLEN) candidate(env);
    env=getenv("PATH");
    if(env&&strlen(env)<sizeof(roots)) {
        strcpy(roots,env); part=roots;
        while(part) {
            next=strchr(part,';'); if(next) *next++=0;
            candidate(part); part=next;
        }
    }
    for(d=3;d<=26&&scanned<4096;d++) {
        r.x.ax=0x4408; r.x.bx=d; int86(0x21,&r,&r);
        if(r.x.cflag||r.x.ax!=1) continue;
        r.x.ax=0x4409; r.x.bx=d; int86(0x21,&r,&r);
        if(r.x.cflag||(r.x.dx&0x1000)) continue;
        root[0]=(char)('A'+d-1); root[1]=':'; root[2]='\\';root[3]=0;
        scan(root,0);
    }
    if(ncandidates==1) { strcpy(win,candidates[0]); return 1; }
    if(quiet) return fail("Specify /WIN:path when no unique Windows install was found.");
    if(ncandidates) {
        puts("Select Windows:");
        for(i=0;i<ncandidates;i++) printf("  %u. %s\n",i+1,candidates[i]);
        printf("Number: "); if(!fgets(answer,sizeof(answer),stdin)) return 0;
        i=(unsigned)atoi(answer);
        if(i<1||i>ncandidates) return fail("No installation selected.");
        strcpy(win,candidates[i-1]); return 1;
    }
    puts("Windows directory (for example C:\\WINDOWS):");
    if(!fgets(win,sizeof(win),stdin)) return 0;
    trimself(win); return windowsdir(win);
}
static int reserved(const char *name) {
    return equal(name,"GO.BAT")||equal(name,"PAYLOAD.LST")||
           equal(name,"TWSTATE.DAT")||equal(name,"TWSTAGE");
}
static int payloadsource(PAYLOAD *file,char *out) {
    if(join(out,home,file->source)&&matches(out,file->bytes,file->crc))
        return 1;
    return file->kind=='S'&&equal(home,app)&&
        join(out,win,"SYSTEM\\SOUND.DRV")&&
        matches(out,file->bytes,file->crc);
}
static int clientpin(PAYLOAD *file) {
    unsigned i;
    for(i=0;i<XTCLIENT_COUNT;i++)
        if(xtclients[i].kind==file->kind&&equal(xtclients[i].name,file->name)&&
           equal(file->source,file->name)&&file->bytes==xtclients[i].bytes&&
           file->crc==xtclients[i].crc) return (int)i+1;
    return 0;
}
static int clientpath(PAYLOAD *file,char *out,int mirror) {
    char root[32];
    if(!mirror&&file->kind=='W') return join(out,win,file->name);
    strcpy(root,mirror?"C:\\WINXT\\FILES":"C:\\WINXT");
    root[0]=win[0]; return join(out,root,file->name);
}
static int clientrecord(unsigned which,unsigned mirror) {
    unsigned i;
    for(i=0;i<nclients;i++)
        if(clientowned[i].which==which&&clientowned[i].mirror==mirror)
            return (int)i+1;
    return 0;
}
static int shellrequired(void) {
    unsigned i; char p[PLEN];
    for(i=0;i<XTCLIENT_COUNT;i++)
        if(xtclients[i].kind=='W'&&equal(xtclients[i].name,"TSHELL.EXE")) {
            if(clientrecord(i,0)) return 1;
            if(!join(p,win,"TSHELL.EXE")) return 0;
            return matches(p,xtclients[i].old_bytes,xtclients[i].old_crc)||
                   matches(p,xtclients[i].bytes,xtclients[i].crc);
        }
    return 0;
}
static int inputpolicy(PAYLOAD *file,unsigned mirror,int shell) {
    if(file->kind=='W'&&equal(file->name,"TSINPUT.DLL")&&!mirror)
        return shell?1:-1;
    return 0;
}
static int fixkit(void) {
    unsigned i,j; PAYLOAD file; char p[PLEN];
    for(i=0;i<XTCLIENT_COUNT;i++)
        if(xtclients[i].kind=='X'&&equal(xtclients[i].name,"FIXXT.EXE")) {
            if(clientrecord(i,0)||clientrecord(i,1)) return 2;
            file.kind='X'; strcpy(file.name,"FIXXT.EXE");
            for(j=0;j<2;j++)
                if(clientpath(&file,p,(int)j)&&
                   (matches(p,xtclients[i].old_bytes,xtclients[i].old_crc)||
                    matches(p,xtclients[i].bytes,xtclients[i].crc))) return 1;
        }
    return 0;
}
static int mirrorsoundpolicy(int kit,int present,int canonical) {
    if(!kit) return 0;
    return kit==1&&present&&!canonical?-1:1;
}
static int mirrorsoundownership(void) {
    PAYLOAD file; int kit,policy;
    kit=fixkit(); soundmirror=0;
    if(!kit) return 1;
    file.kind='S'; strcpy(file.name,"SOUND.DRV");
    if(!clientpath(&file,path,1)) return 0;
    policy=mirrorsoundpolicy(kit,exists(path),
                            matches(path,SOUND_BYTES,SOUND_CRC));
    if(policy<0)
        return fail("The Windows XT sound source belongs to another file; no files changed.");
    soundmirror=policy; return 1;
}
static int clientownership(void) {
    unsigned i,j; int which,owned,policy,shell=shellrequired();
    for(i=0;i<nfiles;i++) if(files[i].kind=='W'||files[i].kind=='X') {
        which=clientpin(&files[i]); if(!which) return 0;
        for(j=0;j<2;j++) {
            policy=inputpolicy(&files[i],j,shell);
            if(policy<0) continue;
            if(!clientpath(&files[i],path,(int)j)) return 0;
            owned=clientrecord((unsigned)which-1,j);
            if(!owned&&exists(path)&&
               !matches(path,xtclients[which-1].old_bytes,
                        xtclients[which-1].old_crc)&&
               !matches(path,files[i].bytes,files[i].crc))
                return fail("A Windows XT client belongs to another file; no files changed.");
            if(!owned&&(exists(path)||policy>0)) {
                if(nclients==MAXCLIENTS) return 0;
                owned=(int)++nclients;
                clientowned[owned-1].which=(unsigned)which-1;
                clientowned[owned-1].mirror=j;
            }
            if(owned) {
                clientowned[owned-1].bytes=files[i].bytes;
                clientowned[owned-1].crc=files[i].crc;
            }
        }
    }
    return 1;
}
static int clientdirs(PAYLOAD *file,int mirror) {
    char root[PLEN],folder[PLEN];
    if(!mirror&&file->kind=='W') return 1;
    strcpy(root,"C:\\WINXT"); root[0]=win[0];
    if(!exists(root)) {
        if(mkdir(root)||nnewdirs==2) return 0;
        strcpy(newdirs[nnewdirs++],root);
    }
    if(mirror) {
        if(!join(folder,root,"FILES")) return 0;
        if(!exists(folder)) {
            if(mkdir(folder)||nnewdirs==2) return 0;
            strcpy(newdirs[nnewdirs++],folder);
        }
    }
    return 1;
}
static void cleanupdirs(void) {
    while(nnewdirs) rmdir(newdirs[--nnewdirs]);
}
static void failedinstallcleanup(int installed) {
    cleanupdirs();
    if(!installed) rmdir(app);
}
static int manifest(void) {
    FILE *f; char line[256],*p,*part[5]; unsigned i,j,sounds=0,found;
    static const char *required[]={"SETUP.EXE","TPLAY.EXE","TBRIDGE.EXE",
        "MINIPLAY.EXE","RAWCLK.DLL","LWFILE.DLL","MCITNDY.DLL",
        "MIDICLK.DLL","RESERVE.COM"};
    if(!join(path,home,"PAYLOAD.LST")||!(f=fopen(path,"rb")))
        return fail("Keep SETUP.EXE with the complete downloaded package.");
    if(!fgets(line,sizeof(line),f)||!equal(trim(line),"TWSETUP1"))
        { fclose(f); return fail("Package list version mismatch."); }
    while(fgets(line,sizeof(line),f)) {
        p=trim(line); if(!*p) continue;
        if(nfiles==MAXFILES) { fclose(f); return fail("Too many package files."); }
        for(i=0;i<5;i++) {
            part[i]=p; p=strchr(p,'|');
            if(i<4&&!p) { fclose(f); return fail("Invalid package list."); }
            if(p) *p++=0;
        }
        if(p||strlen(part[0])!=1||
           (part[0][0]!='A'&&part[0][0]!='S'&&
            part[0][0]!='W'&&part[0][0]!='X')||
           !basename83(part[1])||!basename83(part[2])||
           reserved(part[2])||!*part[3]||strlen(part[3])>9||
           strlen(part[4])!=8)
            { fclose(f); return fail("Invalid package filename or identity."); }
        for(i=0;part[3][i];i++) if(!isdigit((unsigned char)part[3][i]))
            { fclose(f); return fail("Invalid package size."); }
        for(i=0;part[4][i];i++) if(!isxdigit((unsigned char)part[4][i]))
            { fclose(f); return fail("Invalid package CRC."); }
        for(j=0;j<nfiles;j++)
            if(equal(files[j].name,part[2])&&
               (files[j].kind==part[0][0]||
                (files[j].kind!='S'&&part[0][0]!='S')))
                { fclose(f); return fail("Duplicate package destination."); }
        files[nfiles].kind=part[0][0];
        strcpy(files[nfiles].source,part[1]);
        strcpy(files[nfiles].name,part[2]);
        files[nfiles].bytes=strtoul(part[3],0,10);
        files[nfiles].crc=strtoul(part[4],0,16);
        if(files[nfiles].kind=='S') {
            ++sounds;
            if(!equal(part[1],"SOUND.DRV")||!equal(part[2],"SOUND.DRV")||
               files[nfiles].bytes!=SOUND_BYTES||
               files[nfiles].crc!=SOUND_CRC)
                { fclose(f); return fail("Canonical sound driver pin mismatch."); }
        }
        if((files[nfiles].kind=='W'||files[nfiles].kind=='X')&&
           !clientpin(&files[nfiles]))
            { fclose(f); return fail("Current Windows XT client pin mismatch."); }
        if(!payloadsource(&files[nfiles],path))
            { fclose(f); return fail("A package file is missing or damaged."); }
        ++nfiles;
    }
    i=!ferror(f); fclose(f);
    if(!i||sounds!=1||!nfiles) return fail("Incomplete package list.");
    for(i=0;i<sizeof(required)/sizeof(required[0]);i++) {
        found=0;
        for(j=0;j<nfiles;j++)
            if(files[j].kind=='A'&&equal(files[j].name,required[i])) found=1;
        if(!found) return fail("A required application component is missing.");
    }
    for(i=0;i<XTCLIENT_COUNT;i++) {
        found=0;
        for(j=0;j<nfiles;j++)
            if(files[j].kind==xtclients[i].kind&&
               equal(files[j].name,xtclients[i].name)) found=1;
        if(!found) return fail("Current Windows XT client payload is incomplete.");
    }
    return 1;
}
static int statepath(char *out) { return join(out,app,"TWSTATE.DAT"); }
static void launchscript(char *out,const char *directory,const char *windows) {
    sprintf(out,"@ECHO OFF\r\n%c:\r\nCD %s\r\nTPLAY.EXE %s\r\n",
            directory[0],directory+2,windows);
}
static int readpair(FILE *f,unsigned long *bytes,unsigned long *crc) {
    char line[64],tail;
    return fgets(line,sizeof(line),f)&&
           sscanf(line,"%lu|%lx %c",bytes,crc,&tail)==2;
}
static int readstate(void) {
    FILE *f; char line[256],w[PLEN],a[PLEN],tail;
    char kind,name[13]; unsigned i,j,mirror,which;
    if(!statepath(path)||!(f=fopen(path,"rb"))) return 0;
    if(!fgets(line,sizeof(line),f)||!equal(trim(line),"TWSTATE1")||
       !fgets(w,sizeof(w),f)||!fgets(a,sizeof(a),f))
        { fclose(f); return -1; }
    trimself(w); trimself(a);
    if(!absolute(w)||!equal(a,app)||
       !readpair(f,&instbytes,&instcrc)||!instbytes)
        { fclose(f); return -1; }
    if(!fgets(line,sizeof(line),f)||
       sscanf(line,"%u %c",&nowned,&tail)!=1||nowned>MAXFILES)
        { fclose(f); return -1; }
    for(i=0;i<nowned;i++) {
        if(!fgets(line,sizeof(line),f)||
           sscanf(line,"%12[^|]|%lu|%lx %c",ownedfiles[i].name,
                  &ownedfiles[i].bytes,&ownedfiles[i].crc,&tail)!=3||
           !basename83(ownedfiles[i].name)||reserved(ownedfiles[i].name))
            { fclose(f); return -1; }
        for(j=0;j<i;j++) if(equal(ownedfiles[i].name,ownedfiles[j].name))
            { fclose(f); return -1; }
    }
    if(!fgets(line,sizeof(line),f)||
       sscanf(line,"%u %c",&nclients,&tail)!=1||nclients>MAXCLIENTS)
        { fclose(f); return -1; }
    for(i=0;i<nclients;i++) {
        if(!fgets(line,sizeof(line),f)||
           sscanf(line,"%c|%12[^|]|%u|%lu|%lx %c",&kind,name,&mirror,
                  &clientowned[i].bytes,&clientowned[i].crc,&tail)!=5||
           mirror>1) { fclose(f); return -1; }
        which=XTCLIENT_COUNT;
        for(j=0;j<XTCLIENT_COUNT;j++)
            if(kind==xtclients[j].kind&&equal(name,xtclients[j].name)) which=j;
        if(which==XTCLIENT_COUNT)
            { fclose(f); return -1; }
        /* Search only already-read entries for duplicate path records. */
        for(j=0;j<i;j++)
            if(clientowned[j].which==which&&clientowned[j].mirror==mirror)
                { fclose(f); return -1; }
        clientowned[i].which=which; clientowned[i].mirror=mirror;
    }
    if(*win&&!equal(win,w)) { fclose(f); return -1; }
    /* Retain the exact recorded path spelling used to generate GO.BAT. */
    launchscript(edited,a,w); gobytes=(unsigned long)strlen(edited);
    gocrc=0xffffffffUL;
    for(i=0;edited[i];i++) gocrc=crcstep(gocrc,(unsigned char)edited[i]);
    gocrc^=0xffffffffUL;
    strcpy(win,w); fclose(f); return 1;
}
static int writestate(const char *dst) {
    FILE *f; int ok; unsigned i,count=0;
    f=fopen(dst,"wb"); if(!f) return 0;
    ok=fprintf(f,"TWSTATE1\r\n%s\r\n%s\r\n%lu|%08lx\r\n",
        win,app,(unsigned long)SOUND_BYTES,(unsigned long)SOUND_CRC)>0;
    for(i=0;i<nfiles;i++) if(files[i].kind!='S') ++count;
    if(ok) ok=fprintf(f,"%u\r\n",count)>0;
    for(i=0;i<nfiles&&ok;i++) if(files[i].kind!='S')
        ok=fprintf(f,"%s|%lu|%08lx\r\n",files[i].name,
                   files[i].bytes,files[i].crc)>0;
    if(ok) ok=fprintf(f,"%u\r\n",nclients)>0;
    for(i=0;i<nclients&&ok;i++)
        ok=fprintf(f,"%c|%s|%u|%lu|%08lx\r\n",
            xtclients[clientowned[i].which].kind,
            xtclients[clientowned[i].which].name,clientowned[i].mirror,
            clientowned[i].bytes,clientowned[i].crc)>0;
    if(fclose(f)) ok=0;
    return ok;
}
static void stagepath(char *out,unsigned n,int original) {
    char name[13]; sprintf(name,original?"R%03u.BAK":"N%03u.TMP",n);
    join(out,stage,name);
}
static int add(const char *destination,const char *source,int remove) {
    ITEM *it; char p[PLEN]; unsigned long bytes,crc;
    if(nitems==MAXITEMS) return 0;
    it=&items[nitems]; strcpy(it->path,destination);
    it->remove=remove; it->existed=exists(destination);
    if(it->existed) {
        stagepath(p,nitems,1);
        if(!identity(destination,&bytes,&crc)||!copyfile(destination,p)||
           !matches(p,bytes,crc)) return 0;
    }
    if(!remove) {
        stagepath(p,nitems,0);
        if(!identity(source,&it->bytes,&it->crc)||!copyfile(source,p)||
           !matches(p,it->bytes,it->crc)) return 0;
    }
    ++nitems; return 1;
}
static int journal(void) {
    FILE *f; unsigned i; int ok;
    if(!join(path,stage,"TXN.NEW")||!(f=fopen(path,"wb"))) return 0;
    ok=fprintf(f,"TWTRANS1\r\n%s\r\n%u\r\n",win,nitems)>0;
    for(i=0;i<nitems&&ok;i++)
        ok=fprintf(f,"%d|%s\r\n",items[i].existed,items[i].path)>0;
    if(fclose(f)) ok=0;
    if(!ok||!join(temp,stage,"TXN.DAT")) return 0;
    return rename(path,temp)==0;
}
static int recoverytarget(const char *dst) {
    PAYLOAD file; char allowed[PLEN]; unsigned i,mirror;
    if(!absolute(dst)) return 0;
    if((!strnicmp(dst,app,strlen(app))&&dst[strlen(app)]=='\\')||
       (!strnicmp(dst,win,strlen(win))&&dst[strlen(win)]=='\\')) return 1;
    /* A fresh owned journal predates TWSTATE. Permit only the exact current
       client paths preflight can stage, never the whole WINXT namespace. */
    for(i=0;i<XTCLIENT_COUNT;i++) {
        file.kind=xtclients[i].kind; strcpy(file.name,xtclients[i].name);
        for(mirror=0;mirror<2;mirror++)
            if(clientpath(&file,allowed,(int)mirror)&&equal(dst,allowed))
                return 1;
    }
    file.kind='S'; strcpy(file.name,"SOUND.DRV");
    return clientpath(&file,allowed,1)&&equal(dst,allowed);
}
static int rollback(void) {
    FILE *f; char line[256],dst[PLEN],src[PLEN],*p;
    unsigned n,i; int had,ok=1;
    if(!join(path,stage,"TXN.DAT")) return 0;
    f=fopen(path,"rb"); if(!f) return 1;
    if(!fgets(line,sizeof(line),f)||!equal(trim(line),"TWTRANS1")||
       !fgets(line,sizeof(line),f)) { fclose(f); return 0; }
    strcpy(dst,trim(line));
    if(!absolute(dst)||(*win&&!equal(win,dst)))
        { fclose(f); return 0; }
    strcpy(win,dst);
    if(!fgets(line,sizeof(line),f)) { fclose(f); return 0; }
    n=(unsigned)atoi(line); if(n>MAXITEMS) { fclose(f); return 0; }
    for(i=0;i<n;i++) {
        if(!fgets(line,sizeof(line),f)) { ok=0; break; }
        p=strchr(line,'|');
        if(!p) { ok=0; break; }
        *p++=0; had=atoi(line); strcpy(dst,trim(p));
        if(!recoverytarget(dst))
            { ok=0; break; }
        if(had) {
            stagepath(src,i,1);
            if(!copyfile(src,dst)) ok=0;
        } else if(exists(dst)&&remove(dst)) ok=0;
    }
    fclose(f);
    if(ok&&remove(path)) ok=0;
    return ok;
}
static int stageowned(void) {
    char record[PLEN+32],expected[PLEN+32],p[PLEN];
    sprintf(expected,"TWSTAGE1\r\n%s\r\n",app);
    return join(p,stage,"OWNER.DAT")&&readtext(p,record,sizeof(record))&&
           equal(record,expected);
}
static int stagefile(const char *name) {
    if(equal(name,"TXN.NEW")||equal(name,"TXN.DAT")||
       equal(name,"GENERATE.TMP")||equal(name,"OWNER.DAT")) return 1;
    return strlen(name)==8&&(name[0]=='N'||name[0]=='R')&&
        isdigit((unsigned char)name[1])&&isdigit((unsigned char)name[2])&&
        isdigit((unsigned char)name[3])&&name[4]=='.'&&
        (equal(name+5,"TMP")||equal(name+5,"BAK"));
}
static int clearstage(void) {
    struct find_t find; unsigned rc; char pattern[PLEN],p[PLEN];
    if(!stageowned()||!join(pattern,stage,"*.*")) return 0;
    rc=_dos_findfirst(pattern,_A_SUBDIR|_A_HIDDEN|_A_SYSTEM,&find);
    while(!rc) {
        if(strcmp(find.name,".")&&strcmp(find.name,"..")&&
           ((find.attrib&_A_SUBDIR)||!stagefile(find.name))) return 0;
        rc=_dos_findnext(&find);
    }
    rc=_dos_findfirst(pattern,_A_NORMAL,&find);
    while(!rc) {
        if(!(find.attrib&_A_SUBDIR)&&stagefile(find.name)&&
           join(p,stage,find.name)&&!equal(find.name,"OWNER.DAT")) {
            if(remove(p)) return 0;
        }
        rc=_dos_findnext(&find);
    }
    if(!join(p,stage,"OWNER.DAT")||remove(p)) return 0;
    if(rmdir(stage)) {
        sprintf(edited,"TWSTAGE1\r\n%s\r\n",app);
        putfile(p,edited); return 0;
    }
    return 1;
}
static int commit(void) {
    unsigned i; char src[PLEN]; int ok=1;
    if(!journal()) return fail("Could not write the recovery journal; no files changed.");
    for(i=0;i<nitems;i++) {
        if(failafter==(int)i) {
#ifdef SETUP_TEST
            printf("SETUP TEST: injected commit failure after %u writes.\n",i);
#endif
            ok=0; break;
        }
        if(items[i].remove) {
            if(exists(items[i].path)&&remove(items[i].path)) { ok=0; break; }
        } else {
            stagepath(src,i,0);
            if(!copyfile(src,items[i].path)||
               !matches(items[i].path,items[i].bytes,items[i].crc))
                { ok=0; break; }
        }
    }
    if(ok&&validateinstall) {
        if(!join(src,app,"TPLAY.EXE")||
           spawnl(P_WAIT,src,src,win,"/CHECK",(char *)0)!=0) ok=0;
    }
    if(!ok) {
        if(!rollback()) return fail("Recovery could not finish. Run SETUP again to resume recovery.");
        clearstage(); return fail("Install failed; the previous installation was restored.");
    }
    if(!join(path,stage,"TXN.DAT")||remove(path))
        return fail("Could not finish the transaction. Run SETUP again.");
    if(!clearstage()) return fail("Files installed; Setup could not finish private staging cleanup.");
    return 1;
}
static int prepare(void) {
    if(!join(stage,app,"TWSTAGE")) return 0;
    if(exists(stage)) {
        if(!stageowned()) return fail("Reserved staging directory belongs to another program.");
        if(!rollback()) return fail("The saved transaction could not be recovered.");
        if(!clearstage()) return fail("Cannot clean the saved private staging directory.");
        puts("Previous interrupted operation recovered.");
    }
    if(!exists(app)&&mkdir(app)) return fail("Cannot create application directory.");
    if(mkdir(stage)) return fail("Cannot create staging directory.");
    sprintf(edited,"TWSTAGE1\r\n%s\r\n",app);
    if(!join(path,stage,"OWNER.DAT")||!putfile(path,edited))
        { rmdir(stage); return fail("Cannot mark the staging directory."); }
    return 1;
}
static int install(int installed) {
    unsigned i,j,owned; char src[PLEN],dst[PLEN],made[PLEN];
    if(!manifest()) return 0;
    if(!windowsdir(win)) return fail("Selected Windows does not match the supported real-mode system.");
    if(installed) {
        if(readstate()!=1) return fail("Installed state is damaged or selects another Windows directory.");
        if(!join(path,win,"SOUND.DRV")||exists(path))
            return fail("A new root SOUND.DRV belongs to another installation; no files changed.");
        for(i=0;i<nfiles;i++) if(files[i].kind!='S') {
            if(!join(path,app,files[i].name)) return 0;
            if(!exists(path)) continue;
            owned=0;
            for(j=0;j<nowned;j++)
                if(equal(files[i].name,ownedfiles[j].name)) owned=1;
            if(!owned&&!matches(path,files[i].bytes,files[i].crc))
                return fail("A new application filename belongs to another file; no files changed.");
        }
    } else {
        for(i=0;i<nfiles;i++) if(files[i].kind!='S') {
            if(!join(path,app,files[i].name)||
               (exists(path)&&!(equal(home,app)&&
                matches(path,files[i].bytes,files[i].crc))))
                return fail("Application directory contains another installation. Select an empty directory.");
        }
        if(!join(path,app,"GO.BAT")||exists(path)||
           !join(path,app,"PAYLOAD.LST")||
           (exists(path)&&!equal(home,app)))
            return fail("Application directory contains another installation.");
    }
    /* Inspect persisted FIXXT ownership before adopting newly-present paths. */
    if(!mirrorsoundownership()||!clientownership()) return 0;
    if(!prepare()) return 0;
    if(!join(dst,win,"SOUND.DRV")||!add(dst,0,1)) goto stagingfail;
    for(i=0;i<nfiles;i++) {
        if(!payloadsource(&files[i],src)) goto stagingfail;
        if(files[i].kind=='W'||files[i].kind=='X') {
            if(!join(dst,app,files[i].name)||!add(dst,src,0)) goto stagingfail;
            for(j=0;j<2;j++) {
                if(inputpolicy(&files[i],j,shellrequired())<0) continue;
                if(!clientpath(&files[i],dst,(int)j)) goto stagingfail;
                if(clientrecord((unsigned)clientpin(&files[i])-1,j)) {
                    if(!clientdirs(&files[i],(int)j)||!add(dst,src,0))
                        goto stagingfail;
                }
            }
            continue;
        }
        if(files[i].kind=='S') {
            if(!join(dst,win,"SYSTEM\\SOUND.DRV")) goto stagingfail;
        } else if(!join(dst,app,files[i].name)) goto stagingfail;
        if(!add(dst,src,0)) goto stagingfail;
        if(files[i].kind=='S'&&soundmirror) {
            if(!clientdirs(&files[i],1)||!clientpath(&files[i],dst,1)||
               !add(dst,src,0)) goto stagingfail;
        }
    }
    if(!join(src,home,"PAYLOAD.LST")||!join(dst,app,"PAYLOAD.LST")||
       !add(dst,src,0)) goto stagingfail;
    if(!join(made,stage,"GENERATE.TMP")) goto stagingfail;
    launchscript(edited,app,win);
    if(!putfile(made,edited)||!join(dst,app,"GO.BAT")||!add(dst,made,0))
        goto stagingfail;
    if(!join(src,win,"SYSTEM.INI")||!readtext(src,ini,sizeof(ini))||
       !changeini(ini,edited,"SOUND.DRV",selectedsound,1)||
       !putfile(made,edited)||!add(src,made,0)) goto stagingfail;
    if(!writestate(made)||!statepath(dst)||!add(dst,made,0)) goto stagingfail;
    validateinstall=1;
    if(!commit()) {
        failedinstallcleanup(installed);
        return 0;
    }
    printf("Installed Tandy sound and Winamp in %s.\n",app);
    printf("Run %s\\GO, or SETUP /LAUNCH.\n",app);
    return 1;
stagingfail:
    clearstage();
    failedinstallcleanup(installed);
    return fail("Could not stage the installation; no installed files changed.");
}
static int uninstall(void) {
    char dst[PLEN]; unsigned i;
    if(readstate()!=1) return fail("No valid installed state was found.");
    if(!prepare()) return fail("Cannot prepare removal.");
    for(i=0;i<nowned;i++) {
        if(!join(dst,app,ownedfiles[i].name)) goto bad;
        if(exists(dst)&&!matches(dst,ownedfiles[i].bytes,ownedfiles[i].crc)) {
            printf("Kept changed file: %s\n",dst); continue;
        }
        if(!add(dst,0,1)) goto bad;
    }
    if(!join(dst,app,"GO.BAT")) goto bad;
    if(exists(dst)&&!matches(dst,gobytes,gocrc))
        printf("Kept changed file: %s\n",dst);
    else if(!add(dst,0,1)) goto bad;
    if(!join(dst,app,"PAYLOAD.LST")||!add(dst,0,1)||
       !statepath(dst)||!add(dst,0,1)) goto bad;
    if(!commit()) return 0;
    rmdir(app);
    puts("Winamp removed. Shared Tandy sound and Windows XT clients remain installed.");
    return 1;
bad:
    clearstage(); return fail("Could not stage removal; no installed files changed.");
}
static int launch(void) {
    char launcher[PLEN]; int code;
    if(readstate()!=1||!windowsdir(win)) return fail("Run SETUP to install or repair Winamp first.");
    if(!join(path,win,"SYSTEM\\SOUND.DRV")||
       !matches(path,SOUND_BYTES,SOUND_CRC)||
       !join(launcher,app,"TPLAY.EXE")) return fail("Run SETUP /REPAIR first.");
    code=spawnl(P_WAIT,launcher,launcher,win,(char *)0);
    if(code) return fail("The launcher refused or returned an error.");
    return 1;
}
int main(int argc,char **argv) {
    int i,action=0,installed; char *p,answer[32],self[PLEN];
    strcpy(app,"C:\\WINAMP");
    if(!_fullpath(home,argv[0],sizeof(home))||(p=strrchr(home,'\\'))==0)
        return fail("Cannot locate Setup.")?0:2;
    strcpy(self,home);
    if(p==home+2) p[1]=0; else *p=0;
    if(absolute(home)&&join(path,home,"TWSTATE.DAT")&&exists(path)) strcpy(app,home);
    for(i=1;i<argc;i++) {
        p=argv[i];
        if(!strnicmp(p,"/WIN:",5)&&strlen(p+5)<PLEN) strcpy(win,p+5);
        else if(!strnicmp(p,"/APP:",5)&&strlen(p+5)<PLEN) strcpy(app,p+5);
        else if(equal(p,"/QUIET")) quiet=1;
        else if(equal(p,"/INSTALL")) action=1;
        else if(equal(p,"/REPAIR")) action=1;
        else if(equal(p,"/UNINSTALL")) action=2;
        else if(equal(p,"/LAUNCH")) action=3;
#ifdef SETUP_TEST
        else if(!strnicmp(p,"/FAIL:",6)) failafter=atoi(p+6);
#endif
        else { puts("SETUP [/INSTALL|/REPAIR|/UNINSTALL|/LAUNCH]");
            puts("      [/WIN:C:\\WINDOWS] [/APP:C:\\WINAMP] [/QUIET]");
            return 2; }
    }
    if((!absolute(home)&&!(strlen(home)==3&&home[1]==':'&&home[2]=='\\'))||
       !absolute(app)||(*win&&!absolute(win)))
        { fail("Use absolute DOS 8.3 directory names without a trailing slash."); return 2; }
    if(!plainDOS()) return 2;
    if(!sfxload(self)) return 2;
    if(!join(stage,app,"TWSTAGE")) return 2;
    if(exists(stage)) {
        if(!stageowned()) {
            if(rmdir(stage)) {
                fail("Reserved staging directory belongs to another program.");
                return 2;
            }
        } else {
        if(!rollback()) { fail("Could not recover the previous operation."); return 3; }
        if(!clearstage()) {
            fail("Cannot clean the saved private staging directory."); return 3;
        }
        }
    }
    installed=readstate();
    if(installed<0) { fail("Installed state is damaged or mismatched."); return 2; }
    if(!*win&&!choosewindows()) return 2;
    if(equal(app,win)) { fail("Choose a separate application directory."); return 2; }
    if(!action&&!quiet) {
        printf("Windows: %s\nWinamp:  %s\n",win,app);
        puts(installed?"1. Launch  2. Repair  3. Uninstall Winamp":"1. Install");
        printf("Choice: "); if(!fgets(answer,sizeof(answer),stdin)) return 2;
        i=atoi(answer);
        if(installed) action=i==1?3:(i==2?1:(i==3?2:0));
        else action=i==1?1:0;
        if(!action) return 2;
    }
    if(!action) action=1;
    if(action==3) return launch()?0:2;
    if(action==2) return uninstall()?0:2;
    return install(installed)?0:2;
}
