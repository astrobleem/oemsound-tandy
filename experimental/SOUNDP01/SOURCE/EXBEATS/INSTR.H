/* Original compact PSG presets. WORD/DWORD are supplied by windows.h. */
typedef struct {
    DWORD began;
    WORD period;
    BYTE velocity,preset;
} TVOICE;
void InstStart(TVOICE *,unsigned,unsigned,unsigned,DWORD);
int InstRead(TVOICE *,DWORD,WORD,WORD *,WORD *);
