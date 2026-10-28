#ifndef BEATPAT_H
#define BEATPAT_H
#define BP_LANES 5
#define BP_STEPS 8
#define BP_PIT_MIN 26U
#define BP_PIT_MAX 96U
#include "FXCORE.H"
#define BP_FILE_CAP 2048
#define BP_MML_CAP 2048
#define BP_DEMOS 6
#define BP_OK 0
#define BP_FORMAT 1
#define BP_OPEN 2
#define BP_IO 3
#define BP_EXISTS 4
#define BP_RECOVER 5
#define BP_NAME 6
typedef struct {
    unsigned tempo;
    unsigned char pit,program[3],note[BP_LANES][BP_STEPS];
    unsigned char loaded[BP_LANES],trigger[BP_LANES][BP_STEPS];
    char fxName[BP_LANES][13];
    FXPATCH patch[BP_LANES];
} BPAT;
void BPDefault(BPAT *);
const char *BPDemo(BPAT *,unsigned);
const char *BPPreset(unsigned);
int BPValid(const BPAT *);
int BPFits(const FXPATCH *,unsigned);
int BPHasFX(const BPAT *);
unsigned BPEncode(const BPAT *,char *,unsigned);
int BPDecode(BPAT *,const char *,unsigned);
int BPName(const char *,const char *);
int BPLoad(BPAT *,const char *);
int BPSave(const BPAT *,const char *,int);
unsigned BPMML(const BPAT *,char *,unsigned);
#endif
