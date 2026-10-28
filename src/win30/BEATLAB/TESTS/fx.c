/* Exercise actual compiler, merger, producer and transactional song files. */
#include <stdio.h>
#include <string.h>
#include "BEATPLAY.H"
static BBPLAYER player;
static BPAT p,q,keep;
static unsigned checks;
#define CK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL fx:%u %s\n",(unsigned)__LINE__,#x);return 1;}}while(0)
static unsigned period[3],level[4],noise;
static void apply(const BBBURST *b)
{
    unsigned i,v,r=0;
    for(i=0;i<b->count;++i){v=b->data[i];
        if(v&128)r=(v>>4)&7;
        if(r&1)level[r>>1]=v&15;
        else if(r==6)noise=v&7;
        else if(v&128)period[r>>1]=(period[r>>1]&1008)|(v&15);
        else period[r>>1]=(period[r>>1]&15)|((v&63)<<4);
    }
}
int main(void)
{
    static char text[BP_FILE_CAP];FXMSTATE base,out,old;
    FXMLAYER fx[5];BBBURST burst;unsigned r,c,n,k,t,noiseWrites;
    unsigned long prior=0;FILE *f;
    memset(&base,0,sizeof(base));memset(fx,0,sizeof(fx));
    for(r=0;r<3;++r){base.period[r]=200+r*100;base.level[r]=r+3;}
    base.noise=4;base.level[3]=8;base.pit=21000;
    FXMMerge(&out,&base,fx);CK(!memcmp(&out,&base,sizeof(out)));
    fx[0].active=1;fx[0].period=90;fx[0].level=1;
    FXMMerge(&out,&base,fx);CK(out.period[0]==90&&out.level[0]==1);
    CK(out.level[1]==4&&out.level[2]==5&&out.level[3]==8&&out.pit==21000);
    fx[2].active=1;fx[2].period=99;fx[2].level=0;
    fx[3].active=fx[3].borrow2=1;fx[3].period=711;fx[3].noise=7;fx[3].level=2;
    FXMMerge(&out,&base,fx);CK(out.period[2]==711&&out.level[2]==15&&out.noise==7);
    CK(out.level[0]==1&&out.level[1]==4&&out.pit==21000);
    /* A gated noise row still owns tone2, avoiding accidental whistle. */
    fx[3].level=15;FXMMerge(&out,&base,fx);CK(out.level[2]==15);
    fx[3].active=0;fx[2].period=123;FXMMerge(&out,&base,fx);
    CK(out.period[2]==123&&out.level[2]==0&&out.noise==4&&out.level[3]==8);
    fx[4].active=1;fx[4].pit=5432;FXMMerge(&out,&base,fx);CK(out.pit==5432);
    fx[4].active=0;FXMMerge(&out,&base,fx);CK(out.pit==21000);
    old=out;CK(FXMBytes(&out,&old,0,0,burst.data)==0);
    out.level[3]=3;n=FXMBytes(&out,&old,0,0,burst.data);
    CK(n==1&&burst.data[0]==0xf3);
    CK(FXMBytes(&out,&old,0,1,burst.data)==2);
    CK(FXMBytes(&out,&old,1,0,burst.data)==11);
    CK(BBTranspose(400,60,1,1023)==400);
    CK(BBTranspose(400,72,1,1023)==200);
    CK(BBTranspose(400,48,1,1023)==800);
    CK(BBTranspose(0,60,151,65535)==0);
    CK(BBTranspose(151,96,151,65535)==151);
    CK(BBTranspose(65535,26,151,65535)==65535);
    BPDemo(&p,5);CK(BPHasFX(&p)&&BPValid(&p));
    n=BPEncode(&p,text,sizeof(text));CK(n&&n<BP_FILE_CAP);
    memset(&q,0xaa,sizeof(q));CK(BPDecode(&q,text,n)==0&&!memcmp(&p,&q,sizeof(p)));
    keep=q;
    for(k=0;k<n-3;++k){CK(BPDecode(&q,text,k)==BP_FORMAT);CK(!memcmp(&q,&keep,sizeof(q)));}
    text[n]='X';CK(BPDecode(&q,text,n+1)==BP_FORMAT&&!memcmp(&q,&keep,sizeof(q)));
    CK(BPMML(&p,text,sizeof(text))==0&&text[0]==0);
    /* All five imports, including extreme editable parameters and seed. */
    for(r=0;r<5;++r){p.loaded[r]=1;FXPreset(&p.patch[r],r==3?2:0);
        p.patch[r].seed=65535;p.patch[r].v[FX_SLIDE]=-7800;
        p.patch[r].v[FX_TARGET]=r==4;memset(p.fxName[r],0,13);strcpy(p.fxName[r],"TEST.FXP");
        for(c=0;c<8;++c){p.note[r][c]=60;p.trigger[r][c]=1;}}
    CK(BPValid(&p));n=BPEncode(&p,text,sizeof(text));CK(n);
    CK(BPDecode(&q,text,n)==0&&!memcmp(&p,&q,sizeof(p)));
    CK(BPSave(&p,"FXSAVE.BTS",0)==0);CK(BPLoad(&q,"FXSAVE.BTS")==0&&!memcmp(&p,&q,sizeof(p)));
    q.patch[0].v[FX_TARGET]=1;CK(!BPValid(&q));
    BPDefault(&p);n=BPEncode(&p,text,sizeof(text));CK(n);
    /* The legacy DWA1 prefix still loads and has no accidental FX state. */
    for(k=0,c=0;k<n&&c<7;++k)if(text[k]=='\n')++c;
    text[3]='1';memset(&q,0xaa,sizeof(q));CK(BPDecode(&q,text,k)==0&&!BPHasFX(&q));
    for(r=0;r<5;++r)CK(!q.loaded[r]);
    BPDemo(&p,5);CK(BPSave(&p,"FXGROOVE.BTS",0)==0);
    for(r=0;r<5;++r)if(p.loaded[r])CK(FXSave(&p.patch[r],p.fxName[r],0)==0);
    CK(BBPrepare(&player,&p));noiseWrites=0;
    f=fopen("FXTRACE.CSV","wb");CK(f!=0);
    fprintf(f,"tick,p0,p1,p2,v0,v1,v2,v3,noise,pit,writes\n");
    for(t=0;t<1200;++t){CK(BBNext(&player,&burst));CK(burst.count<=11);
        CK(burst.at==BBStamp(t)&&(!t||burst.at-prior>=19886UL));prior=burst.at;
        for(k=0;k<burst.count;++k)if((burst.data[k]&0xf0)==0xe0)++noiseWrites;
        apply(&burst);
        for(r=0;r<3;++r)CK(period[r]==player.old.period[r]&&period[r]<=1023);
        for(r=0;r<4;++r)CK(level[r]==player.old.level[r]&&level[r]<=15);
        CK(noise==player.old.noise&&burst.pit==player.old.pit);
        if(t<400)fprintf(f,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",t,period[0],period[1],period[2],level[0],level[1],level[2],level[3],noise,burst.pit,burst.count);
    }
    CK(!fclose(f));CK(noiseWrites<200); /* no per-tick LFSR reset */
    /* No source patch or producer state aliases the editable song. */
    keep=player.song;p.patch[0].v[FX_PITCH]=100;CK(!memcmp(&keep,&player.song,sizeof(keep)));
    BPDefault(&p);p.tempo=40;CK(BBPrepare(&player,&p));
    for(t=0;t<1000;++t){CK(BBNext(&player,&burst));CK(player.lastStep==(t/45)%8);}
    p.tempo=240;CK(BBPrepare(&player,&p));
    for(t=0;t<1000;++t){CK(BBNext(&player,&burst));CK(player.lastStep==((t*2U)/15)%8);}
    /* The final bounded session state is an explicit PSG+speaker mute. */
    player.tick=BB_LIMIT;CK(BBNext(&player,&burst));CK(!burst.pit);
    for(r=0;r<4;++r)CK(player.old.level[r]==15);
    CK(!BBNext(&player,&burst));CK(BBStamp(BB_LIMIT)<2147483647UL);
    printf("PASS FX merger/producer/files checks=%u noise_latches=%u\n",checks,noiseWrites);
    return 0;
}
