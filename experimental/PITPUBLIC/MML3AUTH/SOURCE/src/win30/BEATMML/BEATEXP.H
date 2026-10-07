#ifndef BEATEXP_H
#define BEATEXP_H
#define BEAT_MML_CAP 2048
unsigned BeatExport(char *out,unsigned cap,unsigned char pattern[4][8],unsigned tempo);
unsigned BeatExportPit(char *out,unsigned cap,
    unsigned char pattern[5][8],unsigned tempo,int enabled);
#endif
