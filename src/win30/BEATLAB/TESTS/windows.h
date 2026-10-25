#ifndef BEAT_HOST_WINDOWS_H
#define BEAT_HOST_WINDOWS_H
#include <stdint.h>
#include <string.h>
#define FAR
#define PASCAL
#define _loadds
#define _cdecl
#define WF_PMODE 1
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint16_t HTASK;
typedef unsigned HMODULE;
typedef int (*PITMOCKPROC)(unsigned,unsigned,unsigned);
WORD GetWinFlags(void);
HTASK GetCurrentTask(void);
int OpenSound(void);
int StopSound(void);
void CloseSound(void);
DWORD GetTickCount(void);
HMODULE GetModuleHandle(char *);
PITMOCKPROC GetProcAddress(HMODULE,char *);
#define lstrcpy strcpy
#endif
