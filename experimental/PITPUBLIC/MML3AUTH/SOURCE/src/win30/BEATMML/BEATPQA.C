#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BEATEXP.H"
#include "MMLCORE.H"
static unsigned char pattern[5][8];
static char oldText[BEAT_MML_CAP],out[BEAT_MML_CAP],guard[BEAT_MML_CAP+2];
static MML_SCORE score;
static MML_ERROR error;
int main(void)
{
    unsigned t,n,c,k,cases=0;
    for(t=40;t<=240;t+=10){
        for(c=0;c<8;++c){pattern[0][c]=60;pattern[1][c]=64;
            pattern[2][c]=48;pattern[3][c]=42;}
        n=BeatExport(oldText,sizeof(oldText),pattern,t);assert(n);
        assert(BeatExportPit(out,sizeof(out),pattern,t,0)==n);
        assert(!strcmp(oldText,out));++cases;
        for(k=45;k<=96;++k){
            for(c=0;c<8;++c)pattern[4][c]=(unsigned char)k;
            n=BeatExportPit(out,sizeof(out),pattern,t,1);assert(n);
            assert(!strncmp(out,"MML3",4));
            assert(mml_validate(&score,out,n,0,&error));
            assert(score.version==3 && score.part_count==5 &&
                score.present[MML_PIT_PART]);
            assert(score.part_duration_ms[4]==score.part_duration_ms[0]);
            ++cases;
        }
    }
    n=BeatExportPit(out,sizeof(out),pattern,120,1);
    for(k=0;k<=n+1;++k){
        unsigned result;memset(guard,0x55,sizeof(guard));
        result=BeatExportPit(guard+1,k,pattern,120,1);
        assert(guard[0]==0x55 && guard[k+1]==0x55);
        if(k<=n){assert(!result);if(k)assert(guard[1]==0);}
        else assert(result==n);
    }
    pattern[4][0]=44;assert(!BeatExportPit(out,sizeof(out),pattern,120,1));
    assert(out[0]==0);assert(BeatExportPit(out,sizeof(out),pattern,120,0));
    pattern[4][0]=97;assert(!BeatExportPit(out,sizeof(out),pattern,120,1));
    assert(!BeatExportPit(out,sizeof(out),pattern,120,2));
    memset(pattern,0,sizeof(pattern));
    n=BeatExportPit(out,sizeof(out),pattern,120,1);assert(n);
    assert(strstr(out,"[P]\r\nT120 L8 V1\r\nR8 R8 R8 R8"));
    assert(mml_validate(&score,out,n,0,&error));
    assert(score.present[4] && score.duration_ms==2000);
    printf("PASS %u exports through original parser; explicit rest P, bounds, "
        "disabled legacy identity and invalid cases\n",cases);
    return 0;
}
