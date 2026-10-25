/* DOS launcher, 8086 / MSC6 small model. No installation-file writes.
   CRC32 is a bounded accidental-change identity check, not an authenticity
   mechanism against deliberately engineered CRC collisions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <direct.h>
#include <process.h>
#include "TPLAYPOL.H"
#include "SOUNDID.H"
#define PLEN 128
#define FP(s,o) ((void far *)(((unsigned long)(s)<<16)|(unsigned)(o)))
extern unsigned _cdecl DOSStat(unsigned);
static char home[PLEN],win[PLEN],path[PLEN],bridge[PLEN],reserve[PLEN];
static char player[PLEN],wincom[PLEN],oldcwd[PLEN];
static unsigned olddrive;
static int refuse(char *why){printf("TPLAY REFUSED: %s\n",why);return 2;}
static int join(char *out,char *root,char *tail){
 if(strlen(root)+strlen(tail)+2>=PLEN)return 0;
 strcpy(out,root);strcat(out,"\\");strcat(out,tail);return 1;
}
static int absolute(char *s){
 unsigned i,n=0,dot=0;
 if(!isalpha((unsigned char)s[0])||s[1]!=':'||s[2]!='\\'||!s[3])return 0;
 for(i=3;s[i];i++){
  unsigned c=(unsigned char)s[i];
  if(c=='\\'){if(!n)return 0;n=dot=0;continue;}
  if(c=='.'){if(!n||dot)return 0;dot=1;n=0;continue;}
  if(!isalnum(c)&&c!='_'&&c!='-'&&c!='$'&&c!='~')return 0;
  if(++n>(dot?3U:8U))return 0;
 }
 return n!=0;
}
static int exists(char *p){FILE *f=fopen(p,"rb");if(!f)return 0;fclose(f);return 1;}
/* At most the pinned length plus one byte is read, including EOF validation. */
static int identity(char *p,unsigned size,unsigned long wanted){
 FILE *f;unsigned n,bit;int c;unsigned long crc=0xffffffffUL;
 f=fopen(p,"rb");if(!f)return 0;
 for(n=0;n<size;n++){
  c=fgetc(f);if(c==EOF){fclose(f);return 0;}crc^=(unsigned)c;
  for(bit=0;bit<8;bit++)crc=(crc>>1)^((crc&1)?0xedb88320UL:0UL);
 }
 c=fgetc(f);n=!ferror(f)&&c==EOF&&((crc^0xffffffffUL)==wanted);
 fclose(f);return n;
}
/* No bootstrap offsets or splash bytes are used by the audio path. The
   paired SYSTEM.DRV/SOUND and runtime hook instruction checks remain exact.
   spawnl /R and normal owned-bridge cleanup handle a launch failure. */
static int bootstrap(char *p){FILE *f;long n;int okay;
 f=fopen(p,"rb");if(!f)return 0;
 okay=fseek(f,0L,SEEK_END)==0;n=okay?ftell(f):-1L;
 okay=okay&&TPBootstrapLength(n)&&!ferror(f);fclose(f);return okay;
}
static char *trim(char *s){char *e;while(*s==' '||*s=='\t')s++;
 e=s+strlen(s);while(e>s&&(e[-1]==' '||e[-1]=='\t'||e[-1]=='\r'||e[-1]=='\n'))*--e=0;return s;}
