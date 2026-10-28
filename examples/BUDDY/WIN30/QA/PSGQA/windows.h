#include <stdint.h>
#include <stddef.h>
#define FAR
#define PASCAL
#define _cdecl
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uintptr_t DWORD;
typedef intptr_t LONG;
typedef unsigned HTASK;
typedef unsigned HMODULE;
typedef void (*FARPROC)(void);
#define WF_PMODE 1
WORD GetWinFlags(void);
int OpenSound(void);
void CloseSound(void);
void StopSound(void);
WORD GetCurrentTask(void);
HMODULE GetModuleHandle(const char *);
FARPROC GetProcAddress(HMODULE,const char *);
char *lstrcpy(char *,const char *);
