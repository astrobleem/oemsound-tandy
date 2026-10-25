#ifndef MCITNDY_API
#define MCITNDY_API
#define MC_OPEN 0x803
#define MC_CLOSE 0x804
#define MC_PLAY 0x806
#define MC_STOP 0x808
#define MC_PAUSE 0x809
#define MC_RESUME 0x855
#define MC_PAUSED 529UL
#define MC_SET 0x80d
#define MC_STATUS 0x814
#define MC_NOTIFY 1UL
#define MC_WAIT 2UL
#define MC_ELEMENT 0x200UL
#define MC_TYPE 0x2000UL
#define MC_TYPEID 0x1000UL
#define MC_ITEM 0x100UL
#define MC_TIME 0x400UL
#define MC_LENGTH 1UL
#define MC_POSITION 2UL
#define MC_MODE 4UL
#define MC_FORMAT 6UL
#define MC_READY 7UL
#define MC_STOPPED 525UL
#define MC_PLAYING 526UL
#define MC_BADID 257UL
#define MC_UNSUPPORTED 274UL
#define MC_BADFILE 296UL
#define MC_FLAGS 284UL
#define MC_BUSY 289UL
#define MC_PARAM 290UL
#define MC_NOTIFYMSG 0x3b9
#define MC_SUCCESS 1
#define MC_FAILURE 8
#define MC_ABORTED 4
#define MC_SUPERSEDED 2
/* Win16 MCI parameter layouts; DWORD callback fields contain HWNDs. */
typedef struct {DWORD callback;WORD device;LPCSTR type,element,alias;} MCOPEN;
typedef struct {DWORD callback;} MCGEN;
typedef struct {DWORD callback,from,to;} MCPLAY;
typedef struct {DWORD callback,result,item,track;} MCSTATUS;
typedef struct {DWORD callback,format,audio;} MCSET;
#ifdef MCI_IMPL
#define MCEXPORT FAR PASCAL _loadds
#else
#define MCEXPORT FAR PASCAL
#endif
DWORD MCEXPORT MciCommand(WORD,WORD,DWORD,DWORD);
DWORD MCEXPORT MciString(LPSTR,LPSTR,WORD,HWND);
WORD MCEXPORT MciError(DWORD,LPSTR,WORD);
WORD MCEXPORT MciPump(void);
WORD MCEXPORT MciBank(WORD,WORD);
#endif
