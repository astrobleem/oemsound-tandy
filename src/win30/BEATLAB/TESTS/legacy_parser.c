/* Establish the archived parser's low-P rejection and unchanged old range. */
#include <stdio.h>
#include <string.h>
#include "BEATPAT.H"
#include "MMLCORE.H"
static unsigned checks;
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL archived parser:%u: %s\n",(unsigned)__LINE__,#x);return 1; \
} } while(0)
int main(void)
{
    BPAT p;MML_SCORE score;MML_ERROR error;char text[BP_MML_CAP];unsigned n,d,low_error=0;
    for(d=0;d<5;++d){
        BPDemo(&p,d);n=BPMML(&p,text,sizeof(text));CK(n);
        if(d==3){CK(!mml_validate(&score,text,n,0,&error)&&error.code!=MML_E_NONE);low_error=error.code;}
        else CK(mml_validate(&score,text,n,0,&error));
    }
    BPDemo(&p,3);memset(p.note[4],45,sizeof(p.note[4]));
    n=BPMML(&p,text,sizeof(text));CK(n&&mml_validate(&score,text,n,0,&error));
    p.pit=0;p.note[4][0]=26;n=BPMML(&p,text,sizeof(text));
    CK(n&&mml_validate(&score,text,n,0,&error)&&score.version==2&&!score.present[4]);
    printf("PASS archived parser low-P rejection/old-range checks=%u rejection_code=%u\n",checks,low_error);return 0;
}
