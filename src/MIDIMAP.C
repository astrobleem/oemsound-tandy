#include <windows.h>

#define MODM_GETNUMDEVS 1
#define MODM_GETDEVCAPS 2
#define MODM_OPEN 3
#define MODM_CLOSE 4
#define MODM_DATA 7
#define MODM_LONGDATA 8
#define MODM_RESET 9

#define MMSYSERR_NOERROR 0
#define MMSYSERR_NOTSUPPORTED 4

#define MOD_MAPPER 5
#define MM_MICROSOFT 1
#define MM_MIDI_MAPPER 1

static void FAR copystr(char FAR *dst, const char FAR *src)
{
    while (*src) *dst++ = *src++;
    *dst = '\0';
}

typedef struct {
    WORD wMid;
    WORD wPid;
    UINT vDriverVersion;
    char szPname[32];
    WORD wTechnology;
    WORD wVoices;
    WORD wNotes;
    WORD wChannelMask;
    DWORD dwSupport;
} MIDIOUTCAPS;
typedef MIDIOUTCAPS FAR* LPMIDIOUTCAPS;

int FAR PASCAL LibMain(HINSTANCE hInst, WORD wDS, WORD cbHeap, LPSTR lpszCmdLine)
{
    return 1;
}

int FAR PASCAL WEP(int nParameter)
{
    return 1;
}

LONG FAR PASCAL _loadds DriverProc(DWORD dwID, HDRVR hDriver, WORD msg, LONG lParam1, LONG lParam2)
{
    switch (msg) {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_FREE:
        return 1;
    case DRV_OPEN:
    case DRV_CLOSE:
        return 1;
    default:
        return 0L;
    }
}

DWORD FAR PASCAL _loadds midiMessage(UINT id, WORD msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    switch (msg) {
    case MODM_GETNUMDEVS:
        return 1;
    case MODM_OPEN:
    case MODM_CLOSE:
    case MODM_DATA:
    case MODM_LONGDATA:
    case MODM_RESET:
        return MMSYSERR_NOERROR;
    case MODM_GETDEVCAPS:
        {
            LPMIDIOUTCAPS caps = (LPMIDIOUTCAPS)dwParam1;
            caps->wMid = MM_MICROSOFT;
            caps->wPid = MM_MIDI_MAPPER;
            caps->vDriverVersion = 0x0100;
            caps->wTechnology = MOD_MAPPER;
            caps->wVoices = 1;
            caps->wNotes = 1;
            caps->wChannelMask = 0xFFFF;
            caps->dwSupport = 0;
            copystr(caps->szPname, "Tandy MIDI Mapper");
        }
        return MMSYSERR_NOERROR;
    default:
        return MMSYSERR_NOTSUPPORTED;
    }
}
