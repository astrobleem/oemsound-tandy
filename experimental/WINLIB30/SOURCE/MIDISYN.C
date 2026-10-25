/* Pure foreground renderer. No Windows/DOS API, port IO or allocation.
   Stage5 voice allocation, note ranges, percussion and envelopes retained. */
#include "MIDISYN.H"
#include "LTMATH.H"
#include "MIDIMSG.H"
#include "PERIODS.H"
#include "PITNOTES.H"
/* Host unsigned long is64bit; retain native32bit subtraction before division. */
#ifdef SYNTH_HOST
#define MS_ZERO ((LTP_U32)0)
#define MS_54 ((LTP_U32)54)
#else
#define MS_ZERO 0UL
#define MS_54 54UL
#endif
static LTP_U32 step(LTP_U32 ms){return ((LTP_I32)ms<0)?MS_ZERO-lt_div16(MS_ZERO-ms+MS_54,55):lt_div16(ms,55);}
void MSReset(MSYN *s){unsigned i;LTP_U8 *p=(LTP_U8 *)s;for(i=0;i<sizeof(*s);i++)p[i]=0;
 for(i=0;i<4;i++)s->baseVolume[i]=(LTP_U8)(s->volume[i]=15);
 for(i=0;i<16;i++){s->channelVolume[i]=s->expression[i]=127;s->pan[i]=64;s->pitchBend[i]=8192;s->bendFactor[i]=4096;}s->flags=3;s->renderDirty=1;s->renderValid=0;}
void MSInvalidate(MSYN *s){s->renderDirty=1;}
void MSSpeakerMode(MSYN *s,LTP_U16 mode){MSInvalidate(s);s->speakerMode=mode;s->speakerNote=s->speakerDivisor=s->speakerBase=s->speakerFallback=0;s->speakerReleased=0;}
/* Cache the extra 2dB attenuation only when a controller changes. PSG gain is
   quantized; PIT2 has fixed amplitude and supports only the zero/nonzero gate. */
static void gain(MSYN *s,LTP_U16 ch){
 static const LTP_U8 threshold[14]={113,90,71,57,45,36,28,23,18,14,11,9,7,6};
 LTP_U16 v=(s->channelVolume[ch]*s->expression[ch]+63U)/127U,i=0;
 if(!v)i=15;else while(i<14&&v<threshold[i])i++;s->gain[ch]=(LTP_U8)i;
}
static void off(MSYN *s,LTP_U16 i){s->volume[i]=s->baseVolume[i]=15;s->released[i]=0;
 if(i<3)s->note[i]=0;else s->noiseNote=0;}
static void speakerOff(MSYN *s){s->speakerNote=s->speakerDivisor=s->speakerBase=s->speakerFallback=0;s->speakerReleased=0;}
static LTP_U16 bendFactor(LTP_U16 bend){
 static const LTP_U16 factor[17]={4598,4532,4467,4403,4340,4277,4216,4156,
  4096,4037,3979,3922,3866,3811,3756,3702,3649};
 unsigned index=(unsigned)bend>>10,part=(unsigned)bend&1023U;
 LTP_I32 difference;
 if(index>=16U)return factor[16];
 difference=(LTP_I32)factor[index+1]-factor[index];
 return (LTP_U16)(factor[index]+(difference*(LTP_I32)part)/1024L);
}
static LTP_U16 bendPeriod(LTP_U16 period,LTP_U16 factor,LTP_U16 maximum){
 LTP_U32 scaled;
 if(factor==4096U)return period;
 scaled=((LTP_U32)period*factor+2048UL)>>12;
 if(!scaled)return 1;
 if(scaled>maximum)return maximum;
 return (LTP_U16)scaled;
}
static void lift(MSYN *s,LTP_U16 ch){LTP_U16 i;
 for(i=0;i<4;i++)if(s->released[i]&&(i<3?s->channel[i]:9)==ch)off(s,i);
 if(s->speakerReleased&&s->speakerChannel==ch)speakerOff(s);
}
/* A prepared PIT voice must retain its original pitch. If the original
   divisor cannot be represented, the paired PSG renderer supplies fallback. */
