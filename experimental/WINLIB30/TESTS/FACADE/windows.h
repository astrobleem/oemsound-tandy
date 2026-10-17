#ifndef AUDIO_FACADE_TEST_WINDOWS_H
#define AUDIO_FACADE_TEST_WINDOWS_H
#include <stdint.h>
/* Layout-only Win16 ABI fixture; pointer tokens model 32-bit far pointers. */
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t HWND;
typedef uint32_t LPCSTR;
typedef uint32_t LPSTR;
#define FAR
#define PASCAL
#define _loadds
#endif
