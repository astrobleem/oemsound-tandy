#define WINVER 0x0300
#include <windows.h>
#include <string.h>
#include "PITAPI.H"
#include "PITLINK.H"
#include "PSGLINK.H"
typedef int (FAR PASCAL *QBEEP)(unsigned);
static HFILE qlog=HFILE_ERROR;
static unsigned failures,checks;
static void check(char *name,int got,int want)
{
    char b[144];
    wsprintf(b,"%s got=%d want=%d\r\n",(LPSTR)name,got,want);
    if(qlog!=HFILE_ERROR) _lwrite(qlog,b,lstrlen(b));
    ++checks;
    if(got!=want) ++failures;
}
static void text(char *key,char *value)
{
    char b[192];
    wsprintf(b,"%s=%s\r\n",(LPSTR)key,(LPSTR)value);
    if(qlog!=HFILE_ERROR) _lwrite(qlog,b,lstrlen(b));
}
static void finishlog(void)
{
    check("TOTAL_FAILURES",failures,0);
    if(qlog!=HFILE_ERROR) _lclose(qlog);
    qlog=HFILE_ERROR;
}
