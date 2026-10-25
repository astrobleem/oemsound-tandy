/* Local derivative component: GPLv3, see DRIVER/LICENSE.
   Prepared PIT2 states share the existing SOUND owner. No Windows calls are
   reachable from LeaseFast; code/data are FIXED PRELOAD in TSOUND.DEF. */
#include "PITLINK.H"
static WORD leaseToken,leaseSerial,leaseActive,leaseSuspended,leaseDivisor;
static void leaseStop(void)
{
    WORD bits;
    if (leaseActive && !leaseSuspended) {
        bits = PitRead();
        if ((bits & 3U) == 3U) PitGate(bits & 0xfcU);
    }
    leaseActive = leaseDivisor = 0;
}
static void leaseRevoke(void)
{
    leaseStop();
    leaseToken = leaseSuspended = 0;
}
static int FAR PASCAL _loadds LeaseFast(WORD token,WORD divisor,WORD command)
{
    WORD flags,bits;
    int result;
    flags = SaveIRQ();
    result = 0;
    if (!token || token != leaseToken) result = -1;
    else if (command > TP_REENTER) result = -16;
    else if (command == TP_SUSPEND) {
        leaseStop();
        leaseSuspended = 1;
    } else if (command == TP_MUTE) leaseStop();
    else if (command == TP_REENTER) {
        if (leaseSuspended) {
            if (PitRead() & 3U) result = -3;
            else leaseSuspended = 0;
        }
    } else if (divisor < TP_MIN_DIVISOR) result = -13;
    else if (leaseSuspended) result = -3;
    else {
        bits = PitRead();
        if ((!leaseActive && (bits & 3U)) ||
            (leaseActive && (bits & 3U) != 3U)) {
            leaseActive = 0;
            leaseSuspended = 1;
            result = -3;
        } else if (!leaseActive || leaseDivisor != divisor) {
            PitGate(bits & 0xfcU);
            PitProgram(divisor);
            PitGate(PitRead() | 3U);
            leaseDivisor = divisor;
            leaseActive = 1;
        }
    }
    RestoreIRQ(flags);
    return result;
}
static void leaseInfo(TPINFO FAR *out)
{
    out->size = sizeof(TPINFO);
    out->version = TP_VERSION;
    out->features = TP_FIXED | TP_EXCLUSIVE | TP_SUSPEND_SAFE;
    out->minDivisor = TP_MIN_DIVISOR;
    out->maxDivisor = TP_MAX_DIVISOR;
    out->token = leaseToken;
    out->fast = LeaseFast;
}
int SAPI TandyPitLease(WORD version,TPINFO FAR *out,WORD bytes,WORD operation)
{
    WORD flags,i,pending;
    int result;
    if (version != TP_VERSION || !out || bytes != sizeof(TPINFO) ||
        operation > TP_RELEASE) return -16;
    if (operation != TP_QUERY && !owns()) return -1;
    flags = SaveIRQ();
    result = 0;
    if (operation == TP_CLAIM) {
        pending=0;
        for (i=0;i<3;++i) if (voices[i].count) pending=1;
        if (leaseToken || pitActive || running || timer || beepCallActive ||
            beepActive || pending || timerStarting || timerStopping ||
            (PitRead() & 3U))
            result = -3;
        else {
            if (++leaseSerial == 0) ++leaseSerial;
            leaseToken = leaseSerial;
            leaseSuspended = leaseActive = leaseDivisor = 0;
        }
    } else if (operation == TP_RELEASE) {
        if (!out->token || out->token != leaseToken) result = -1;
        else leaseRevoke();
    }
    leaseInfo(out);
    if (operation == TP_QUERY) out->token = 0;
    RestoreIRQ(flags);
    return result;
}
