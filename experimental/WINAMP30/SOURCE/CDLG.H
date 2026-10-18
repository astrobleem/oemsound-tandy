/* Original implementation ABI declarations, Win16 only.
 * Layout: Microsoft Windows 3.1 SDK Reference Vol.2 (1992), p.413.
 * This library implements a bounded subset for Windows 3.0 real mode. */
#ifndef CDLG_H
#define CDLG_H
#pragma pack(1)
typedef struct tagOPENFILENAME {
 DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance;
 LPCSTR lpstrFilter; LPSTR lpstrCustomFilter; DWORD nMaxCustFilter;
 DWORD nFilterIndex; LPSTR lpstrFile; DWORD nMaxFile;
 LPSTR lpstrFileTitle; DWORD nMaxFileTitle; LPCSTR lpstrInitialDir;
 LPCSTR lpstrTitle; DWORD Flags; UINT nFileOffset; UINT nFileExtension;
 LPCSTR lpstrDefExt; LPARAM lCustData; FARPROC lpfnHook; LPCSTR lpTemplateName;
} OPENFILENAME;
#pragma pack()
typedef OPENFILENAME FAR *LPOPENFILENAME;
#define CDERR_STRUCTSIZE 1L
#define CDERR_INITIALIZATION 2L
#define CDERR_MEMALLOCFAILURE 9L
#define CDERR_MEMLOCKFAILURE 10L
#define FNERR_INVALIDFILENAME 0x3002L
#define FNERR_BUFFERTOOSMALL 0x3003L
#define OFN_READONLY 1L
#define OFN_OVERWRITEPROMPT 2L
#define OFN_HIDEREADONLY 4L
#define OFN_NOCHANGEDIR 8L
#define OFN_EXTENSIONDIFFERENT 0x400L
#define OFN_PATHMUSTEXIST 0x800L
#define OFN_FILEMUSTEXIST 0x1000L
#define OFN_CREATEPROMPT 0x2000L
#define OFN_NOREADONLYRETURN 0x8000L
BOOL FAR PASCAL GetOpenFileName(LPOPENFILENAME);
BOOL FAR PASCAL GetSaveFileName(LPOPENFILENAME);
DWORD FAR PASCAL CommDlgExtendedError(void);
int FAR PASCAL GetFileTitle(LPCSTR,LPSTR,UINT);
#endif