static void speakerPitch(MSYN *s){LTP_U32 divisor;
 s->speakerDivisor=s->speakerFallback=0;
 if(!s->speakerNote||s->gain[s->speakerChannel]==15)return;
 if(!s->speakerBase){s->speakerFallback=1;return;}
 divisor=((LTP_U32)s->speakerBase*s->bendFactor[s->speakerChannel]+2048UL)>>12;
 if(!divisor||divisor>65535UL){s->speakerFallback=1;return;}
 s->speakerDivisor=(LTP_U16)divisor;
}
static void speakerNote(MSYN *s,LTP_U16 note,LTP_U16 channel){
 s->speakerNote=note+1;s->speakerChannel=channel;
 s->speakerBase=note<14U?0:pitNotes[note];
 s->speakerReleased=0;speakerPitch(s);
 if(s->speakerAttacks!=65535U)s->speakerAttacks++;
}
/* InstRead receives near pointers: keep these in DS, not the caller's SS. */
void MSRender(MSYN *s,LTP_U32 ms){unsigned i,ch;static LTP_U16 p,v;LTP_U16 flags;LTP_U32 q;
 q=s->renderValid&&s->ms==ms?s->renderQuantum:step(ms);s->ms=ms;flags=s->flags;
 if(s->renderValid&&!s->renderDirty&&s->renderQuantum==q&&s->renderFlags==flags)return;
 for(i=0;i<4;i++)if(i<3?s->note[i]:s->noiseNote){
  ch=i<3?s->channel[i]:9;p=s->voice[i].period;
  s->voice[i].modulation=s->modulation[ch];
  if(s->flags&LTP_ENVELOPES){if(!InstRead(&s->voice[i],q,s->flags,&p,&v)){off(s,(LTP_U16)i);continue;}
   s->baseVolume[i]=(LTP_U8)v;}
  p=bendPeriod(p,s->bendFactor[ch],1023);
  if(i<3)s->p[i]=p;
  v=s->baseVolume[i]+s->gain[ch];s->volume[i]=v>15?15:v;
 }
 speakerPitch(s);
 s->renderQuantum=q;s->renderFlags=flags;s->renderDirty=0;s->renderValid=1;
}

