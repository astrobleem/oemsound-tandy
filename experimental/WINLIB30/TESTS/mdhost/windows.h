#ifndef TEST_WINDOWS_H
#define TEST_WINDOWS_H
#include <stdint.h>
#include <string.h>
#define FAR
#define PASCAL
#define _loadds
typedef uint16_t WORD;typedef uint32_t DWORD;typedef int32_t LONG;typedef uint8_t BYTE;typedef uintptr_t HMODULE;typedef uintptr_t HTASK;typedef char *LPSTR;
HTASK GetCurrentTask(void);HMODULE GetModuleHandle(const char *);WORD GetModuleFileName(HMODULE,char *,WORD);HMODULE LoadLibrary(const char *);WORD FreeLibrary(HMODULE);void *GetProcAddress(HMODULE,LPSTR);
#define lstrcpy strcpy
#endif
