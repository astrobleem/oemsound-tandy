/* Host-only filesystem harness. Never part of a DOS consumer package. */
#ifndef DOSHOST_H
#define DOSHOST_H
#include <stdint.h>
#define far
#define strnicmp _strnicmp
#define P_WAIT _P_WAIT
#define spawnl _spawnl
/* Map only the literal guest WINXT namespace into a newly-owned host test
   tree, so actual journal paths are exercised without touching host C:\. */
static char host_xtroot[260];
static const char *HostPath(const char *p,char *out) {
    if(*host_xtroot&&!_strnicmp(p,"C:\\WINXT",8)&&
       (!p[8]||p[8]=='\\')) {
        sprintf(out,"%s%s",host_xtroot,p+8); return out;
    }
    return p;
}
static FILE *HostOpen(const char *p,const char *mode) {
    char out[260]; return fopen(HostPath(p,out),mode);
}
static int HostAccess(const char *p,int mode) {
    char out[260]; return _access(HostPath(p,out),mode);
}
static int HostRemove(const char *p) {
    char out[260]; return remove(HostPath(p,out));
}
static int HostMkdir(const char *p) {
    char out[260]; return _mkdir(HostPath(p,out));
}
static int HostRmdir(const char *p) {
    char out[260]; return _rmdir(HostPath(p,out));
}
#define fopen HostOpen
#define access HostAccess
#define remove HostRemove
#define mkdir HostMkdir
#define rmdir HostRmdir
union REGS {
    struct { unsigned ax,bx,cx,dx,si,di,cflag; } x;
    struct { unsigned char al,ah,bl,bh,cl,ch,dl,dh; } h;
};
static void int86(unsigned interrupt,union REGS *in,union REGS *out) {
    (void)interrupt;
    out->x.ax=in->x.ax; out->x.cflag=1;
}
struct find_t {
    unsigned attrib;
    char name[260];
    intptr_t handle;
    struct _finddata_t data;
};
static unsigned _dos_findfirst(const char *p,unsigned attrs,
                               struct find_t *f) {
    char out[260];
    (void)attrs;
    f->handle=_findfirst(HostPath(p,out),&f->data);
    if(f->handle==-1) return 1;
    f->attrib=f->data.attrib; strcpy(f->name,f->data.name); return 0;
}
static unsigned _dos_findnext(struct find_t *f) {
    if(_findnext(f->handle,&f->data)) { _findclose(f->handle); return 1; }
    f->attrib=f->data.attrib; strcpy(f->name,f->data.name); return 0;
}
#endif
