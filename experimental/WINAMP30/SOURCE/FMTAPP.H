#ifndef FMTAPP_H
#define FMTAPP_H
void FmtInit(LPSTR);
WORD FmtOpen(LPSTR,WORD);
WORD FmtClose(void);
WORD FmtPump(void);
WORD FmtPause(void);
DWORD FmtPosition(void);
WORD FmtDebug(WORD);
char *FmtReason(void);
#endif
