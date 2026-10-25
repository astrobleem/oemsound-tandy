#ifndef RAWAPP_H
#define RAWAPP_H
#include "RAWFILE.H"
void RawInit(LPSTR);
WORD RawOpen(LPSTR,WORD);
WORD RawClose(void);
WORD RawPump(void);
DWORD RawPosition(void);
WORD RawDebug(WORD);
#endif
