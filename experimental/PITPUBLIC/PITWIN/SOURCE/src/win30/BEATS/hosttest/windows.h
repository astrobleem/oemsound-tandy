#include <stdint.h>
#include <string.h>
#define FAR
#define PASCAL
#define _loadds
#define _cdecl
#define WF_PMODE 1
#define NULL ((void *)0)
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint16_t HTASK;
WORD GetWinFlags(void);
HTASK GetCurrentTask(void);
int OpenSound(void);
int StopSound(void);
void CloseSound(void);
#define lstrcpy strcpy

typedef WORD HMODULE;
typedef void (*FARPROC)(void);
HMODULE GetModuleHandle(const char *name);
FARPROC GetProcAddress(HMODULE module,const char *name);
