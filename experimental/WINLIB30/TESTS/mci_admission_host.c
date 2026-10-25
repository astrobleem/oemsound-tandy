/* Actual SMF scanner + MCI foreground admission; no timer/Windows runtime. */
#include <stdio.h>
#include <string.h>
#include "../SOURCE/MCITNDY.C"
static unsigned char data[65535];
static unsigned bytes,pos,reads,seeks,checks,failures,callbacks,rejectAt;
static WORD bankChoice;
WORD SCBank(WORD b){if(b>1)return 1;bankChoice=b;return 0;}
WORD SCStart(void){return 0;} WORD SCPump(void){return 0;}
WORD SCStop(void){return 0;} WORD SCPause(void){return 0;}
WORD SCResume(void){return 0;} DWORD SCPosition(void){return 0;}
WORD SCDebug(WORD q){(void)q;return 0;}
HFILE _lopen(LPCSTR n,unsigned f){(void)n;(void)f;pos=0;return 1;}
LONG _llseek(HFILE f,LONG n,int from){(void)f;seeks++;pos=from==2?bytes+(unsigned)n:(unsigned)n;return (LONG)pos;}
WORD _lread(HFILE f,void *d,WORD n){(void)f;reads++;if(n>bytes-pos)return 0;memcpy(d,data+pos,n);pos+=n;return n;}
int _lclose(HFILE f){(void)f;return 0;}
static void check(const char *n,int ok){checks++;if(!ok){printf("FAIL %s\n",n);failures++;}}
static void put(unsigned n){data[bytes++]=(unsigned char)n;}
static void finishData(void){unsigned n=bytes-22;data[18]=(unsigned char)(n>>24);data[19]=(unsigned char)(n>>16);data[20]=(unsigned char)(n>>8);data[21]=(unsigned char)n;}
static void make(unsigned n,unsigned split,unsigned skipped){
 unsigned i;
 static const unsigned char hdr[]={77,84,104,100,0,0,0,6,0,0,0,1,0,96,77,84,114,107,0,0,0,0};
 memcpy(data,hdr,sizeof(hdr));bytes=sizeof(hdr);
 for(i=0;i<n;i++){put(split&&i==split?1:0);put(0x90);put(i&127);put(80);
  if(skipped){put(0);put(0xa0);put(i&127);put(1);}}
 put(0);put(0xff);put(0x2f);put(0);finishData();
}
static int admit(const SMF_EVENT *e){
 callbacks++;check("original event untouched",e->status==0x90&&e->data1==((callbacks-1)&127)&&e->data2==80);
 return !rejectAt||callbacks!=rejectAt;
}
static void beginRead(void){reads=seeks=0;hfile=_lopen("memory",0);}
int main(int argc,char **argv){unsigned baselineReads,baselineSeeks,n,songEvents;SMF_ULONG songDuration;SMF_EVENT event;int r,k;FILE *song;
 device=7;owner=1;check("bank valid current owner",MciBank(7,1)==0&&bankChoice==1);
 check("bank invalid choice unchanged",MciBank(7,2)==MC_PARAM&&bankChoice==1);
 owner=2;check("bank foreign task refused",MciBank(7,0)==MC_BADID&&bankChoice==1);owner=1;
 check("bank foreign device refused",MciBank(8,0)==MC_BADID&&bankChoice==1);
 check("bank default selection",MciBank(7,0)==0&&bankChoice==0);device=owner=0;
 make(200,0,0);beginRead();check("ordinary full validation",smf_open_stream(readAt,bytes)==1);
 baselineReads=reads;baselineSeeks=seeks;check("ordinary count",smf_event_count()==200);
 beginRead();callbacks=rejectAt=0;
 check("checked full validation",smf_open_stream_checked(readAt,bytes,admit)==1);
 check("exactly one validation pass",reads==baselineReads&&seeks==baselineSeeks);
 check("all admitted once",callbacks==200&&smf_event_count()==200);
 n=callbacks;while((r=smf_next(&event))>0){}check("clean replay",r==0);
 check("no admission callback in playback",callbacks==n);
 beginRead();check("null callback compatible",smf_open_stream_checked(readAt,bytes,0)==1);
 check("null callback same work",reads==baselineReads&&seeks==baselineSeeks);
 beginRead();callbacks=0;rejectAt=17;
 check("callback rejection",smf_open_stream_checked(readAt,bytes,admit)==SMF_OPEN_REJECTED);
 check("callback stops on first rejection",callbacks==17);
 check("reject still scans full file",reads==baselineReads-1&&seeks==baselineSeeks-1);
 check("rejection disables playback",smf_next(&event)==-1);
 data[bytes-1]=1;beginRead();callbacks=0;rejectAt=1;
 check("future malformed data overrides rejection",smf_open_stream_checked(readAt,bytes,admit)==0);
 check("future malformed reason retained",strstr(smf_error(),"admission")==0);
 make(0,0,0);check("empty MCI file refused",readFile("memory")==MC_BADFILE);freeFile();
 make(1024,0,0);beginRead();check("group baseline open",smf_open_stream(readAt,bytes)==1);
 baselineReads=reads;baselineSeeks=seeks;
 reads=seeks=0;check("1024 same-ms admitted",readFile("memory")==0);
 /* readFile additionally seeks EOF once; no second channel-event walk. */
 check("MCI uses one scan",reads==baselineReads&&seeks==baselineSeeks+1);
 check("MCI accepted count unchanged",count==1024&&fileBytes==bytes);freeFile();
 make(1025,0,0);check("1025 same-ms refused",readFile("memory")==MC_BADFILE);freeFile();
 make(1025,1024,0);check("new millisecond resets guard",readFile("memory")==0);freeFile();
 make(1024,0,1);check("skipped messages do not consume group",readFile("memory")==0);
 check("all original channel events counted",count==2048);freeFile();
 make(1025,0,1);check("skip cannot bypass group limit",readFile("memory")==MC_BADFILE);freeFile();
 make(1025,0,0);data[bytes-1]=1;
 check("malformed tail after group refusal",readFile("memory")==MC_BADFILE);
 check("parser error precedes group rejection",strstr(smf_error(),"admission")==0);freeFile();
 for(k=1;k<argc;k++){
  song=fopen(argv[k],"rb");check("retained song readable",song!=0);if(!song)continue;
  bytes=(unsigned)fread(data,1,sizeof(data),song);check("retained song fits fixture",fgetc(song)==EOF);fclose(song);
  beginRead();check("retained ordinary parser",smf_open_stream(readAt,bytes)==1);
  baselineReads=reads;baselineSeeks=seeks;songEvents=smf_event_count();songDuration=smf_duration_ms();
  reads=seeks=0;check("retained MCI admission",readFile("memory")==0);
  check("retained exact single-pass reads",reads==baselineReads&&seeks==baselineSeeks+1);
  check("retained event count and duration",count==songEvents&&length==songDuration);
  printf("SONG %s bytes=%u events=%u duration=%lu single_pass_reads=%u MCI_reads=%u\n",argv[k],bytes,count,(unsigned long)length,baselineReads,reads);
  freeFile();
 }
 printf("MCI/SMF one-pass admission: %s checks=%u failures=%u\n",failures?"FAIL":"PASS",checks,failures);
 return failures!=0;
}
