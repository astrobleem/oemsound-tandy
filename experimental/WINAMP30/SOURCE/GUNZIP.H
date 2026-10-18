#ifndef GUNZIP_H
#define GUNZIP_H
#ifdef _MSC_VER
#define GZ_FAR _far
#else
#define GZ_FAR
#endif
typedef unsigned char GZ8;
typedef unsigned short GZ16;
typedef unsigned long GZ32;
typedef int (*GZREAD)(void *,GZ32,GZ8 *,GZ16);
/* First-party, bounded RFC1951/1952 decoder. No allocation/OS calls.
   One member, CRC/ISIZE verified. Caller owns <=60000 byte FAR buffer.
   Return0 success;1 structure/truncated;2 bounds;3 Huffman;4 CRC/size. */
int gz_decode(GZREAD,void *,GZ32,GZ8 GZ_FAR *,GZ16,GZ16 *);
#endif
