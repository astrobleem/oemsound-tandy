/* Fresh app-local DOS installer. No Windows/driver/INI/boot changes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <sys/stat.h>
#include "PAYLOAD.H"
static unsigned char block[1024];
static char source[128],target[128],path[144];
/* One table lookup per byte. Explicit byte moves avoid an expensive
   generic 32-bit shift on 8088; CRC polynomial/result remain unchanged. */
#ifdef __WATCOMC__
static unsigned long CrcBlock(unsigned,unsigned,const unsigned char *,
    unsigned,const unsigned long *);
#pragma aux CrcBlock = \
    "jcxz crc_done" \
    "crc_next:" \
    "xor al,[si]" \
    "xor bh,bh" \
    "mov bl,al" \
    "shl bx,1" \
    "shl bx,1" \
    "mov al,ah" \
    "mov ah,dl" \
    "mov dl,dh" \
    "xor dh,dh" \
    "xor ax,[bx+di]" \
    "xor dx,[bx+di+2]" \
    "inc si" \
    "loop crc_next" \
    "crc_done:" \
    parm [ax] [dx] [si] [cx] [di] value [dx ax] \
    modify exact [ax bx cx dx si];
#else
static unsigned long CrcBlock(unsigned lo,unsigned hi,const unsigned char *p,
    unsigned count,const unsigned long *table)
{
    unsigned i;unsigned long crc=(lo&65535U)|((unsigned long)hi<<16);
    for(i=0;i<count;i++)crc=(crc>>8)^table[(unsigned)(crc^p[i])&255];
    return crc;
}
#endif
static unsigned long checksum(FILE *f,unsigned long *bytes)
{
    unsigned n;unsigned long crc=0xffffffffUL;*bytes=0;
    while((n=fread(block,1,sizeof(block),f))!=0) {
        *bytes+=n;
        crc=CrcBlock((unsigned)crc,(unsigned)(crc>>16),block,n,crcTable);
    }
    return crc^0xffffffffUL;
}
static void fullpath(char *out,const char *folder,const char *name)
{
    strcpy(out,folder);strcat(out,"\\");strcat(out,name);
}
static int verify(const char *folder)
{
    unsigned i;unsigned long size,crc;FILE *f;
    for(i=0;i<PAYLOAD_COUNT;i++) {
        fullpath(path,folder,payload[i].name);f=fopen(path,"rb");
        if(!f){printf("Missing: %s\n",path);return 0;}
        crc=checksum(f,&size);
        if(ferror(f)){fclose(f);printf("Read error: %s\n",path);return 0;}
        fclose(f);
        if(size!=payload[i].bytes||crc!=payload[i].crc) {
            printf("Content mismatch: %s\n",path);return 0;
        }
    }
    printf("Verified %u files in %s\n",PAYLOAD_COUNT,folder);return 1;
}
static int validTarget(const char *s)
{
    unsigned n,i,part=0;
    n=(unsigned)strlen(s);
    if(n<4||n>100||!isalpha((unsigned char)s[0])||s[1]!=':'||s[2]!='\\')return 0;
    for(i=3;i<n;i++) {
        if(s[i]=='\\') {if(!part)return 0;part=0;}
        else {if(!(isalnum((unsigned char)s[i])||s[i]=='_'||s[i]=='-'))return 0;
            if(++part>8)return 0;}
    }
    return part!=0;
}
int main(int argc,char **argv)
{
    unsigned i,n,created=0;int check=0,ok=0;FILE *in=0,*out=0;struct stat st;
    unsigned long freeNeeded=0;
    puts("Buddy Windows installer - fresh directory only");
    if(argc>1&&!strcmp(argv[1],"/CHECK")){check=1;argv++;argc--;}
    if(argc>2){puts("BUDSET [X:\\FOLDER] or BUDSET /CHECK X:\\FOLDER");return 2;}
    strcpy(target,argc==2?argv[1]:"C:\\BUDDY");
    if(!validTarget(target)){puts("Use an absolute DOS 8.3 directory (no dots or spaces).");return 2;}
    if(check)return verify(target)?0:3;
    if(!getcwd(source,sizeof(source))){puts("Cannot read source directory.");return 2;}
    /* Run BUDSET while in the extracted package directory. */
    if(!verify(source)){puts("Source check failed; nothing installed.");return 3;}
    if(!stat(target,&st)){puts("Target exists. Choose a new directory; nothing overwritten.");return 4;}
    for(i=0;i<PAYLOAD_COUNT;i++)freeNeeded+=payload[i].bytes;
    printf("Installing %lu bytes to %s\n",freeNeeded,target);
    if(mkdir(target)){puts("Cannot create target. Its parent must exist.");return 4;}
    for(i=0;i<PAYLOAD_COUNT;i++) {
        fullpath(path,source,payload[i].name);in=fopen(path,"rb");
        fullpath(path,target,payload[i].name);out=fopen(path,"wb");
        if(out)created=i+1;
        if(!in||!out)goto done;
        while((n=fread(block,1,sizeof(block),in))!=0)
            if(fwrite(block,1,n,out)!=n)goto done;
        if(ferror(in))goto done;
        fclose(in);in=0;
        if(fclose(out)){out=0;goto done;}out=0;
    }
    if(!verify(target))goto done;
    ok=1;
done:
    if(in)fclose(in);if(out)fclose(out);
    if(!ok) {
        /* Only files created in this newly created directory are removed. */
        for(i=0;i<created;i++){fullpath(path,target,payload[i].name);remove(path);}
        rmdir(target);puts("Install failed; new files rolled back.");return 5;
    }
    puts("Installed. In Windows, use File/Run and select TINYVID.EXE there.");
    puts("B = PC speaker; P = configured Tandy-compatible C0 card; A = Auto.");
    puts("No system files, INI files, PATH or boot files were changed.");return 0;
}
