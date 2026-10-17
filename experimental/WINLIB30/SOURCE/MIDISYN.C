/* Pure foreground renderer. No Windows/DOS API, port IO or allocation.
   Stage5 voice allocation, note ranges, percussion and envelopes retained. */
#include "MIDISYN.H"
#include "PERIODS.H"
/* Host unsigned long is64bit; retain native32bit subtraction before division. */
#ifdef SYNTH_HOST
#define MS_ZERO ((DWORD)0)
#define MS_54 ((DWORD)54)
#else
#define MS_ZERO 0UL
#define MS_54 54UL
#endif
static DWORD step(DWORD ms){return ((MSLONG)ms<0)?MS_ZERO-((MS_ZERO-ms+MS_54)/55UL):ms/55UL;}
void MSReset(MSYN *s){unsigned i;BYTE *p=(BYTE *)s;for(i=0;i<sizeof(*s);i++)p[i]=0;for(i=0;i<4;i++)s->volume[i]=15;s->flags=1;}
void MSRender(MSYN *s,DWORD ms){unsigned i;static WORD p,v;s->ms=ms;if(!(s->flags&1))return;
 for(i=0;i<4;i++)if(i<3?s->note[i]:s->noiseNote){
  if(!InstRead(&s->voice[i],step(ms),s->flags,&p,&v)){s->volume[i]=15;if(i<3)s->note[i]=0;else s->noiseNote=0;}
  else {s->volume[i]=v;if(i<3)s->p[i]=p;}
 }
}
WORD MSMessage(MSYN *s,DWORD ms,DWORD value){unsigned status,ch,a,b,i,j,slot=3,oldest=65535,ranks[3],volume,preset;
 status=(WORD)value&240;ch=(WORD)value&15;a=((WORD)value>>8)&255;b=(WORD)(value>>16)&255;
 if(value&0xff000000UL||a>127||b>127)return 1;
 if(!(status==0x80||status==0xc0||(status==0xb0&&(a==120||a==123))||(status==0x90&&(!b||(ch==9?(a>=35&&a<=81):(a>=45&&a<=96))))))return 1;
 MSRender(s,ms);
 if(status==0xc0){s->program[ch]=a;return 0;}
 if(status==0xb0){if(a!=120&&a!=123)return 1;for(i=0;i<3;i++)if(s->note[i]&&s->channel[i]==ch){s->note[i]=0;s->volume[i]=15;}if(ch==9){s->noiseNote=0;s->volume[3]=15;}return 0;}
 if(status==0x80||(status==0x90&&!b)){if(ch==9){if(s->noiseNote==a){s->noiseNote=0;s->volume[3]=15;}}else for(i=0;i<3;i++)if(s->note[i]==a&&s->channel[i]==ch){s->note[i]=0;s->volume[i]=15;}return 0;}
 if(status!=0x90)return 1;volume=14-((b-1)*14)/126;
 if(ch==9){if(a<35||a>81)return 1;s->noise=(a==35||a==36)?2:(a==38||a==40)?5:(a==42||a==44||a==46)?4:6;s->noiseNote=a;s->volume[3]=volume;
  preset=(a==35||a==36)?8:(a==42||a==44||a==46)?10:9;InstStart(&s->voice[3],preset,volume,1,step(ms));MSRender(s,ms);return 0;}
 if(a<45||a>96)return 1;
 for(i=0;i<3;i++)if(s->note[i]==a&&s->channel[i]==ch)slot=i;
 if(slot==3)for(i=0;i<3;i++)if(!s->note[i]){slot=i;break;}
 if(slot==3)for(i=0;i<3;i++)if(s->age[i]<oldest){slot=i;oldest=s->age[i];}
 if(s->counter==65535){for(i=0;i<3;i++){ranks[i]=1;for(j=0;j<3;j++)if(s->age[j]<s->age[i])ranks[i]++;}for(i=0;i<3;i++)s->age[i]=ranks[i];s->counter=3;}
 s->note[slot]=a;s->channel[slot]=ch;s->age[slot]=++s->counter;s->p[slot]=periods[a-45];s->volume[slot]=volume;
 InstStart(&s->voice[slot],s->program[ch]>>4,volume,s->p[slot],step(ms));MSRender(s,ms);return 0;
}
/* Exact floor(ms * 3579545 / 3000), modulo32 bits. No overflowing
   intermediate fraction; the explicit final modulo supports one-hour SMF. */
DWORD MSCycles(DWORD ms){DWORD sec=ms/1000UL;return sec*1193181UL+(sec*2UL)/3UL+((ms%1000UL)*3579545UL+((sec*2UL)%3UL)*1000UL)/3000UL;}
