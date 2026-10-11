/* Plain-DOS session guard; no port writes, timer changes or global lock.
 * Probes match stable Maker DOSPLAY IsWindows: enhanced-mode 1600h,
 * then real/standard-mode 4680h. Not a sound-TSR detector.
 * Caller includes dos.h first. Use before touching shared hardware.
 */
#ifndef TANDY_DOSGUARD_H
#define TANDY_DOSGUARD_H
static int DosWindows(void)
{
    union REGS r;
    r.x.ax=0x1600;int86(0x2f,&r,&r);
    if(r.h.al!=0 && r.h.al!=0x80)return 1;
    r.x.ax=0x4680;int86(0x2f,&r,&r);
    return r.x.ax==0;
}
static int DosSessionPermit(void)
{
    if(DosWindows())return 0;
    return *(unsigned char far *)0xfc000000UL==0x21;
}
#endif
