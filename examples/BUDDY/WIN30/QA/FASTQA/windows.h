#include <stdint.h>
#include <stddef.h>
#define FAR
#define PASCAL
#define _cdecl
#define __export
#define __loadds
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef unsigned HMODULE;
typedef void (*FARPROC)(void);
#define WF_PMODE 1
WORD GetWinFlags(void);
int OpenSound(void);
void CloseSound(void);
WORD GetCurrentTask(void);
HMODULE GetModuleHandle(const char *);
WORD GetModuleFileName(HMODULE,char *,WORD);
HMODULE LoadLibrary(const char *);
void FreeLibrary(HMODULE);

void Yield(void);
