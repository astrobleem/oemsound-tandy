/* Independent DOS-side shutdown probe. Never writes a device port. */
#include <dos.h>
#include <conio.h>
#include <stdio.h>
int main(int argc,char **argv)
{
    unsigned long vectors[3],ticks;
    unsigned gate,mode;
    union REGS regs;
    FILE *f;
    if(argc!=2)return 1;
    vectors[0]=(unsigned long)_dos_getvect(8);
    vectors[1]=(unsigned long)_dos_getvect(0x23);
    vectors[2]=(unsigned long)_dos_getvect(0x24);
    gate=inp(0x61)&3;
    regs.h.ah=0x0f;int86(0x10,&regs,&regs);mode=regs.h.al&0x7f;
    ticks=*(volatile unsigned long __far *)MK_FP(0x40,0x6c);
    f=fopen(argv[1],"wb");if(!f)return 2;
    fwrite(vectors,4,3,f);fwrite(&gate,2,1,f);fwrite(&mode,2,1,f);
    fwrite(&ticks,4,1,f);fclose(f);
    return 0;
}
