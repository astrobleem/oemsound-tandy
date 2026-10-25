/* Actual MCIPSG -> MIDICLK -> bank renderer -> mocked raw hardware. */
#define GetProcAddress RawAddress
#define GetModuleFileName RawFilename
#define GetModuleHandle RawModule
#define LoadLibrary RawLibrary
#define FreeLibrary RawFree
#define main inherited_md_main
#include "mdhost/driver.c"
#undef main
#undef GetProcAddress
#undef GetModuleFileName
#undef GetModuleHandle
#undef LoadLibrary
#undef FreeLibrary
#include "../SOURCE/MCIPSG.H"
#include "../SOURCE/SMF.H"
static FILE *song;static WORD wantPair;
static WORD pairPut(TCXPAIR FAR *p){return putraw(&p->frame);}
static WORD pairOpen(WORD FAR *h){WORD r=MDOpen(h);if(!r&&wantPair){paired=1;rpairput=pairPut;init(0);}return r;}
static unsigned readSong(SMF_ULONG off,unsigned char *dst,unsigned n)
{if(fseek(song,(long)off,SEEK_SET))return 0;return fread(dst,1,n,song);}
HMODULE GetModuleHandle(const char *s){(void)s;return 36;}
WORD GetModuleFileName(HMODULE m,char *p,WORD n){(void)m;(void)n;strcpy(p,"C:\\WINAMP\\MCITNDY.DLL");return strlen(p);}
HMODULE LoadLibrary(const char *n){(void)n;return 36;}
WORD FreeLibrary(HMODULE m){(void)m;return 0;}
void *GetProcAddress(HMODULE m,LPSTR p){(void)m;switch((uintptr_t)p){
 case 1:return pairOpen;case 2:return MDPush;case 3:return MDCommit;
 case 4:return MDStart;case 5:return MDPump;case 6:return MDPause;
 case 7:return MDResume;case 9:return MDClose;case 10:return MDStatus;
 case 13:return MDTiming;case 19:return MDInstrumentBank;}return 0;}
int main(int argc,char **argv){WORD bank,pair;long size;unsigned i;
 ck(argc==2,"actual retained song argument");song=fopen(argv[1],"rb");ck(song!=0,"retained song open");
 fseek(song,0,SEEK_END);size=ftell(song);
 for(pair=0;pair<2;pair++)for(bank=0;bank<2;bank++){
  wantPair=pair;
  ck(smf_open_stream(readSong,(SMF_ULONG)size)==1,"actual SMF accepted");
  ck(!SCBank(bank),"actual file bank selected");
  i=SCStart();if(i)fprintf(stderr,"bank=%u state=%u error=%u rawframes=%u\n",bank,info.state,info.error,nframes);
  ck(!i,"actual file renderer starts");ck(synth.bank==bank&&folded.bank==bank,"both bank states applied");
  ck(nframes>0,"actual frames queued");ck(!SCStop()&&!lease,"actual file cleanup");smf_close();
 }
 fclose(song);printf("PASS actual complete bank file path checks=%u\n",tests);return 0;
}
