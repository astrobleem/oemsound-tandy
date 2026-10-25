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
   Return0 success;1 structure/truncated;2 bounds;3 Huffman;4 CRC/size;
   5 output sink failed. */
int gz_decode(GZREAD,void *,GZ32,GZ8 GZ_FAR *,GZ16,GZ16 *);
/* Same decoder with bounded memory: the caller supplies only a GZ_WINDOW-byte
   history ring (exactly 32768 bytes, FAR). Output goes to the sink in
   GZ_CHUNK-byte pieces (plus one final shorter piece); the sink returns
   nonzero on success. limit caps the total output (return 2 beyond it).
   The CRC32/ISIZE trailer is verified before success; on any failure the
   sink may already have received a prefix, so discard what it wrote. */
#define GZ_WINDOW 32768U
#define GZ_CHUNK 4096U
typedef int (*GZWRITE)(void *,const GZ8 GZ_FAR *,GZ16);
int gz_decode_stream(GZREAD,void *,GZ32,GZ8 GZ_FAR *,GZWRITE,void *,GZ32,GZ32 *);
#endif
