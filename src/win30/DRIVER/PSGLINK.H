#ifndef PSGLINK_H
#define PSGLINK_H
/* Include windows.h first. OpenSound must succeed before CLAIM/RELEASE.
   Claim after StopSound. A repeated own CLAIM validates without I/O.
   StopSound, CloseSound and WEP revoke. Busy and nonowner calls do no I/O. */
#define TSG_VERSION 1
#define TSG_QUERY 0
#define TSG_CLAIM 1
#define TSG_RELEASE 2
typedef int (FAR PASCAL *TSGGET)(unsigned,unsigned);
#endif
