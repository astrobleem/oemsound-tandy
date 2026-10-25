/* SOUND v1 raw-PSG owner. Fixed task ownership lives in the driver.
   Discover the installed module only; never load or replace the driver. */
#ifndef TANDY_PSGOWN_H
#define TANDY_PSGOWN_H
#define PSG_LEASE_VERSION 1
#define PSG_LEASE_CLAIM 1
#define PSG_LEASE_RELEASE 2
typedef int (FAR PASCAL *PSGLEASEPROC)(unsigned, unsigned);
typedef int (FAR PASCAL *PSGOPENPROC)(void);
typedef void (FAR PASCAL *PSGCLOSEPROC)(void);
typedef struct {
    PSGLEASEPROC lease;
    int opened;
    PSGCLOSEPROC closeSound;
} PSG_OWNER;
static int PsgOwnerHeld(PSG_OWNER *state)
{
    return state->opened && state->lease && state->closeSound &&
        state->lease(PSG_LEASE_VERSION, PSG_LEASE_CLAIM) == 0;
}
static int PsgOwnerAcquire(PSG_OWNER *state)
{
    HMODULE driver;
    PSGLEASEPROC lease;
    PSGOPENPROC openSound;
    PSGCLOSEPROC closeSound;
    if (state->opened) return PsgOwnerHeld(state);
    state->lease = NULL;
    state->closeSound = NULL;
    driver = GetModuleHandle("SOUND");
    if (!driver) return 0;
    lease = (PSGLEASEPROC)GetProcAddress(driver, "TANDYPSGLEASE");
    openSound = (PSGOPENPROC)GetProcAddress(driver, "OPENSOUND");
    closeSound = (PSGCLOSEPROC)GetProcAddress(driver, "CLOSESOUND");
    if (!lease || !openSound || !closeSound) return 0;
    if (openSound() < 1) return 0;
    if (lease(PSG_LEASE_VERSION, PSG_LEASE_CLAIM) != 0) {
        closeSound();
        return 0;
    }
    state->lease = lease;
    state->closeSound = closeSound;
    state->opened = 1;
    return 1;
}
static void PsgOwnerRelease(PSG_OWNER *state)
{
    /* An own repeated CLAIM validates without I/O. It also permits cleanup
       after this task's StopSound revoked the raw claim. A foreign task or
       a managed queue is never closed through a stale local state. */
    if (PsgOwnerHeld(state) &&
        state->lease(PSG_LEASE_VERSION, PSG_LEASE_RELEASE) == 0)
        state->closeSound();
    state->lease = NULL;
    state->closeSound = NULL;
    state->opened = 0;
}
#endif
