/* Fixed-size editable patterns; no timing, hardware or bank implementation. */
#include "BEATPAT.H"
#include "BEATEXP.H"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
static const char *presets[8]={"KEYS","ORGAN","BASS","PAD","REED","LEAD","BELL","HIT"};
static const char *names[BP_DEMOS]={"Starter","Pocket drums","Lowrider lead","Speaker bass","Late-night keys"};
static const unsigned tempos[BP_DEMOS]={120,90,92,88,90};
static const unsigned char programs[BP_DEMOS][3]={{0,0,0},{0,0,0},{80,16,32},{80,16,0},{80,96,32}};
static const unsigned char demos[BP_DEMOS][5][8]={
 {{60,0,67,0,64,0,72,0},{64,0,0,67,67,0,0,71},{48,0,48,0,53,0,55,0},{42,42,38,42,42,42,38,42},{0,0,0,0,0,0,0,0}},
 {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{36,42,38,42,36,42,38,46},{0,0,0,0,0,0,0,0}},
 {{81,0,79,0,76,0,0,79},{0,0,72,0,0,0,71,0},{45,0,45,0,48,0,50,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}},
 {{81,0,79,0,84,0,81,0},{72,0,0,0,76,0,0,0},{64,0,0,0,67,0,0,0},{42,0,38,0,42,0,38,0},{45,45,48,45,45,48,50,45}},
 {{81,0,84,0,79,0,76,0},{0,0,0,72,0,0,74,0},{45,0,0,45,48,0,50,0},{42,0,38,0,42,0,38,0},{0,0,0,0,0,0,0,0}}
};
const char *BPPreset(unsigned p){return presets[(p>>4)&7];}
const char *BPDemo(BPAT *p,unsigned n)
{
    if(n>=BP_DEMOS)n=0;
    p->tempo=tempos[n];p->pit=(unsigned char)(n==3);
    memcpy(p->program,programs[n],sizeof(p->program));
    memcpy(p->note,demos[n],sizeof(p->note));return names[n];
}
void BPDefault(BPAT *p){BPDemo(p,0);}
int BPValid(const BPAT *p)
{
    unsigned r,c,n;
    if(!p||p->tempo<40||p->tempo>240||p->pit>1)return 0;
    for(r=0;r<3;++r)if(p->program[r]>127)return 0;
    for(r=0;r<5;++r)for(c=0;c<8;++c){n=p->note[r][c];
        if(n&&(n<(r==3?35U:45U)||n>(r==3?81U:96U)))return 0;}
    return 1;
}
unsigned BPEncode(const BPAT *p,char *out,unsigned cap)
{
    char text[BP_FILE_CAP];unsigned n,r,c;
    if(!out||!cap||!BPValid(p))return 0;
    n=(unsigned)sprintf(text,"DWA1\r\n%u %u %u %u %u\r\n",p->tempo,(unsigned)p->pit,
        (unsigned)p->program[0],(unsigned)p->program[1],(unsigned)p->program[2]);
    for(r=0;r<5;++r)for(c=0;c<8;++c)
        n+=(unsigned)sprintf(text+n,c==7?"%u\r\n":"%u ",(unsigned)p->note[r][c]);
    if(n+1>cap){out[0]=0;return 0;}
    memcpy(out,text,n+1);return n;
}
static int number(const char *s,unsigned n,unsigned *at,unsigned *value)
{
    unsigned v=0,k=0;
    while(*at<n&&(s[*at]==' '||s[*at]=='\r'||s[*at]=='\n'||s[*at]=='\t'))++*at;
    while(*at<n&&s[*at]>='0'&&s[*at]<='9'){
        v=v*10U+(unsigned)(s[(*at)++]-'0');if(++k>3)return 0;}
    if(!k||(*at<n&&s[*at]!=' '&&s[*at]!='\r'&&s[*at]!='\n'&&s[*at]!='\t'))return 0;
    *value=v;return 1;
}
int BPDecode(BPAT *out,const char *s,unsigned n)
{
    BPAT p;unsigned at=4,v,r,c;
    if(!out||!s||n<5||n>BP_FILE_CAP||memcmp(s,"DWA1",4)||
       (s[4]!='\r'&&s[4]!='\n'))return BP_FORMAT;
    if(!number(s,n,&at,&p.tempo)||!number(s,n,&at,&v)||v>1)return BP_FORMAT;
    p.pit=(unsigned char)v;
    for(r=0;r<3;++r){if(!number(s,n,&at,&v)||v>127)return BP_FORMAT;p.program[r]=(unsigned char)v;}
    for(r=0;r<5;++r)for(c=0;c<8;++c){
        if(!number(s,n,&at,&v)||v>127)return BP_FORMAT;p.note[r][c]=(unsigned char)v;}
    while(at<n&&(s[at]==' '||s[at]=='\t'||s[at]=='\r'||s[at]=='\n'))++at;
    if(at!=n||!BPValid(&p))return BP_FORMAT;
    *out=p;return BP_OK;
}
int BPName(const char *name,const char *ext)
{
    unsigned i,n=0;char stem[9];
    if(!name||!ext)return 0;
    while(name[n]&&name[n]!='.'&&n<9){
        if(!isalnum((unsigned char)name[n])&&name[n]!='_')return 0;
        stem[n]=(char)toupper((unsigned char)name[n]);++n;}
    if(!n||n>8||name[n]!='.'||strlen(name+n+1)!=3)return 0;
    stem[n]=0;
    for(i=0;i<3;++i)if(toupper((unsigned char)name[n+1+i])!=toupper((unsigned char)ext[i]))return 0;
    if(!strcmp(stem,"CON")||!strcmp(stem,"PRN")||!strcmp(stem,"AUX")||!strcmp(stem,"NUL"))return 0;
    if(n==4&&(!strncmp(stem,"COM",3)||!strncmp(stem,"LPT",3))&&stem[3]>='1'&&stem[3]<='9')return 0;
    return 1;
}
int BPLoad(BPAT *p,const char *path)
{
    FILE *f;char data[BP_FILE_CAP];unsigned n;int more,bad;
    f=fopen(path,"rb");if(!f)return BP_OPEN;
    n=(unsigned)fread(data,1,sizeof(data),f);more=fgetc(f);bad=ferror(f);fclose(f);
    if(bad)return BP_IO;if(more!=EOF)return BP_FORMAT;
    return BPDecode(p,data,n);
}
int BPSave(const BPAT *p,const char *path,int overwrite)
{
    char data[BP_FILE_CAP],temp[260],backup[260];unsigned n,len;FILE *f;int exists,bad;
    n=BPEncode(p,data,sizeof(data));if(!n)return BP_FORMAT;
    len=(unsigned)strlen(path);if(len<4||len>=sizeof(temp))return BP_NAME;
    strcpy(temp,path);strcpy(temp+len-3,"$$$");
    strcpy(backup,path);strcpy(backup+len-3,"BAK");
    if(!strcmp(temp,path)||!strcmp(backup,path))return BP_NAME;
    f=fopen(path,"rb");exists=f!=0;if(f)fclose(f);
    if(exists&&!overwrite)return BP_EXISTS;
    f=fopen(temp,"rb");if(f){fclose(f);return BP_EXISTS;}
    f=fopen(temp,"wb");if(!f)return BP_OPEN;
    bad=fwrite(data,1,n,f)!=n;if(fclose(f))bad=1;
    if(bad){remove(temp);return BP_IO;}
    if(exists){
        f=fopen(backup,"rb");if(f){fclose(f);if(remove(backup)){remove(temp);return BP_IO;}}
        if(rename(path,backup)){remove(temp);return BP_IO;}
    }
    if(rename(temp,path)){
        if(exists&&rename(backup,path))return BP_RECOVER;
        remove(temp);return BP_IO;
    }
    return BP_OK;
}
unsigned BPMML(const BPAT *p,char *out,unsigned cap)
{
    static char base[BP_MML_CAP];static unsigned char notes[5][8];
    unsigned n,i=0,j=0,part=0,k;const char *s;
    if(!out||!cap||!BPValid(p))return 0;
    memcpy(notes,p->note,sizeof(notes));
    n=BeatExportPit(base,sizeof(base),notes,p->tempo,p->pit);if(!n)return 0;
    while(i<n){
        if(part<3&&i+5<=n&&!memcmp(base+i,"@KEYS",5)){
            s=BPPreset(p->program[part++]);if(j+1>=cap)goto fail;out[j++]='@';
            for(k=0;s[k];++k){if(j+1>=cap)goto fail;out[j++]=s[k];}i+=5;
        }else {if(j+1>=cap)goto fail;out[j++]=base[i++];}
    }
    out[j]=0;return j;
fail:out[0]=0;return 0;
}
