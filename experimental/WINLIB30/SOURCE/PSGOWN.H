#ifndef PSGOWN_H
#define PSGOWN_H
/* Shared SOUND raw-PSG ownership. Foreground task calls only.
   The timer callback never calls Windows or the lease entry point. */
#define PSG_VERSION 1U
#define PSG_QUERY 0U
#define PSG_CLAIM 1U
#define PSG_RELEASE 2U
typedef int (FAR PASCAL *PSGLEASE)(unsigned,unsigned);
static PSGLEASE rawLease;
static unsigned rawClaimed;
static int rawClaim(void) {
    HMODULE h;
    PSGLEASE entry;
    h=GetModuleHandle("SOUND");
    if (!h) return 1;
    entry=(PSGLEASE)GetProcAddress(h,"TANDYPSGLEASE");
    if (!entry || entry(PSG_VERSION,PSG_CLAIM)) return 1;
    rawLease=entry;
    rawClaimed=1;
    return 0;
}
static int rawRelease(void) {
    if (!rawClaimed) return 0;
    if (!rawLease || rawLease(PSG_VERSION,PSG_RELEASE)) return 1;
    rawClaimed=0;
    rawLease=0;
    return 0;
}
#endif
