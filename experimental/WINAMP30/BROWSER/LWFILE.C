/* COMMDLG30: original, bounded 8086/Win16 implementation. */
#define WINVER 0x0300
#include <windows.h>
#include "CDLG.H"
#define MAXPATH 128
#define NFILTER 16
#define SUPPORTED 0xBC0FL
#define IDC_FILE 100
#define IDC_LIST 101
#define IDC_TYPE 102
#define IDC_DIR 103
#define IDC_RO 104
#define IDC_LABEL 105
#define NSLOT 32
extern UINT FAR PASCAL DOSDRIVE(void);
extern void FAR PASCAL DOSCHDRV(UINT);
extern BOOL FAR PASCAL DOSGETDIR(LPSTR,UINT);
extern BOOL FAR PASCAL DOSSETDIR(LPCSTR);
extern int FAR PASCAL DOSATTR(LPCSTR);
extern HFILE FAR PASCAL DOSNEW(LPCSTR);
typedef struct {
 HTASK task; DWORD error; HGLOBAL active,ask;
} TASKSTATE;
typedef struct {
 LPOPENFILENAME out; OPENFILENAME in; int save,slot;
 char file[MAXPATH],path[MAXPATH],work[MAXPATH],full[MAXPATH],pending[MAXPATH];
 char title[64],defext[4]; char filter[NFILTER][64],label[NFILTER][40];
 int filters,index; UINT originalDrive;
 BYTE captured[26]; char cwd[26][68];
 HGLOBAL self; HFILE pendingHandle; OFSTRUCT of;
} CONTEXT;
typedef CONTEXT FAR *CTX;
static TASKSTATE states[NSLOT];
static HINSTANCE module;
static char prop[]="CD30CTX";
static int state(void) {
 HTASK t=GetCurrentTask(); int i,free=-1;
 for(i=0;i<NSLOT;i++) {if(states[i].task==t)return i;if(!states[i].task&&free<0)free=i;}
 if(free>=0){states[free].task=t;states[free].error=0;states[free].active=0;}
 return free;
}
static BOOL span(LPCSTR p,UINT n) {return p&&((DWORD)LOWORD((DWORD)p)+(DWORD)n<=65536L);}
static UINT length(LPCSTR s,UINT limit) {
 UINT n;if(!s)return limit;
 for(n=0;n<limit;n++){if(!span(s,n+1))return limit;if(!s[n])return n;}return limit;
}
static BOOL copy(LPSTR d,LPCSTR s,UINT cap) {
 UINT n=length(s,cap),i;if(n==cap)return FALSE;
 for(i=0;i<=n;i++)d[i]=s[i];return TRUE;
}
static char upper(char c){return (char)(c>='a'&&c<='z'?c-'a'+'A':c);}
static BOOL badchar(char c) {return ((BYTE)c<33||(BYTE)c>=127)||c=='"'||c=='<'||c=='>'||c=='|'||c=='['||c==']'||c=='+'||c=='='||c==','||c==';';}
static BOOL device(LPCSTR s,UINT n) {
 char a=upper(s[0]),b=(char)(n>1?upper(s[1]):0),c=(char)(n>2?upper(s[2]):0);
 if(n==3&&((a=='C'&&b=='O'&&c=='N')||(a=='A'&&b=='U'&&c=='X')||(a=='N'&&b=='U'&&c=='L')||(a=='P'&&b=='R'&&c=='N')))return TRUE;
 if(n==4&&s[3]>='1'&&s[3]<='9'&&((a=='C'&&b=='O'&&c=='M')||(a=='L'&&b=='P'&&c=='T')))return TRUE;
 return n==6&&a=='C'&&b=='L'&&c=='O'&&upper(s[3])=='C'&&upper(s[4])=='K'&&s[5]=='$';
}
/* Validate the bounded DOS 8.3 path, allowing . and .. directory components. */
static BOOL pathok(LPCSTR s,BOOL wild) {
 UINT n=length(s,MAXPATH),i,start=0,j,base,ext;int dot;char c;
 if(!n||n==MAXPATH)return FALSE;
 if((s[0]=='\\'||s[0]=='/')&&(s[1]=='\\'||s[1]=='/'))return FALSE;
 if(n>1&&s[1]==':'){c=upper(s[0]);if(c<'A'||c>'Z')return FALSE;start=2;}
 if(start==n)return FALSE;
 for(i=start;i<=n;i++)if(s[i]=='\\'||s[i]=='/'||!s[i]) {
  if(i==start){if(i==n)return FALSE;start=i+1;continue;}
  if((i-start==1&&s[start]=='.')||(i-start==2&&s[start]=='.'&&s[start+1]=='.')) {if(i==n)return FALSE;}
  else {
   base=ext=0;dot=0;
   for(j=start;j<i;j++){c=s[j];if(c=='.'){if(dot)return FALSE;dot=1;}
    else {if(badchar(c)||c==':'||((c=='*'||c=='?')&&!wild))return FALSE;if(dot)ext++;else base++;}}
   if(!base||base>8||ext>3||device(s+start,base))return FALSE;
  }
  start=i+1;
 }
 return TRUE;
}
static int titleof(LPCSTR s,LPSTR out,UINT cap) {
 UINT n=length(s,MAXPATH),i,start=0,need;
 if(!pathok(s,FALSE))return -1;
 for(i=0;i<n;i++)if(s[i]=='\\'||s[i]=='/'||s[i]==':')start=i+1;
 need=n-start+1;if(cap<need)return need;
 if(!span(out,need))return -1;
 for(i=0;i<need;i++)out[i]=s[start+i];return 0;
}
int FAR PASCAL GetFileTitle(LPCSTR s,LPSTR out,UINT cap) {
 int st=state(),r;if(st>=0)states[st].error=0;
 r=titleof(s,out,cap);if(st>=0&&r<0)states[st].error=FNERR_INVALIDFILENAME;
 return r;
}
DWORD FAR PASCAL CommDlgExtendedError(void) {int st=state();return st<0?CDERR_MEMALLOCFAILURE:states[st].error;}
BOOL FAR PASCAL LibMain(HINSTANCE h,WORD ds,WORD heap,LPSTR cmd) {module=h;if(heap&&!LocalInit(ds,0,heap))return FALSE;return TRUE;}
int FAR PASCAL WEP(int reason){return 1;}
static void error(CTX c,DWORD e){states[c->slot].error=e;}
static BOOL capture(CTX c,UINT d) {
 if(d>=26)return FALSE;if(c->captured[d])return TRUE;
 c->cwd[d][0]=(char)('A'+d);c->cwd[d][1]=':';c->cwd[d][2]='\\';
 if(!DOSGETDIR(c->cwd[d]+3,d+1))return FALSE;
 c->captured[d]=1;return TRUE;
}
static BOOL changedir(CTX c,LPCSTR path) {
 UINT old=DOSDRIVE(),d=old;
 if(path[0]&&path[1]==':'){d=upper(path[0])-'A';if(d>=26)return FALSE;}
 if(!capture(c,d))return FALSE;
 if(d!=old){DOSCHDRV(d);if(DOSDRIVE()!=d)return FALSE;}
 if(path[0]&&path[1]==':'&&!path[2])return TRUE;
 if(DOSSETDIR(path))return TRUE;
 if(d!=old)DOSCHDRV(old);return FALSE;
}
static BOOL restore(CTX c) {
 UINT i;BOOL ok=TRUE;
 if(!(c->in.Flags&OFN_NOCHANGEDIR))return TRUE;
 for(i=0;i<26;i++)if(c->captured[i]){DOSCHDRV(i);if(DOSDRIVE()!=i||!DOSSETDIR(c->cwd[i]))ok=FALSE;}
 DOSCHDRV(c->originalDrive);if(DOSDRIVE()!=c->originalDrive)ok=FALSE;
 return ok;
}
static BOOL filters(CTX c) {
 LPCSTR p=c->in.lpstrFilter;UINT n,i,k;char ch;
 if(!p){copy(c->label[0],"All files",40);copy(c->filter[0],"*.*",64);c->filters=1;c->index=0;return TRUE;}
 for(i=0;i<NFILTER;i++) {
  n=length(p,40);if(n==40)return FALSE;if(!n)break;
  if(!span(p,n+2))return FALSE;copy(c->label[i],p,40);p+=n+1;n=length(p,64);if(!n||n==64)return FALSE;
  for(k=0;k<n;k++){ch=p[k];if(ch=='\\'||ch=='/'||ch==':'||ch=='"'||ch=='<'||ch=='>'||ch=='|'||(BYTE)ch<33)return FALSE;}
  if(!span(p,n+2))return FALSE;copy(c->filter[i],p,64);p+=n+1;
 }
 if(!i||i==NFILTER&&*p)return FALSE;c->filters=i;
 c->index=(c->in.nFilterIndex>=1L&&c->in.nFilterIndex<=(DWORD)i)?(int)c->in.nFilterIndex-1:0;
 return TRUE;
}
static HWND control(HWND w,LPCSTR cls,LPCSTR text,DWORD style,int x,int y,int cx,int cy,int id) {
 return CreateWindow(cls,text,WS_CHILD|WS_VISIBLE|style,x,y,cx,cy,w,(HMENU)id,module,NULL);
}
static BOOL controls(HWND w,CTX c) {
 int sw=GetSystemMetrics(SM_CXSCREEN),sh=GetSystemMetrics(SM_CYSCREEN);
 int ww=sw<200?sw:sw<400?292:392,ch,fh,y,bw;RECT r;HDC dc;TEXTMETRIC tm;HWND h;
 dc=GetDC(w);if(!dc)return FALSE;if(!GetTextMetrics(dc,&tm)){ReleaseDC(w,dc);return FALSE;}ReleaseDC(w,dc);fh=tm.tmHeight;if(fh<1||fh>24)return FALSE;ch=sh-GetSystemMetrics(SM_CYCAPTION)-2*GetSystemMetrics(SM_CYDLGFRAME)-6;
 MoveWindow(w,(sw-ww)/2,1,ww,sh-2,FALSE);GetClientRect(w,&r);ww=r.right;ch=r.bottom;if(ch<6*fh+37)return FALSE;
 y=2;
 if(!control(w,"STATIC","&File:",0,3,y,ww-6,fh,IDC_LABEL))return FALSE;y+=fh;
 h=control(w,"EDIT",c->file,WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,3,y,ww-6,fh+4,IDC_FILE);if(!h)return FALSE;
 SendMessage(h,EM_LIMITTEXT,127,0L);y+=fh+5;
 if(!control(w,"STATIC","",SS_LEFT,3,y,ww-6,fh,IDC_DIR))return FALSE;y+=fh+1;
 if(!control(w,"LISTBOX","",WS_BORDER|WS_VSCROLL|WS_TABSTOP|LBS_NOTIFY|LBS_SORT,3,y,ww-6,ch-y-(fh+5)*3-6,IDC_LIST))return FALSE;
 y=ch-(fh+5)*3-4;
 if(!control(w,"COMBOBOX","",WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,3,y,ww-6,fh*6,IDC_TYPE))return FALSE;y+=fh+6;
 h=control(w,"BUTTON","&Read only",WS_TABSTOP|BS_AUTOCHECKBOX,3,y,ww-6,fh+3,IDC_RO);if(!h)return FALSE;
 if(c->in.Flags&OFN_HIDEREADONLY)ShowWindow(h,SW_HIDE);
 if(c->in.Flags&OFN_READONLY)SendMessage(h,BM_SETCHECK,1,0L);
 y=ch-fh-5;bw=(ww-9)/2;
 if(!control(w,"BUTTON",c->save?"&Save":"&Open",WS_TABSTOP|BS_DEFPUSHBUTTON,3,y,bw,fh+4,IDOK))return FALSE;
 if(!control(w,"BUTTON","Cancel",WS_TABSTOP|BS_PUSHBUTTON,bw+6,y,bw,fh+4,IDCANCEL))return FALSE;
 return TRUE;
}
static void refresh(HWND w,CTX c) {
 UINT i,j;LPSTR p=c->filter[c->index];
 copy(c->work,"*.*",MAXPATH);
 DlgDirList(w,c->work,IDC_LIST,IDC_DIR,DDL_DIRECTORY|DDL_DRIVES|DDL_EXCLUSIVE);
 while(*p) {
  for(j=0;*p&&*p!=';'&&j<MAXPATH-1;j++)c->work[j]=*p++;
  c->work[j]=0;if(*p==';')p++;
  if(j)SendDlgItemMessage(w,IDC_LIST,LB_DIR,DDL_READONLY|DDL_ARCHIVE,(LONG)(LPSTR)c->work);
 }
 for(i=0;i<MAXPATH;i++)c->work[i]=0;
}
static BOOL haswild(LPCSTR p){UINT i;for(i=0;p[i];i++)if(p[i]=='*'||p[i]=='?')return TRUE;return FALSE;}

