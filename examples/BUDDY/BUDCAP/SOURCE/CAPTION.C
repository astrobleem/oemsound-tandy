#include <stdio.h>
#include <string.h>
#include "CAPTION.H"
#include "CAPFONT.H"
#ifndef _cdecl
#define _cdecl
#endif
extern void _cdecl VideoRow(unsigned char *,unsigned,unsigned);
typedef struct {unsigned long when;char text[CAP_WIDTH+1];} CAPCUE;
static CAPCUE cues[CAP_MAX];
static unsigned count, nextCue, updates;
static int shown=-1;
static unsigned char row[160];
unsigned long CaptionNextTime(void)
{return nextCue<count?cues[nextCue].when:0xffffffffUL;}
unsigned CaptionCount(void) {return count;}
unsigned CaptionUpdates(void) {return updates;}
int CaptionLoad(char *name,unsigned long duration)
{
    FILE *f;char line[80],*p;unsigned len,i;unsigned long when;
    count=nextCue=updates=0;shown=-1;
    f=fopen(name,"r");if(!f)return 0;
    while(fgets(line,sizeof(line),f)) {
        len=(unsigned)strlen(line);
        if(len && line[len-1]!='\n' && !feof(f))goto bad;
        while(len && (line[len-1]=='\n'||line[len-1]=='\r'))line[--len]=0;
        if(!len||line[0]=='#')continue;
        if(count==CAP_MAX)goto bad;
        p=line;when=0;
        if(*p<'0'||*p>'9')goto bad;
        while(*p>='0'&&*p<='9') {
            if(when>60000UL)goto bad;
            when=when*10UL+(unsigned)(*p++-'0');
        }
        if(*p++!='|'||when>duration||(count&&when<=cues[count-1].when))goto bad;
        len=(unsigned)strlen(p);if(len>CAP_WIDTH)goto bad;
        for(i=0;i<len;i++) {
            if((unsigned char)p[i]<32||(unsigned char)p[i]>126)goto bad;
            if(p[i]>='a'&&p[i]<='z')p[i]-='a'-'A';
            if(p[i]>90)p[i]='?';
        }
        cues[count].when=when;strcpy(cues[count].text,p);count++;
    }
    if(ferror(f))goto bad;
    fclose(f);return 1;
bad:
    fclose(f);count=0;return -1;
}
int CaptionService(unsigned long now,int (*music)(void))
{
    int target=shown;unsigned y,i,len,x,offset;char *text;unsigned char *g;
    while(nextCue<count&&cues[nextCue].when<=now)target=nextCue++;
    if(target==shown)return 1;
    text=cues[target].text;len=(unsigned)strlen(text);x=(320-len*6)/4;
    for(y=0;y<8;y++) {
        if(music&&!music())return 0;
        memset(row,0,sizeof(row));
        if(y<7)for(i=0;i<len;i++) {
            g=glyph[(unsigned char)text[i]-32][y];
            row[x+i*3]=g[0];row[x+i*3+1]=g[1];row[x+i*3+2]=g[2];
        }
        offset=((186+y)&3)*8192+((186+y)>>2)*160;
        VideoRow(row,offset,sizeof(row));
    }
    shown=target;updates++;return 1;
}