static int selections(void){
 FILE *f;char line[256],*p,*q,*value;unsigned total=0,boot=0,boots=0,system=0,sound=0,display=0;int video=0;
 if(!join(path,win,"SYSTEM.INI")||!(f=fopen(path,"r")))return -1;
 while(fgets(line,sizeof(line),f)){
  total+=strlen(line);if(total>8192U||(!strchr(line,'\n')&&!feof(f))){fclose(f);return -1;}
  if((q=strchr(line,';'))!=0)*q=0;p=trim(line);if(!*p)continue;
  if(*p=='['){q=strchr(p,']');if(!q||*trim(q+1)){fclose(f);return -1;}*q=0;
   boot=!stricmp(trim(p+1),"boot");if(boot&&++boots!=1){fclose(f);return -1;}continue;}
  if(!boot)continue;q=strchr(p,'=');if(!q){fclose(f);return -1;}*q=0;value=trim(q+1);p=trim(p);
  if(!stricmp(p,"system.drv")){if(++system!=1||stricmp(value,"SYSTEM.DRV")){fclose(f);return -1;}}
  if(!stricmp(p,"sound.drv")){if(++sound!=1||stricmp(value,"SOUND.DRV")){fclose(f);return -1;}}
  if(!stricmp(p,"display.drv")){
   if(++display!=1){fclose(f);return -1;}
   video=TPDisplay(value);
   if(video<0){fclose(f);return -1;}
   printf("TPLAY: display=%s; video reservation=%s.\n",value,
          video?"16KB normal-640K":"none (stock CGA)");
  }
 }
 if(ferror(f)||boots!=1||system!=1||sound!=1||display!=1){fclose(f);return -1;}
 fclose(f);return video;
}
static int broker(void){union REGS r;struct SREGS s;
 segread(&s);r.x.ax=0xd7a0;r.x.bx=0x5451;int86x(0x2f,&r,&r,&s);return r.x.ax==0xd7af?(r.x.cx==2?2:1):0;
}
static int inWindows(void){union REGS r;
 r.x.ax=0x1600;int86(0x2f,&r,&r);if(r.h.al!=0&&r.h.al!=0x80)return 1;
 r.x.ax=0x4680;int86(0x2f,&r,&r);return r.x.ax==0;
}
static int defaultIRQ(void){unsigned far *v=FP(0,0x20);
 /* The broker captures/chains the actual BIOS entry; no FEAh offset is
    used. Keep RAM TSRs and option ROMs out of the owned system-ROM path. */
 printf("TPLAY: initial IRQ0=%04x:%04x.\n",v[1],v[0]);
 return TPRomIRQ(v[1],v[0]);
}
int main(int argc,char **argv){
 char *q;int video,check=0,test=0,status,cleanup;union REGS r;unsigned parent,root;
 unsigned marker,model;
 if(argc<2||argc>3)return refuse("Use TPLAY C:\\WINDOWS [/CHECK or /TEST], outside Windows.");
 if(argc==3){check=!stricmp(argv[2],"/CHECK");test=!stricmp(argv[2],"/TEST");if(!check&&!test)return refuse("Unknown option.");}
 r.h.ah=0x30;int86(0x21,&r,&r);
 if(r.h.al<3)return refuse("Requires DOS 3.0 or newer.");
 if(strlen(argv[1])>=PLEN)return refuse("Windows directory too long.");
 strcpy(win,argv[1]);if(!absolute(win))return refuse("Use an absolute DOS 8.3 Windows directory, without a trailing slash.");
 if(!_fullpath(home,argv[0],sizeof(home)))return refuse("Cannot resolve launcher directory.");
 q=strrchr(home,'\\');if(!q)return refuse("Cannot resolve launcher directory.");*q=0;
 if(!absolute(home))return refuse("Keep this package in its own DOS 8.3 directory.");
 if(test&&stricmp(home,"C:\\MMLAB"))return refuse("/TEST is only for the disposable C:\\MMLAB cloud fixture.");
 if(broker())return refuse("A timer bridge already exists; leave its owner in control.");
 if(inWindows())return refuse("Exit Windows completely before using TPLAY.");
 marker=*(unsigned char far *)FP(0xfc00,0);
 model=*(unsigned char far *)FP(0xffff,0x0e);
 printf("TPLAY: FC00:0000=%02x FFFF:000E=%02x.\n",marker,model);
 if(!TPBoard(marker,model))return refuse("Requires the supported Tandy EX/HX ROM hardware markers.");
 if(!defaultIRQ())return refuse("IRQ0 has an unrecognized owner.");
 video=selections();if(video<0)return refuse("SYSTEM.INI must select generic SYSTEM, canonical SOUND and a qualified display without duplicates.");
 if(!join(path,win,"SYSTEM.DRV")||exists(path))return refuse("Ambiguous root SYSTEM.DRV.");
 if(!join(path,win,"SYSTEM\\SYSTEM.DRV")||!identity(path,2784U,0x3ff05fecUL))return refuse("SYSTEM.DRV identity mismatch or missing file.");
 if(!join(path,win,"SOUND.DRV")||exists(path))return refuse("Ambiguous root SOUND.DRV.");
 if(!join(path,win,"SYSTEM\\SOUND.DRV")||!identity(path,SOUND_BYTES,SOUND_CRC))return refuse("Paired SOUND.DRV identity mismatch or missing file.");
 if(!join(wincom,win,"WIN.COM")||!bootstrap(wincom))return refuse("WIN.COM missing or outside the DOS COM bootstrap size.");
 if(!join(bridge,home,"TBRIDGE.EXE")||!identity(bridge,2922U,0x79c22c55UL))return refuse("Keep the matching TBRIDGE.EXE with TPLAY.");
 if(!join(player,home,test?"RAWGUI.EXE":"MINIPLAY.EXE")||!exists(player))return refuse("Player or test client is missing.");
 if(!join(path,home,"RAWCLK.DLL")||!exists(path))return refuse("Keep RAWCLK.DLL with TPLAY.");
 if(video&&(!join(reserve,home,"RESERVE.COM")||!identity(reserve,1512U,0xc794fceaUL)))return refuse("Keep the qualified RESERVE.COM with TPLAY for Tandy graphics.");
 printf("TPLAY preflight PASS: bounded driver checks complete; no resident or sound acquisition yet.\n");fflush(stdout);
 if(check)return 0;
 if(!getcwd(oldcwd,sizeof(oldcwd)))return refuse("Cannot preserve current DOS directory.");olddrive=_getdrive();
 if(video){status=spawnl(P_WAIT,reserve,reserve,(char *)0);
  /* MSC6 returns the complete DOS4D word: AH3 is normal TSR termination. */
  if(!TPReserveStatus(status))return refuse("Video reservation refused; Windows was not started.");
 }
 /* Never adopt an existing bridge, including one appearing after preflight. */
 if(broker())return refuse("A bridge appeared after preflight; Windows was not started.");
 status=spawnl(P_WAIT,bridge,bridge,(char *)0);
 printf("TPLAY: bridge install return=%d.\n",status);
 if(broker()!=2)return refuse("Bridge installation failed; Windows was not started.");
 /* A child TBRIDGE records this launcher's PSP as its sole uninstall parent. */
 r.h.ah=0x62;int86(0x21,&r,&r);parent=r.x.bx;root=0;
 /* DOSStat ABI helper only reads broker diagnostics. */
 root=DOSStat(15);
 if(root!=parent){printf("TPLAY: bridge owner mismatch; retaining foreign bridge.\n");return 3;}
 printf("TPLAY: owned bridge installed; starting Windows.\n");fflush(stdout);
 status=-1;
 if(_chdrive(toupper((unsigned char)win[0])-'A'+1)==0&&chdir(win)==0)
  status=spawnl(P_WAIT,wincom,wincom,"/R",player,(char *)0);
 _chdrive(olddrive);chdir(oldcwd);
 cleanup=spawnl(P_WAIT,bridge,bridge,"/U",(char *)0);
 if(cleanup!=0||broker()){printf("TPLAY: cleanup refused; bridge retained. Do not force removal.\n");return 3;}
 printf("TPLAY: owned bridge removed; Windows return=%d.\n",status);
 return status==0?0:4;
}
