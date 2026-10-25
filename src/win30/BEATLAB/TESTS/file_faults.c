/* Failure injection through the actual save implementation and libc I/O. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "BEATPAT.H"
static int fault;
static int ending(const char *s,const char *tail)
{
    unsigned n=(unsigned)strlen(s),k=(unsigned)strlen(tail);
    return n>=k&&!strcmp(s+n-k,tail);
}
static int checked_rename(const char *from,const char *to)
{
    if((fault&&ending(from,".$$$"))||(fault==2&&ending(from,".BAK"))){
        errno=EACCES;return -1;
    }
    return rename(from,to);
}
static size_t checked_write(const void *p,size_t size,size_t count,FILE *f)
{
    if(fault==3){if(count)fwrite(p,size,count/2,f);return count/2;}
    return fwrite(p,size,count,f);
}
#define rename checked_rename
#define fwrite checked_write
#include "../BEATPAT.C"
#undef rename
#undef fwrite
static unsigned checks;
#define CK(x) do { ++checks; if(!(x)) { \
    fprintf(stderr,"FAIL file_faults:%u: %s\n",(unsigned)__LINE__,#x); return 1; \
} } while(0)
static int same(const BPAT *a,const BPAT *b)
{
    return a->tempo==b->tempo&&a->pit==b->pit&&
        !memcmp(a->program,b->program,sizeof(a->program))&&
        !memcmp(a->note,b->note,sizeof(a->note));
}
int main(void)
{
    BPAT old,next,got;FILE *f;
    BPDemo(&old,0);BPDemo(&next,3);
    CK(BPSave(&old,"ROLLBACK.BTS",0)==BP_OK);
    fault=1;CK(BPSave(&next,"ROLLBACK.BTS",1)==BP_IO);fault=0;
    CK(BPLoad(&got,"ROLLBACK.BTS")==BP_OK&&same(&got,&old));
    f=fopen("ROLLBACK.$$$","rb");CK(!f);
    fault=3;CK(BPSave(&next,"ROLLBACK.BTS",1)==BP_IO);fault=0;
    CK(BPLoad(&got,"ROLLBACK.BTS")==BP_OK&&same(&got,&old));
    f=fopen("ROLLBACK.$$$","rb");CK(!f);
    fault=2;CK(BPSave(&next,"ROLLBACK.BTS",1)==BP_RECOVER);fault=0;
    CK(BPLoad(&got,"ROLLBACK.BTS")==BP_OPEN);
    CK(BPLoad(&got,"ROLLBACK.BAK")==BP_OK&&same(&got,&old));
    CK(BPLoad(&got,"ROLLBACK.$$$")==BP_OK&&same(&got,&next));
    printf("PASS save failure/rollback/recovery checks=%u\n",checks);
    return 0;
}