typedef struct {BOOL yesno;char text[64],caption[64];} ASK;
typedef ASK FAR *LPASK;
BOOL FAR PASCAL PromptProc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
 int st,width,height,fh,i;HGLOBAL h;LPASK a;HDC dc;TEXTMETRIC tm;RECT r;
 if(msg==WM_INITDIALOG){st=state();if(st<0)return FALSE;h=states[st].ask;if(!SetProp(w,"CD30ASK",h)){EndDialog(w,-1);return TRUE;}}else h=GetProp(w,"CD30ASK");
 if(!h)return FALSE;a=(LPASK)GlobalLock(h);if(!a)return FALSE;
 if(msg==WM_INITDIALOG){
  dc=GetDC(w);if(!dc){EndDialog(w,-1);GlobalUnlock(h);return TRUE;}if(!GetTextMetrics(dc,&tm)){ReleaseDC(w,dc);EndDialog(w,-1);GlobalUnlock(h);return TRUE;}ReleaseDC(w,dc);fh=tm.tmHeight;
  width=GetSystemMetrics(SM_CXSCREEN);if(width>240)width=240;
  height=fh*4+14+GetSystemMetrics(SM_CYCAPTION)+2*GetSystemMetrics(SM_CYDLGFRAME);
  MoveWindow(w,(GetSystemMetrics(SM_CXSCREEN)-width)/2,(GetSystemMetrics(SM_CYSCREEN)-height)/2,width,height,FALSE);
  SetWindowText(w,a->caption);GetClientRect(w,&r);width=r.right;
  if(!control(w,"STATIC",a->text,SS_LEFT,4,4,width-8,fh*2+2,110)){EndDialog(w,-1);GlobalUnlock(h);return TRUE;}
  i=(width-12)/2;
  if(a->yesno){if(!control(w,"BUTTON","&Yes",WS_TABSTOP|BS_PUSHBUTTON,4,r.bottom-fh-7,i,fh+4,IDYES)||!control(w,"BUTTON","&No",WS_TABSTOP|BS_DEFPUSHBUTTON,i+8,r.bottom-fh-7,i,fh+4,IDNO)){EndDialog(w,-1);GlobalUnlock(h);return TRUE;}SetFocus(GetDlgItem(w,IDNO));}
  else {if(!control(w,"BUTTON","OK",WS_TABSTOP|BS_DEFPUSHBUTTON,(width-i)/2,r.bottom-fh-7,i,fh+4,IDOK)){EndDialog(w,-1);GlobalUnlock(h);return TRUE;}SetFocus(GetDlgItem(w,IDOK));}
  GlobalUnlock(h);return FALSE;
 }
 if(msg==WM_COMMAND){if(wp==IDYES||wp==IDNO||wp==IDOK||wp==IDCANCEL)EndDialog(w,wp);}
 else if(msg==WM_CLOSE)EndDialog(w,IDCANCEL);
 else if(msg==WM_DESTROY)RemoveProp(w,"CD30ASK");
 else {GlobalUnlock(h);return FALSE;}
 GlobalUnlock(h);return TRUE;
}
static int promptfail(HWND owner,int st,DWORD code){if(st>=0)states[st].error=code;EndDialog(owner,0);return IDCANCEL;}
static int prompt(HWND w,LPCSTR text,LPCSTR caption,BOOL yesno) {
 int st=state(),result=IDCANCEL;HGLOBAL h,t;LPASK a;BYTE FAR *b;DLGPROC proc;UINT i;
 if(st<0||states[st].ask)return promptfail(w,st,CDERR_INITIALIZATION);
 h=GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,sizeof(ASK));if(!h)return promptfail(w,st,CDERR_MEMALLOCFAILURE);
 a=(LPASK)GlobalLock(h);if(!a){GlobalFree(h);return promptfail(w,st,CDERR_MEMLOCKFAILURE);}
 a->yesno=yesno;copy(a->text,text,64);
 if(titleof(caption,a->caption,64)){for(i=0;i<63&&caption[i];i++)a->caption[i]=caption[i];a->caption[i]=0;}
 t=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,32);if(!t){GlobalUnlock(h);GlobalFree(h);return promptfail(w,st,CDERR_MEMALLOCFAILURE);}
 b=(BYTE FAR *)GlobalLock(t);if(!b){GlobalFree(t);GlobalUnlock(h);GlobalFree(h);return promptfail(w,st,CDERR_MEMLOCKFAILURE);}
 *(DWORD FAR *)b=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;b[9]=70;b[11]=40;GlobalUnlock(t);
 states[st].ask=h;proc=(DLGPROC)MakeProcInstance((FARPROC)PromptProc,module);
 if(proc){result=DialogBoxIndirect(module,t,w,proc);FreeProcInstance((FARPROC)proc);}else result=-1;
 states[st].ask=0;GlobalFree(t);GlobalUnlock(h);GlobalFree(h);if(result<0)return promptfail(w,st,CDERR_INITIALIZATION);return result;
}
static void notice(HWND w,LPCSTR s){prompt(w,s,"File selection",FALSE);}
static BOOL cleantemp(CTX c) {
 if(c->pendingHandle!=HFILE_ERROR){if(_lclose(c->pendingHandle)==HFILE_ERROR)return FALSE;c->pendingHandle=HFILE_ERROR;}
 if(c->pending[0]){if(OpenFile(c->pending,&c->of,OF_DELETE)==HFILE_ERROR)return FALSE;c->pending[0]=0;}
 return TRUE;
}
static BOOL testcreate(CTX c,UINT start) {
 UINT i,j,v;HFILE f;static char hex[]="0123456789ABCDEF";
 if(start>MAXPATH-11)return FALSE;
 for(i=0;i<start;i++)c->work[i]=c->full[i];
 c->work[start]='C';c->work[start+1]='D';
 for(i=0;i<16;i++){
  v=(UINT)GetTickCount()+i;
  for(j=0;j<4;j++){c->work[start+5-j]=hex[v&15];v>>=4;}
  copy(c->work+start+6,".TMP",5);
  f=DOSNEW(c->work);
  if(f!=HFILE_ERROR){
   copy(c->pending,c->work,MAXPATH);c->pendingHandle=f;return cleantemp(c);
  }
 }
 return FALSE;
}
static BOOL accept(HWND w,CTX c) {
 UINT n,i,start=0,dot=0,need;int attr,trailing;DWORD fl=c->in.Flags;HFILE f;
 GetDlgItemText(w,IDC_FILE,c->file,MAXPATH);n=length(c->file,MAXPATH);
 if(!n){notice(w,"Enter file name.");return FALSE;}
 for(i=0;i<n;i++)if(c->file[i]=='/')c->file[i]='\\';
 if(n>1&&c->file[0]=='\\'&&c->file[1]=='\\'){notice(w,"Local DOS paths.");return FALSE;}
 attr=DOSATTR(c->file);
 if(attr>=0&&(attr&16)){if(changedir(c,c->file)){SetDlgItemText(w,IDC_FILE,"");refresh(w,c);}else notice(w,"Bad directory.");return FALSE;}
 if(n==2&&c->file[1]==':'){if(changedir(c,c->file)){SetDlgItemText(w,IDC_FILE,"");refresh(w,c);}else notice(w,"Drive unavailable.");return FALSE;}
 if(haswild(c->file)) {
  /* A wildcard entry is a filter, never a selected filename. */
  if(n>=64||!pathok(c->file,TRUE)){notice(w,"Bad pattern.");return FALSE;}
  for(i=0;i<n;i++)if(c->file[i]=='\\'||c->file[i]==':'){notice(w,"Choose directory.");return FALSE;}
  copy(c->filter[c->index],c->file,64);refresh(w,c);return FALSE;
 }
 if(!pathok(c->file,FALSE)){notice(w,"Use DOS 8.3 name.");return FALSE;}
 trailing=c->file[n-1]=='.';
 for(i=0;i<n;i++){if(c->file[i]=='\\'||c->file[i]==':'){start=i+1;dot=0;}else if(c->file[i]=='.')dot=i;}
 if(!dot&&c->defext[0]) {
  need=length(c->defext,4);if(n+need+1>=MAXPATH){error(c,FNERR_INVALIDFILENAME);EndDialog(w,0);return FALSE;}
  c->file[n++]='.';for(i=0;i<=need;i++)c->file[n+i]=c->defext[i];
 }
 if(OpenFile(c->file,&c->of,OF_PARSE)==HFILE_ERROR||!pathok(c->of.szPathName,FALSE)){notice(w,"Bad path.");return FALSE;}
 copy(c->full,c->of.szPathName,MAXPATH);n=length(c->full,MAXPATH);dot=0;start=0;
 for(i=0;i<n;i++){c->full[i]=upper(c->full[i]);if(c->full[i]=='\\'||c->full[i]==':'){start=i+1;dot=0;}else if(c->full[i]=='.')dot=i;}
 attr=DOSATTR(c->full);
 copy(c->path,c->full,MAXPATH);if(start>3)c->path[start-1]=0;else c->path[start]=0;
 if(fl&(OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_CREATEPROMPT)||c->save) {
  i=DOSATTR(c->path);if(i==(UINT)-1||!(i&16)){notice(w,"No such path.");return FALSE;}
 }
 if(attr<0&&(fl&OFN_FILEMUSTEXIST)&&!(fl&OFN_CREATEPROMPT)){notice(w,"No such file.");return FALSE;}
 if(attr>=0&&(attr&16)){notice(w,"Select a file.");return FALSE;}
 if(attr>=0&&(attr&1)&&(fl&OFN_NOREADONLYRETURN)){notice(w,"Read only file.");return FALSE;}
 if(attr>=0&&c->save&&(fl&OFN_OVERWRITEPROMPT))if(prompt(w,"Replace file?",c->full,TRUE)!=IDYES)return FALSE;
 if(attr<0&&(fl&OFN_CREATEPROMPT))if(prompt(w,"Create file?",c->full,TRUE)!=IDYES)return FALSE;
 /* Probe existing files without truncation. New-file creation belongs to caller. */
 if(attr>=0&&(c->save||(fl&OFN_NOREADONLYRETURN))){f=OpenFile(c->full,&c->of,OF_WRITE|OF_SHARE_DENY_NONE);if(f==HFILE_ERROR){notice(w,"Cannot write.");return FALSE;}_lclose(f);}
 need=n+1;
 if(c->in.nMaxFile<(DWORD)need) {
  if(c->in.nMaxFile>=2L){c->in.lpstrFile[0]=(char)need;c->in.lpstrFile[1]=(char)(need>>8);}
  error(c,FNERR_BUFFERTOOSMALL);EndDialog(w,0);return FALSE;
 }
 if(((c->save&&attr<0)||(fl&OFN_NOREADONLYRETURN))&&!testcreate(c,start)){
  if(c->pending[0]){error(c,CDERR_INITIALIZATION);if(!cleantemp(c))prompt(w,"Temp cleanup failed.",c->pending,FALSE);EndDialog(w,0);}else notice(w,"Cannot create.");
  return FALSE;
 }
 copy(c->in.lpstrFile,c->full,need);
 if(c->in.lpstrFileTitle&&c->in.nMaxFileTitle) {
  need=n-start;if((DWORD)need>=c->in.nMaxFileTitle)need=(UINT)c->in.nMaxFileTitle-1;
  for(i=0;i<need;i++)c->in.lpstrFileTitle[i]=c->full[start+i];c->in.lpstrFileTitle[need]=0;
 }
 c->out->nFileOffset=start;c->out->nFileExtension=trailing?0:dot?dot+1:n;
 c->out->nFilterIndex=(DWORD)c->index+1;
 fl&=~(OFN_READONLY|OFN_EXTENSIONDIFFERENT);
 if(SendDlgItemMessage(w,IDC_RO,BM_GETCHECK,0,0L))fl|=OFN_READONLY;
 if(c->defext[0]&&dot&&c->full[dot+1]){
  for(i=0;i<4;i++){char actual=c->full[dot+1+i];
   if(upper(actual)!=upper(c->defext[i])){fl|=OFN_EXTENSIONDIFFERENT;break;}
   if(!actual)break;
  }
 }
 c->out->Flags=fl;error(c,0);EndDialog(w,1);return TRUE;
}
BOOL FAR PASCAL FileDlgProc(HWND w,UINT msg,WPARAM wp,LPARAM lp) {
 HGLOBAL h;CTX c;int st,i;BOOL dir;
 if(msg==WM_INITDIALOG){st=state();if(st<0)return FALSE;h=states[st].active;if(!h)return FALSE;if(!SetProp(w,prop,h)){states[st].error=CDERR_MEMALLOCFAILURE;EndDialog(w,0);return TRUE;}}else h=GetProp(w,prop);
 if(!h)return FALSE;c=(CTX)GlobalLock(h);if(!c)return FALSE;
 switch(msg) {
 case WM_INITDIALOG:
  SetWindowText(w,c->title);if(!controls(w,c)){error(c,CDERR_INITIALIZATION);EndDialog(w,0);break;}
  for(i=0;i<c->filters;i++)SendDlgItemMessage(w,IDC_TYPE,CB_ADDSTRING,0,(LONG)(LPSTR)c->label[i]);
  SendDlgItemMessage(w,IDC_TYPE,CB_SETCURSEL,c->index,0L);refresh(w,c);
  if(c->file[0])SendDlgItemMessage(w,IDC_FILE,EM_SETSEL,0,MAKELONG(0,32767));
  GlobalUnlock(h);return TRUE;
 case WM_COMMAND:
  if(wp==IDCANCEL){error(c,0);EndDialog(w,0);break;}
  if(wp==IDOK){accept(w,c);break;}
  if(wp==IDC_TYPE&&HIWORD(lp)==CBN_SELCHANGE){i=(int)SendDlgItemMessage(w,IDC_TYPE,CB_GETCURSEL,0,0L);if(i>=0&&i<c->filters){c->index=i;refresh(w,c);}break;}
  if(wp==IDC_LIST&&(HIWORD(lp)==LBN_SELCHANGE||HIWORD(lp)==LBN_DBLCLK)) {
   dir=DlgDirSelect(w,c->work,IDC_LIST);
   if(dir){if(HIWORD(lp)==LBN_DBLCLK){if(changedir(c,c->work))refresh(w,c);else notice(w,"Bad directory.");}}
   else {SetDlgItemText(w,IDC_FILE,c->work);if(HIWORD(lp)==LBN_DBLCLK)accept(w,c);}
  }
  break;
 case WM_CLOSE:error(c,0);EndDialog(w,0);break;
 case WM_DESTROY:RemoveProp(w,prop);break;
 default:GlobalUnlock(h);return FALSE;
 }
 GlobalUnlock(h);return TRUE;
}
static BOOL dialog(LPOPENFILENAME p,int save) {
 int st=state(),result=0;HGLOBAL h,t;CTX c;BYTE FAR *b;UINT i,n;DLGPROC proc;
 if(st<0)return FALSE;states[st].error=0;
 if(!p||!span((LPCSTR)p,72)||p->lStructSize!=72L){states[st].error=CDERR_STRUCTSIZE;return FALSE;}
 if(states[st].active||(p->Flags&~SUPPORTED)||p->lpstrCustomFilter){states[st].error=CDERR_INITIALIZATION;return FALSE;}
 if(!p->lpstrFile||!p->nMaxFile||p->nMaxFile>65535L||!span(p->lpstrFile,(UINT)p->nMaxFile)) {states[st].error=FNERR_INVALIDFILENAME;return FALSE;}
 if(p->lpstrFileTitle&&(p->nMaxFileTitle>65535L||!span(p->lpstrFileTitle,(UINT)p->nMaxFileTitle))){states[st].error=FNERR_INVALIDFILENAME;return FALSE;}
 h=GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,sizeof(CONTEXT));if(!h){states[st].error=CDERR_MEMALLOCFAILURE;return FALSE;}
 c=(CTX)GlobalLock(h);if(!c){GlobalFree(h);states[st].error=CDERR_MEMLOCKFAILURE;return FALSE;}
 c->self=h;c->pendingHandle=HFILE_ERROR;c->slot=st;c->out=p;c->save=save;c->in=*p;c->originalDrive=DOSDRIVE();
 if(!capture(c,c->originalDrive)){error(c,CDERR_INITIALIZATION);goto done;}
 n=(UINT)p->nMaxFile;if(n>MAXPATH)n=MAXPATH;
 if(length(p->lpstrFile,n)==n||!copy(c->file,p->lpstrFile,MAXPATH)){error(c,FNERR_INVALIDFILENAME);goto done;}
 if(!filters(c)){error(c,CDERR_INITIALIZATION);goto done;}
 if(p->lpstrTitle){if(!copy(c->title,p->lpstrTitle,64)){error(c,CDERR_INITIALIZATION);goto done;}}else copy(c->title,save?"Save file":"Open file",64);
 if(p->lpstrDefExt){n=length(p->lpstrDefExt,4);if(n==4){error(c,FNERR_INVALIDFILENAME);goto done;}for(i=0;i<n;i++)if(badchar(p->lpstrDefExt[i])||p->lpstrDefExt[i]=='.'||p->lpstrDefExt[i]=='*'||p->lpstrDefExt[i]=='?'||p->lpstrDefExt[i]=='\\'||p->lpstrDefExt[i]=='/'||p->lpstrDefExt[i]==':'){error(c,FNERR_INVALIDFILENAME);goto done;}copy(c->defext,p->lpstrDefExt,4);}
 if(p->lpstrInitialDir){if(!copy(c->path,p->lpstrInitialDir,MAXPATH)||(c->path[0]=='\\'&&c->path[1]=='\\')||!changedir(c,c->path)){error(c,FNERR_INVALIDFILENAME);goto done;}}
 n=length(c->file,MAXPATH);i=n;while(i&&c->file[i-1]!='\\'&&c->file[i-1]!='/'&&c->file[i-1]!=':')i--;
 if(i){copy(c->path,c->file,MAXPATH);if(i>3)c->path[i-1]=0;else c->path[i]=0;
  if(!changedir(c,c->path)){error(c,FNERR_INVALIDFILENAME);goto done;}
  copy(c->work,c->file+i,MAXPATH);copy(c->file,c->work,MAXPATH);
 }
 /* No resource compiler is needed: a Win16 empty dialog template, with
    native controls created in WM_INITDIALOG to fit the actual pixel width. */
 t=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,32);if(!t){error(c,CDERR_MEMALLOCFAILURE);goto done;}
 b=(BYTE FAR *)GlobalLock(t);if(!b){GlobalFree(t);error(c,CDERR_MEMLOCKFAILURE);goto done;}
 *(DWORD FAR *)b=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
 b[5]=0;b[7]=0;b[9]=70;b[11]=90; /* cdit=0; x=y=0; cx=70; cy=90 */
 GlobalUnlock(t);states[st].active=h;
 proc=(DLGPROC)MakeProcInstance((FARPROC)FileDlgProc,module);
 if(!proc){error(c,CDERR_INITIALIZATION);GlobalFree(t);goto done;}
 result=DialogBoxIndirect(module,t,p->hwndOwner,proc);
 FreeProcInstance((FARPROC)proc);GlobalFree(t);states[st].active=0;
 if(result<0){error(c,CDERR_INITIALIZATION);result=0;}
 done:
 if(c->pending[0]){
  cleantemp(c);
  error(c,CDERR_INITIALIZATION);result=0;
 }
 if(!restore(c)){error(c,CDERR_INITIALIZATION);result=0;}
 states[st].active=0;GlobalUnlock(h);GlobalFree(h);return result>0;
}
BOOL FAR PASCAL GetOpenFileName(LPOPENFILENAME p){return dialog(p,0);}
BOOL FAR PASCAL GetSaveFileName(LPOPENFILENAME p){return dialog(p,1);}
