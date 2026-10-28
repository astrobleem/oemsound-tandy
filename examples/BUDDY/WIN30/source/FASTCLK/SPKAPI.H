#define SP_CAPACITY 1024
typedef struct {DWORD at;WORD divisor;} SPSTATE;
typedef WORD (FAR PASCAL *SPOPEN)(void);
typedef WORD (FAR PASCAL *SPLOAD)(SPSTATE FAR *,WORD,DWORD);
typedef WORD (FAR PASCAL *SPONE)(void);
typedef DWORD (FAR PASCAL *SPCLOCK)(void);
typedef WORD (FAR PASCAL *SPDEBUG)(WORD);
