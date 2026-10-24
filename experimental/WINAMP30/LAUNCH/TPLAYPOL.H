#ifndef TPLAYPOL_H
#define TPLAYPOL_H
/* DOS preflight policy shared by the EX/HX launcher and host checks.
   No BIOS offsets are used by TBRIDGE: it saves/chains the entry it finds.
   A RAM/option-ROM IRQ0 owner is still refused before installation. */
static int TPBoard(unsigned marker,unsigned model)
{
    return marker == 0x21U && model == 0xffU;
}
static int TPRomIRQ(unsigned segment,unsigned offset)
{
    unsigned long address;
    address = ((unsigned long)segment << 4) + offset;
    return segment >= 0xf000U && address >= 0xfc000UL &&
           address < 0x100000UL;
}
static int TPBootstrapLength(long bytes)
{
    /* WIN.COM is a DOS COM bootstrap, not an MZ/NE module. Its splash/display
       payload changes size. DOS loads at most 64K minus the 256-byte PSP. */
    return bytes >= 256L && bytes <= 65280L;
}
/* Display names denote reviewed framebuffer contracts, not audio APIs.
   Every Tandy graphics profile below uses the normal-640K 16KB overlap
   guard, even modes whose visible buffer is smaller. Never default an
   unrecognized display to the unreserved stock-CGA path. */
static int TPNameEqual(const char *a,const char *b)
{
    unsigned x,y;
    do {
        x=(unsigned char)*a++;y=(unsigned char)*b++;
        if(x>='a' && x<='z')x-='a'-'A';
        if(y>='a' && y<='z')y-='a'-'A';
        if(x!=y)return 0;
    } while(x);
    return 1;
}
static int TPDisplay(const char *name)
{
    static const char *const guarded[]={
        "HPXY.DRV", "TANDY88.DRV", "TNDY160.DRV", "TNDY3204.DRV",
        "TNDY6402.DRV", "TNDY6404.DRV", "TR51616.DRV", "TR53216.DRV",
        "TR53204.DRV", "TR56402.DRV", "TR56404.DRV"
    };
    unsigned i;
    if(TPNameEqual(name,"CGA.DRV"))return 0;
    for(i=0;i<sizeof(guarded)/sizeof(guarded[0]);++i)
        if(TPNameEqual(name,guarded[i]))return 1;
    return -1;
}
static int TPReserveStatus(int status)
{
    /* MSC6 spawnl returns DOS4D's full word: AH=3 is TSR termination.
       Only AL=0 from the exact helper is success. */
    return status==0 || status==0x300;
}
#endif
