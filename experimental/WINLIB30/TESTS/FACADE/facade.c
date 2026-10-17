#include "windows.h"
#include "LWAUDIO.H"
typedef void (*RAW_INIT_SIG)(LPSTR);
typedef WORD (*RAW_OPEN_SIG)(LPSTR,WORD);
typedef WORD (*RAW_CLOSE_SIG)(void);
typedef WORD (*RAW_PUMP_SIG)(void);
typedef DWORD (*RAW_POSITION_SIG)(void);
typedef WORD (*RAW_DEBUG_SIG)(WORD);
typedef DWORD (FAR PASCAL *MCI_COMMAND_SIG)(WORD,WORD,DWORD,DWORD);
static RAW_INIT_SIG rawInit=RawInit;
static RAW_OPEN_SIG rawOpen=RawOpen;
static RAW_CLOSE_SIG rawClose=RawClose;
static RAW_PUMP_SIG rawPump=RawPump;
static RAW_POSITION_SIG rawPosition=RawPosition;
static RAW_DEBUG_SIG rawDebug=RawDebug;
static MCI_COMMAND_SIG mciCommand=MciCommand;
int main(void) {
 (void)rawInit;(void)rawOpen;(void)rawClose;(void)rawPump;
 (void)rawPosition;(void)rawDebug;(void)mciCommand;return 0;
}
