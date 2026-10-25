#ifndef FMTAPP_H
#define FMTAPP_H
#define FMT_INFLATE 1
#define FMT_CHECK 2
#define FMT_CANCELLED 9
typedef WORD (*FMTPROGRESS)(WORD,DWORD,DWORD);
void FmtSetProgress(FMTPROGRESS);
void FmtInit(LPSTR);
WORD FmtOpen(LPSTR,WORD);
WORD FmtClose(void);
WORD FmtPump(void);
WORD FmtPause(void);
DWORD FmtPosition(void);
WORD FmtDebug(WORD);
char *FmtReason(void);
#endif
