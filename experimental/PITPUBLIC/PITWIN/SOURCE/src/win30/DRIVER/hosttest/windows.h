#include <stdint.h>
#define FAR
#define PASCAL
#define _loadds
#define _cdecl
#define WF_PMODE 1
typedef uint16_t HTASK;
typedef uint32_t DWORD;
typedef void (*FARPROC)(void);
#define LOWORD(x) ((unsigned)(x)&65535U)
#define HIWORD(x) ((unsigned)((x)>>16)&65535U)
HTASK GetCurrentTask(void);
unsigned GetWinFlags(void);
void Yield(void);