static LTP_U16 applyMessage(MSYN *s,LTP_U32 ms,LTP_U32 value,unsigned renderNow){unsigned status,ch,a,b,i,j,slot=3,oldest=65535,ranks[3],volume,preset,pitch;
 status=(LTP_U16)value&240;ch=(LTP_U16)value&15;a=((LTP_U16)value>>8)&255;b=(LTP_U16)(value>>16)&255;
 if(!MSValidMessage(value,s->speakerMode))return 1;
 if(renderNow)MSRender(s,ms);
 MSInvalidate(s);
 if(status==0xc0){s->program[ch]=a;return 0;}
 if(status==0xe0){s->pitchBend[ch]=(LTP_U16)(a|(b<<7));s->bendFactor[ch]=bendFactor(s->pitchBend[ch]);if(renderNow)MSRender(s,ms);return 0;}
 if(status==0xb0){
  if(a==0||a==32||a==91||a==93)return 0;
  if(a==1){s->modulation[ch]=(LTP_U8)b;for(i=0;i<4;i++)if((i<3?s->note[i]:s->noiseNote)&&(i<3?s->channel[i]:9)==ch)s->voice[i].modulation=(LTP_U8)b;}
  else if(a==7||a==11){if(a==7)s->channelVolume[ch]=(LTP_U8)b;else s->expression[ch]=(LTP_U8)b;gain(s,ch);}
  else if(a==10)s->pan[ch]=(LTP_U8)b; /* Deliberate mono arrangement; UI reports it. */
  else if(a==64){s->sustain[ch]=(LTP_U8)(b>=64);if(b<64)lift(s,ch);}
  else if(a==121){s->expression[ch]=127;s->modulation[ch]=0;s->pitchBend[ch]=8192;s->bendFactor[ch]=4096;s->sustain[ch]=0;gain(s,ch);lift(s,ch);}
  else {
   for(i=0;i<4;i++)if((i<3?s->note[i]:s->noiseNote)&&(i<3?s->channel[i]:9)==ch){
    if(a==123&&s->sustain[ch])s->released[i]=1;else off(s,i);}
   if(s->speakerNote&&s->speakerChannel==ch){if(a==123&&s->sustain[ch])s->speakerReleased=1;else speakerOff(s);}
  }if(renderNow)MSRender(s,ms);return 0;
 }
 if(status==0x80||(status==0x90&&!b)){if(ch==9){if(s->noiseNote==a){if(s->sustain[ch])s->released[3]=1;else off(s,3);}}
 else {if(s->speakerNote==a+1&&s->speakerChannel==ch){if(s->sustain[ch])s->speakerReleased=1;else speakerOff(s);}
 for(i=0;i<3;i++)if(s->note[i]&&(s->key[i]?s->key[i]-1:s->note[i])==a&&s->channel[i]==ch){if(s->sustain[ch])s->released[i]=1;else off(s,i);}}return 0;}
 if(status!=0x90)return 1;volume=14-((b-1)*14)/126;
 if(ch==9){if(a<35||a>81)return 1;s->noise=(a==35||a==36)?2:(a==38||a==40)?5:(a==42||a==44||a==46)?4:6;s->noiseNote=a;s->volume[3]=volume;s->baseVolume[3]=(LTP_U8)volume;s->released[3]=0;
  preset=(a==35||a==36)?8:(a==42||a==44||a==46)?10:9;InstStart(&s->voice[3],preset,volume,1,step(ms));if(renderNow)MSRender(s,ms);return 0;}
 if(!s->speakerMode&&a<45)return 1;
 /* The prepared PIT lookup ends at 96. Extended high notes stay on PSG. */
 if(s->speakerMode==1&&(a<45||(ch==15&&a<=96)||
    (s->speakerNote==a+1&&s->speakerChannel==ch))){speakerNote(s,a,ch);return 0;}
 pitch=a;while(pitch<45U)pitch+=12U;
 if(a<45&&s->bassFolded!=65535U)s->bassFolded++;
 for(i=0;i<3;i++)if(s->note[i]&&(s->key[i]?s->key[i]-1:s->note[i])==a&&s->channel[i]==ch)slot=i;
 if(slot==3)for(i=0;i<3;i++)if(!s->note[i]){slot=i;break;}
 if(slot==3&&s->speakerMode==1&&!s->speakerNote&&a<=96){speakerNote(s,a,ch);return 0;}
 if(slot==3){
  /* Reclaim pedal-released voices first, then preserve the lowest sounding key. */
  for(i=0;i<3;i++)if(s->released[i]&&(slot==3||s->age[i]<oldest)){slot=i;oldest=s->age[i];}
  if(slot==3){unsigned bass=0;for(i=1;i<3;i++)if(s->key[i]<s->key[bass])bass=i;
   for(i=0;i<3;i++)if(i!=bass&&(slot==3||s->age[i]<oldest)){slot=i;oldest=s->age[i];}
   if(slot==3)for(i=0;i<3;i++)if(s->age[i]<oldest){slot=i;oldest=s->age[i];}
  }
 }
 if(s->counter==65535){for(i=0;i<3;i++){ranks[i]=1;for(j=0;j<3;j++)if(s->age[j]<s->age[i])ranks[i]++;}for(i=0;i<3;i++)s->age[i]=ranks[i];s->counter=3;}
 s->note[slot]=pitch;s->key[slot]=a+1;s->channel[slot]=ch;s->age[slot]=++s->counter;s->p[slot]=periods[pitch];s->volume[slot]=volume;s->baseVolume[slot]=(LTP_U8)volume;s->released[slot]=0;
 InstStart(&s->voice[slot],s->program[ch]>>4,volume,s->p[slot],step(ms));if(renderNow)MSRender(s,ms);return 0;
}
LTP_U16 MSMessage(MSYN *s,LTP_U32 ms,LTP_U32 value)
{return applyMessage(s,ms,value,1);}
LTP_U16 MSMessageDeferred(MSYN *s,LTP_U32 ms,LTP_U32 value)
{return applyMessage(s,ms,value,0);}
/* Exact floor(ms * 3579545 / 3000), modulo32 bits. No overflowing
   intermediate fraction; the explicit final modulo supports one-hour SMF. */
LTP_U32 MSCycles(LTP_U32 ms){LTP_U32 sec=lt_div16(ms,1000),remain=ms-sec*1000UL;
 LTP_U32 phase=sec*2UL,third=lt_div16(phase,3);phase-=third*3UL;
 return sec*1193181UL+third+lt_div16(remain*3579545UL+phase*1000UL,3000);}
