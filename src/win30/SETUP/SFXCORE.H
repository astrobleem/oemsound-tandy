#ifndef SFXCORE_H
#define SFXCORE_H
#include <string.h>
/* Stored native DOS bundle format. All offsets are unsigned little endian. */
#define SXMAX 64
#define SXFOOT 32
#define SXENTRYBYTES 32
typedef struct {
    unsigned long image,bundle,count,tablecrc,bundlecrc;
} SXARCHIVE;
typedef struct {
    char name[13];
    unsigned long offset,bytes,crc;
} SXENTRY;
static unsigned long SXU32(const unsigned char *p) {
    return (unsigned long)p[0]|((unsigned long)p[1]<<8)|
        ((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);
}
static unsigned long SXImage(const unsigned char *mz) {
    unsigned last,pages; unsigned long bytes;
    last=(unsigned)mz[2]+((unsigned)mz[3]<<8);
    pages=(unsigned)mz[4]+((unsigned)mz[5]<<8);
    if(mz[0]!='M'||mz[1]!='Z'||!pages||last>=512U) return 0;
    bytes=((unsigned long)pages-1UL)*512UL+(last?last:512U);
    return bytes>=64UL&&bytes<=131072UL?bytes:0UL;
}
static int SXEqual(const char *a,const char *b) {
    unsigned x,y;
    do {
        x=(unsigned char)*a++; y=(unsigned char)*b++;
        if(x>='a'&&x<='z') x-='a'-'A';
        if(y>='a'&&y<='z') y-='a'-'A';
        if(x!=y) return 0;
    } while(x);
    return 1;
}
static int SXName(const char *name) {
    char base[9]; unsigned i=0,n=0,dot=0,c; const char *p=name;
    while(*p&&*p!='.'&&i<8) {
        c=(unsigned char)*p++;
        base[i++]=(char)((c>='a'&&c<='z')?c-('a'-'A'):c);
    }
    base[i]=0;
    if(SXEqual(base,"CON")||SXEqual(base,"PRN")||SXEqual(base,"AUX")||
       SXEqual(base,"NUL")||SXEqual(base,"CLOCK$")||
       ((i==4)&&(base[3]>='1'&&base[3]<='9')&&
        ((!strncmp(base,"COM",3))||(!strncmp(base,"LPT",3))))) return 0;
    for(p=name;*p;p++) {
        c=(unsigned char)*p;
        if(c=='.') { if(!n||dot) return 0; dot=1;n=0;continue; }
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='_'||c=='-'||c=='$'||c=='~')) return 0;
        if(++n>(dot?3U:8U)) return 0;
    }
    return n!=0&&!SXEqual(name,"SFXOWN.DAT");
}
static int SXFooter(const unsigned char *raw,unsigned long length,
                    SXARCHIVE *a) {
    if(memcmp(raw,"TWINSFX1",8)||SXU32(raw+28)) return 0;
    a->image=SXU32(raw+8); a->bundle=SXU32(raw+12);
    a->count=SXU32(raw+16); a->tablecrc=SXU32(raw+20);
    a->bundlecrc=SXU32(raw+24);
    return length>=SXFOOT&&a->image>=64UL&&a->image<=131072UL&&
        a->count>=1UL&&a->count<=SXMAX&&
        a->bundle>=a->count*SXENTRYBYTES&&a->bundle<=4194304UL&&
        a->image<=length-SXFOOT&&
        a->bundle==length-SXFOOT-a->image;
}
static int SXDecode(const unsigned char *raw,unsigned long expected,
                    unsigned long total,SXENTRY *entry) {
    unsigned i;
    for(i=0;i<13&&raw[i];i++);
    if(!i||i==13||raw[13]||raw[14]||raw[15]||SXU32(raw+28)) return 0;
    memcpy(entry->name,raw,13); entry->name[12]=0;
    /* Unused name bytes are zero; there are no hidden alternate names. */
    while(i<13) if(raw[i++]) return 0;
    entry->offset=SXU32(raw+16);entry->bytes=SXU32(raw+20);
    entry->crc=SXU32(raw+24);
    return SXName(entry->name)&&entry->offset==expected&&
        entry->bytes<=1048576UL&&expected<=total&&
        entry->bytes<=total-expected;
}
static int SXUnique(const SXENTRY *entries,unsigned count,const char *name) {
    unsigned i;
    for(i=0;i<count;i++) if(SXEqual(entries[i].name,name)) return 0;
    return 1;
}
#endif
