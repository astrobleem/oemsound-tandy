/* C89 tests for editable state, transactional decoding and real MML export. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BEATPAT.H"
#include "MMLCORE.H"
static unsigned checks;
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL core:%u: %s\n",(unsigned)__LINE__,#x); return 1; \
} } while(0)
static int same(const BPAT *a,const BPAT *b)
{
    return a->tempo==b->tempo&&a->pit==b->pit&&
        !memcmp(a->program,b->program,sizeof(a->program))&&
        !memcmp(a->note,b->note,sizeof(a->note));
}
static unsigned notes(const BPAT *p,unsigned lane)
{
    unsigned i,n=0;for(i=0;i<8;++i)if(p->note[lane][i])++n;return n;
}
static int unchanged_reject(BPAT *p,const char *data,unsigned n)
{
    BPAT before;memcpy(&before,p,sizeof(before));
    return BPDecode(p,data,n)!=BP_OK&&!memcmp(&before,p,sizeof(before));
}
static unsigned rng=12345;
static unsigned pick(unsigned n)
{
    rng=rng*25173U+13849U;return (rng>>2)%n;
}
int main(void)
{
    BPAT p,q,before;char data[BP_FILE_CAP+32],encoded[BP_FILE_CAP];
    char mml[BP_MML_CAP],guard[BP_MML_CAP+2];
    unsigned d,n,i,r,c,k,len,expected,seen,step;FILE *f;
    MML_SCORE score;MML_ITER iterator;MML_EVENT event;MML_ERROR error;
    const char *name;
    static const char *badnames[]={"", ".BTS", "TOOLONG99.BTS", "A..BTS",
        "A/B.BTS", "A\\B.BTS", "C:A.BTS", "../A.BTS", "A B.BTS",
        "CON.BTS", "prn.BTS", "AUX.BTS", "NUL.BTS", "COM1.BTS",
        "LPT9.BTS", "BEAT.MML", "A.BTS.X"};
    BPDefault(&p);BPDemo(&q,0);CK(same(&p,&q));
    CK(BPName("BEAT.BTS","BTS"));CK(BPName("beat_123.bts","BTS"));
    CK(BPName("A.MML","MML"));CK(!BPName("A.BTS","MML"));
    for(i=0;i<sizeof(badnames)/sizeof(badnames[0]);++i)CK(!BPName(badnames[i],"BTS"));
    for(d=0;d<5;++d){
        name=BPDemo(&p,d);CK(name&&*name);CK(BPValid(&p));
        if(d==0){CK(!p.pit);CK(notes(&p,0)==4&&notes(&p,1)==4&&notes(&p,2)==4&&notes(&p,3)==8);}
        if(d==1){CK(!p.pit);CK(!notes(&p,0)&&!notes(&p,1)&&!notes(&p,2)&&!notes(&p,4));CK(notes(&p,3)==8);}
        if(d==2){CK(notes(&p,0)&&notes(&p,2));CK(p.program[2]==32);CK(!notes(&p,4)&&!p.pit);}
        if(d==3){
            CK(p.pit);for(c=0;c<8;++c){CK(p.note[4][c]>=33&&p.note[4][c]<=38);
                if(c&1)for(r=0;r<4;++r)CK(!p.note[r][c]);}
            for(r=0;r<5;++r)CK(p.note[r][0]&&p.note[r][4]);
        }
        if(d==4){CK(p.program[0]==80&&p.program[1]==96&&p.program[2]==32);CK(!p.pit);}
        n=BPEncode(&p,encoded,sizeof(encoded));CK(n&&n<BP_FILE_CAP);
        memset(&q,0x5a,sizeof(q));CK(BPDecode(&q,encoded,n)==BP_OK);CK(same(&p,&q));
        for(i=0,k=0;i<n;++i)if(encoded[i]!='\r')data[k++]=encoded[i];
        CK(BPDecode(&q,data,k)==BP_OK&&same(&p,&q));
        CK(unchanged_reject(&q,encoded,4));CK(unchanged_reject(&q,encoded,n/2));
        memcpy(data,encoded,n);data[n]='X';CK(unchanged_reject(&q,data,n+1));
        memcpy(data,encoded,n);data[n]='1';CK(unchanged_reject(&q,data,n+1));
        memcpy(data,encoded,n);data[0]='X';CK(unchanged_reject(&q,data,n));
        memcpy(data,encoded,n);data[4]=0;CK(unchanged_reject(&q,data,n));
        for(k=0;k<n+2;++k){
            memset(guard,0x55,sizeof(guard));len=BPEncode(&p,guard+1,k);
            CK(guard[0]==0x55&&guard[k+1]==0x55);
            if(k<=n){CK(!len);if(k)CK(!guard[1]);}else CK(len==n);
        }
        n=BPMML(&p,mml,sizeof(mml));CK(n&&n<BP_MML_CAP);
        CK(mml_validate(&score,mml,n,0,&error));
        CK(score.part_count==(p.pit?5U:4U));CK(score.present[4]==p.pit);
        CK(score.low_pit==(d==3));
        for(r=0;r<score.part_count;++r){
            expected=notes(&p,r);CK(score.note_count[r]==8U+expected);
            mml_reset(&iterator,&score,r);seen=0;step=0;
            while((i=(unsigned)mml_next(&iterator,&event,&error))==1U){
                if(event.kind==MML_NOTEON){
                    while(step<8&&!p.note[r][step])++step;
                    CK(step<8&&event.pitch==p.note[r][step]);
                    CK(event.when_ms>=(30000UL*step)/p.tempo&&
                       event.when_ms<=(30000UL*step)/p.tempo+1UL);
                    if(r<3)CK(event.program==(p.program[r]&112U));
                    ++step;++seen;
                }
            }
            CK(i==0);CK(seen==expected);
        }
        for(k=0;k<=n+1;++k){
            memset(guard,0x55,sizeof(guard));len=BPMML(&p,guard+1,k);
            CK(guard[0]==0x55&&guard[k+1]==0x55);
            if(k<=n){CK(!len);if(k)CK(!guard[1]);}else CK(len==n);
        }
    }
    for(i=0;i<256;++i){
        p.tempo=40+pick(201);p.pit=(unsigned char)pick(2);
        for(r=0;r<3;++r)p.program[r]=(unsigned char)pick(128);
        for(r=0;r<5;++r)for(c=0;c<8;++c)p.note[r][c]=
            (unsigned char)(pick(4)?(r==3?35+pick(47):r==4?26+pick(71):45+pick(52)):0);
        CK(BPValid(&p));n=BPEncode(&p,encoded,sizeof(encoded));CK(n);
        CK(BPDecode(&q,encoded,n)==BP_OK&&same(&p,&q));
    }
    BPDemo(&p,3);before=p;p.tempo=39;CK(!BPValid(&p));p=before;
    p.tempo=241;CK(!BPValid(&p));p=before;p.pit=2;CK(!BPValid(&p));p=before;
    p.program[1]=128;CK(!BPValid(&p));p=before;
    for(r=0;r<5;++r){p.note[r][0]=(unsigned char)(r==3?34:r==4?25:44);CK(!BPValid(&p));p=before;
        p.note[r][0]=(unsigned char)(r==3?82:97);CK(!BPValid(&p));p=before;}
    BPDemo(&p,3);p.pit=0;p.note[4][0]=26;CK(BPValid(&p));
    n=BPEncode(&p,encoded,sizeof(encoded));CK(n&&BPDecode(&q,encoded,n)==BP_OK&&same(&p,&q));
    n=BPMML(&p,mml,sizeof(mml));CK(n&&!strstr(mml,"[P]"));
    CK(mml_validate(&score,mml,n,0,&error)&&score.version==2&&!score.present[4]&&!score.low_pit);
    p.pit=1;n=BPMML(&p,mml,sizeof(mml));CK(n);
    CK(mml_validate(&score,mml,n,0,&error)&&score.present[4]&&score.low_pit);
    strcpy(mml,"MML3\n[P]\nT120 O1 D4\n");
    CK(mml_validate(&score,mml,(unsigned)strlen(mml),0,&error));
    strcpy(mml,"MML3\n[P]\nT120 O1 C#4\n");
    CK(!mml_validate(&score,mml,(unsigned)strlen(mml),0,&error)&&error.code==MML_E_PITCH);
    strcpy(mml,"MML3\n[A]\nT120 O2 G#4\n[P]\nO1 D4\n");
    CK(!mml_validate(&score,mml,(unsigned)strlen(mml),0,&error)&&error.code==MML_E_PITCH);
    strcpy(mml,"MML3\n[A]\nT120 O1 D4\n[P]\nO1 D4\n");
    CK(!mml_validate(&score,mml,(unsigned)strlen(mml),0,&error));
    strcpy(mml,"MML3\n[P]\nT120 O7 C4\n");
    CK(mml_validate(&score,mml,(unsigned)strlen(mml),0,&error));
    strcpy(mml,"MML3\n[P]\nT120 O7 C#4\n");
    CK(!mml_validate(&score,mml,(unsigned)strlen(mml),0,&error)&&error.code==MML_E_PITCH);
    memset(data,' ',sizeof(data));memcpy(data,"DWA1",4);
    CK(unchanged_reject(&p,data,sizeof(data)));
    CK(unchanged_reject(&p,"DWA1\n9999 0 0 0 0\n",(unsigned)strlen("DWA1\n9999 0 0 0 0\n")));
    CK(unchanged_reject(&p,"DWA1\n-1 0 0 0 0\n",(unsigned)strlen("DWA1\n-1 0 0 0 0\n")));
    BPDemo(&p,0);CK(BPSave(&p,"PATTERN.BTS",0)==BP_OK);
    CK(BPLoad(&q,"PATTERN.BTS")==BP_OK&&same(&p,&q));
    BPDemo(&before,3);CK(BPSave(&before,"PATTERN.BTS",0)==BP_EXISTS);
    CK(BPLoad(&q,"PATTERN.BTS")==BP_OK&&same(&p,&q));
    CK(BPSave(&before,"PATTERN.BTS",1)==BP_OK);
    CK(BPLoad(&q,"PATTERN.BTS")==BP_OK&&same(&before,&q));
    CK(BPLoad(&q,"PATTERN.BAK")==BP_OK&&same(&p,&q));
    f=fopen("PATTERN.$$$","rb");CK(!f);
    f=fopen("BLOCK.$$$","wb");CK(f!=0);CK(fwrite("do not replace",1,14,f)==14);CK(!fclose(f));
    CK(BPSave(&p,"BLOCK.BTS",0)==BP_EXISTS);
    f=fopen("BLOCK.$$$","rb");CK(f!=0);n=(unsigned)fread(data,1,sizeof(data),f);fclose(f);
    CK(n==14&&!memcmp(data,"do not replace",14));
    CK(BPLoad(&q,"MISSING.BTS")==BP_OPEN);
    f=fopen("BROKEN.BTS","wb");CK(f!=0);CK(fwrite("DWA1\nno pattern",1,15,f)==15);fclose(f);
    before=q;CK(BPLoad(&q,"BROKEN.BTS")==BP_FORMAT);CK(same(&q,&before));
    f=fopen("LARGE.BTS","wb");CK(f!=0);
    for(i=0;i<BP_FILE_CAP+1;++i)fputc(' ',f);fclose(f);
    before=q;CK(BPLoad(&q,"LARGE.BTS")==BP_FORMAT&&same(&q,&before));
    CK(BPSave(&p,"missing/PATTERN.BTS",0)==BP_OPEN);
    printf("PASS pattern/codec/files/MML checks=%u original_demos=%u\n",checks,(unsigned)BP_DEMOS);
    return 0;
}
