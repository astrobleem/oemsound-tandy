/* Original bounded MML2 exporter: no allocation, playback or file access. */
#include "BEATEXP.H"
static int put(char *out,unsigned cap,unsigned *n,const char *s)
{
    while(*s) { if(*n+1>=cap)return 0;out[(*n)++]=*s++; }
    out[*n]=0;return 1;
}
static int num(char *out,unsigned cap,unsigned *n,unsigned v)
{
    char b[6];unsigned k=0;
    do {b[k++]=(char)('0'+v%10);v/=10;}while(v);
    while(k) {char c[2];c[0]=b[--k];c[1]=0;if(!put(out,cap,n,c))return 0;}
    return 1;
}
unsigned BeatExport(char *out,unsigned cap,unsigned char pat[4][8],unsigned tempo)
{
    static const char *names[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    unsigned r,c,n=0,v;char part[6]="[A]\r\n";
    if(!out||!cap)return 0;
    out[0]=0;
    if(tempo<40||tempo>240)return 0;
    for(r=0;r<4;r++)for(c=0;c<8;c++) {
        v=pat[r][c];if(v&&(v<(r==3?35U:45U)||v>(r==3?81U:96U)))return 0;
    }
#define P(s) if(!put(out,cap,&n,s))goto fail
#define N(v) if(!num(out,cap,&n,v))goto fail
    P("MML2\r\n; One cycle\r\n");
    for(r=0;r<4;r++) {
        part[1]="ABCN"[r];P(part);P("T");N(tempo);
        P(r==3?" L8 V9\r\n":" L8 V11 @KEYS\r\n");
        for(c=0;c<8;c++) {
            v=pat[r][c];
            if(!v) {P("R8");}
            else {
                if(r==3){P("N");N(v);P("/16.");}
                else {P("O");N(v/12-1);P(" ");P(names[v%12]);P("16.");}
                P(" R32");
            }
            P(c==3||c==7?"\r\n":" ");
        }
    }
    P("; Drums included. 75% gate.\r\n; Base PSG attenuation preserved.\r\n");
    P("; Musical timing, not a live recording.\r\n");
    return n;
fail:out[0]=0;return 0;
#undef P
#undef N
}

/* Explicit P opt-in. Original BeatExport remains byte-identical. */
unsigned BeatExportPit(char *out,unsigned cap,unsigned char pat[5][8],
    unsigned tempo,int enabled)
{
    static const char *names[12]={"C","C#","D","D#","E","F",
        "F#","G","G#","A","A#","B"};
    unsigned n,c,v;
    if(!out || !cap)return 0;
    out[0]=0;
    if(!pat || (enabled!=0 && enabled!=1))return 0;
    if(enabled)for(c=0;c<8;++c)
        if(pat[4][c] && (pat[4][c]<45 || pat[4][c]>96))return 0;
    n=BeatExport(out,cap,pat,tempo);
    if(!n || !enabled)return n;
    out[3]='3';
#define P(s) if(!put(out,cap,&n,s))goto fail
#define N(v) if(!num(out,cap,&n,v))goto fail
    P("[P]\r\nT");N(tempo);P(" L8 V1\r\n");
    for(c=0;c<8;++c){
        v=pat[4][c];
        if(!v){P("R8");}
        else {P("O");N(v/12-1);P(" ");P(names[v%12]);
            P("16. R32");}
        P(c==3 || c==7?"\r\n":" ");
    }
    return n;
fail:out[0]=0;return 0;
#undef P
#undef N
}
