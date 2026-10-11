/* Optional PIT2 core. Caller serializes I/O and verifies its lease. */
extern unsigned _cdecl PitRead(void);
extern void _cdecl PitGate(unsigned);
extern void _cdecl PitProgram(unsigned);
static unsigned pitActive;
static void pitStop(void)
{
    if (pitActive) { PitGate(PitRead() & 0xfc); pitActive=0; }
}
static int pitSet(unsigned version,unsigned hz,unsigned level)
{
    unsigned bits,divisor;
    unsigned long count;
    if (version!=1 || level>1) return -16;
    if (!level || !hz) { pitStop(); return 0; }
    if (hz<37 || hz>7902) return -13;
    count=(1193182UL+hz/2)/hz;
    if (count<2 || count>65535UL) return -13;
    bits=PitRead();
    if (!pitActive && (bits & 3)) return -3;
    divisor=(unsigned)count;
    /* Gate off during programming; retain current EX routing/mute bits. */
    PitGate(bits & 0xfc);
    PitProgram(divisor);
    PitGate(PitRead() | 3);
    pitActive=1;
    return 0;
}
