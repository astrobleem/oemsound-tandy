#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PSG_HOST
#include "../MIDIMAP.C"
int hostTandy;
static unsigned task=1,sound,traceCount,provider,busy,leaseHeld,claims,releases;
static int pmode,failSound,failRelease;
static WORD trace[128];
static int mockLease(WORD version,WORD op)
{
    if(version!=1)abort();
    if(op==0)return busy?-1:0;
    if(op==1){if(!sound||busy||leaseHeld)return -1;leaseHeld=1;claims++;return 0;}
    if(op==2){if(!sound||!leaseHeld)abort();if(failRelease)return -1;leaseHeld=0;releases++;return 0;}
    abort();return -1;
}
WORD GetWinFlags(void){return pmode;}
int OpenSound(void){if(failSound||sound)return -1;sound=1;return 3;}
void CloseSound(void){if(!sound||leaseHeld)abort();sound=0;}
void StopSound(void){if(!sound)abort();}
WORD GetCurrentTask(void){return task;}
HMODULE GetModuleHandle(const char *s){return provider&&!strcmp(s,"SOUND")?40:0;}
FARPROC GetProcAddress(HMODULE h,const char *s){return h==40&&!strcmp(s,"TANDYPSGLEASE")?(FARPROC)mockLease:NULL;}
char *lstrcpy(char *a,const char *b){return strcpy(a,b);}
void PsgByte(unsigned v){if(!owner||!sound||traceCount>=128)abort();trace[traceCount++]=v;}
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}checks++;}while(0)
int main(void)
{
    unsigned checks=0,n;DWORD cookie=0,r;TOPEN d={0,0,0};TCAPS caps;
    C(midiMessage(0,MODM_GETDEVCAPS,0,(DWORD)&caps,sizeof(caps))==0);
    C(caps.wVoices==3&&caps.vDriverVersion==0x102);
    /* Generic CGA never implies PSG. Unknown hardware: no writes or owner. */
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ERROR);
    C(!sound&&!owner&&!traceCount);
    d.dwInstance=PSG_EXPLICIT_C0;pmode=1;
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ERROR);pmode=0;
    failSound=1;C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ALLOCATED);
    C(!sound&&!owner&&!traceCount);failSound=0;
    /* Explicit C0 configuration works without a Tandy ROM. */
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==0);
    C(cookie==MIDI_COOKIE&&sound&&owner==1&&traceCount==4&&DebugState(16)==1);
    C(midiMessage(0,MODM_DATA,cookie,0x00643c90UL,0)==0&&DebugState(0)==1);
    task=2;n=traceCount;C(midiMessage(0,MODM_CLOSE,cookie,0,0)==MMSYSERR_ALLOCATED&&traceCount==n);task=1;
    C(midiMessage(0,MODM_CLOSE,cookie,0,0)==0&&!sound&&!owner&&!DebugState(0));
    /* Tandy ROM also qualifies independently of display. */
    hostTandy=1;d.dwInstance=0;C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==0);
    C(midiMessage(0,MODM_CLOSE,cookie,0,0)==0);hostTandy=0;
    /* A lease export is not hardware detection. Explicit card ownership
       must still claim the shared lease before any PSG I/O. */
    provider=1;busy=1;n=traceCount;
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ERROR&&traceCount==n);
    d.dwInstance=PSG_EXPLICIT_C0;
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ALLOCATED);
    C(!sound&&!owner&&!claimed&&traceCount==n);busy=0;d.dwInstance=0;
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==MMSYSERR_ERROR);
    C(!sound&&!owner&&!claimed&&traceCount==n);
    d.dwInstance=PSG_EXPLICIT_C0;
    C(midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0)==0&&claimed&&leaseHeld);
    C(midiMessage(0,MODM_RESET,cookie,0,0)==0);
    failRelease=1;
    C(midiMessage(0,MODM_CLOSE,cookie,0,0)==MMSYSERR_ERROR);
    C(sound&&owner&&claimed&&leaseHeld&&DebugState(17)&&!DebugState(0));
    failRelease=0;
    C(midiMessage(0,MODM_CLOSE,cookie,0,0)==0&&!sound&&!owner&&!claimed&&!leaseHeld);
    C(claims==1&&releases==1&&!DebugState(17));
    n=traceCount;d.dwCallback=1;r=midiMessage(0,MODM_OPEN,(DWORD)&cookie,(DWORD)&d,0);
    C(r==MMSYSERR_NOTSUPPORTED&&traceCount==n&&!sound);
    C(WEP(0)==1&&traceCount==n);
    printf("PASS %u checks; explicit hardware selection, shared lease and clean PSG release\n",checks);return 0;
}
