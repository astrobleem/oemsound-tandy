/* SOUND v1 raw-PSG owner. Fixed task ownership lives in the driver.
   The caller must release before unloading; there is no unguarded fallback. */
#ifndef TANDY_PSGOWN_H
#define TANDY_PSGOWN_H
#define PSG_LEASE_VERSION 1
#define PSG_LEASE_CLAIM 1
#define PSG_LEASE_RELEASE 2
typedef int (FAR PASCAL *PSGLEASEPROC)(unsigned, unsigned);
typedef struct { PSGLEASEPROC lease; int opened; } PSG_OWNER;
static int PsgOwnerHeld(PSG_OWNER *state)
{
    return state->opened && state->lease &&
        state->lease(PSG_LEASE_VERSION, PSG_LEASE_CLAIM) == 0;
}
static int PsgOwnerAcquire(PSG_OWNER *state)
{
    HMODULE driver;
    PSGLEASEPROC lease;
    if (state->opened) return PsgOwnerHeld(state);
    driver = GetModuleHandle("SOUND");
    if (!driver) return 0;
    lease = (PSGLEASEPROC)GetProcAddress(driver, "TANDYPSGLEASE");
    if (!lease || OpenSound() < 1) return 0;
    if (lease(PSG_LEASE_VERSION, PSG_LEASE_CLAIM) != 0) {
        CloseSound();
        return 0;
    }
    state->lease = lease;
    state->opened = 1;
    return 1;
}
static void PsgOwnerRelease(PSG_OWNER *state)
{
    if (state->opened) {
        /* RELEASE verifies GetCurrentTask before muting raw output.
           CloseSound likewise ignores a caller that no longer owns it. */
        state->lease(PSG_LEASE_VERSION, PSG_LEASE_RELEASE);
        CloseSound();
    }
    state->lease = NULL;
    state->opened = 0;
}
#endif
