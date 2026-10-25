/* Portable regressions for the actual native installer parser. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SFXCORE.H"
#define equal SXEqual
static unsigned tests;
static void check(int yes,const char *name) {
    ++tests;
    if(!yes) { printf("FAILED: %s\n",name); exit(1); }
}
static void put32(unsigned char *p,unsigned long value) {
    unsigned i;
    for(i=0;i<4;i++) { p[i]=(unsigned char)value;value>>=8; }
}
#include "SFXTEST.H"
int main(void) {
    sfxchecks();
    printf("PASS %u native installer parser checks\n",tests);
    return 0;
}
