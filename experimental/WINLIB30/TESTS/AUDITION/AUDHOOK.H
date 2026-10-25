/* Developer-only observation of the actual audition mixer. No synth edits. */
typedef struct {
 unsigned event,ch,note,velocity,slot,preset,bank,allocated;
 LTP_U32 on,off;
 unsigned long samples,first,last;
} AUDNOTE;
static AUDNOTE audNotes[MAX_EVENTS];
static int audOwner[4];static double audParts[4];
static unsigned audCount,audScene,audBank,audNegative;
static int audMuted;
static void AudBegin(unsigned scene,unsigned bank,unsigned negative)
{
 unsigned i;memset(audNotes,0,sizeof(audNotes));audCount=0;
 audScene=scene;audBank=bank;audNegative=negative;audMuted=-1;
 for(i=0;i<4;i++)audOwner[i]=-1;
}
static int AudEvent(unsigned event,LTP_U32 ms,LTP_U32 m,int rejected)
{
 unsigned i,ch=(unsigned)m&15,note=(unsigned)(m>>8)&127;
 unsigned status=(unsigned)m&240,velocity=(unsigned)(m>>16)&127;
 if(IsNoteOn(m)){
  AUDNOTE *a=&audNotes[audCount];a->event=event;a->ch=ch;
  a->note=note;a->velocity=velocity;a->on=ms;a->off=0xffffffffUL;
  a->slot=4;a->first=0xffffffffUL;
  if(!rejected){
   if(ch==PERC_CH){if(synth.noiseNote==note)a->slot=3;}
   else for(i=0;i<3;i++)if(synth.note[i]&&synth.channel[i]==ch&&
      (synth.key[i]?synth.key[i]-1:synth.note[i])==note){a->slot=i;break;}
  }
  if(a->slot<4){a->allocated=1;a->preset=synth.voice[a->slot].preset;
   a->bank=synth.voice[a->slot].bank;audOwner[a->slot]=(int)audCount;}
  if(audNegative&&audScene==SC_PHRASE&&audBank==0&&ch==0&&audMuted<0)
   audMuted=(int)audCount;
  audCount++;
 }else if(status==0x80||(status==0x90&&velocity==0)){
  for(i=audCount;i>0;i--)if(audNotes[i-1].ch==ch&&audNotes[i-1].note==note&&
       audNotes[i-1].off==0xffffffffUL){audNotes[i-1].off=ms;break;}
 }
 return rejected;
}
static void AudSampleBegin(void){unsigned i;for(i=0;i<4;i++)audParts[i]=0.0;}
static double AudPart(unsigned voice,double value)
{
 if(audNegative&&audOwner[voice]==audMuted)value=0.0;
 audParts[voice]=value;return value;
}
static short AudQuantize(double value)
{
 value*=32767.0;
 if(value>32767.0)value=32767.0;else if(value< -32768.0)value= -32768.0;
 return (short)(value<0.0?value-0.5:value+0.5);
}
static void AudCommit(unsigned long sample,short mixed,double sum)
{
 unsigned i;int owner;
 for(i=0;i<4;i++){
  owner=audOwner[i];
  if(owner>=0&&audParts[i]!=0.0&&AudQuantize(sum-audParts[i])!=mixed){
   AUDNOTE *a=&audNotes[owner];if(!a->samples)a->first=sample;
   a->samples++;a->last=sample;
  }
 }
}
static unsigned AudReport(FILE *f)
{
 unsigned i,missing=0;
 for(i=0;i<audCount;i++){
  AUDNOTE *a=&audNotes[i];double first=-1.0,last=-1.0;
  if(a->samples){first=1000.0*(double)a->first/(double)AUD_RATE;
   last=1000.0*(double)a->last/(double)AUD_RATE;}else missing++;
  fprintf(f,"%s,%s,%u,%u,%u,%lu,%lu,%u,%u,%u,%lu,%.3f,%.3f,%.3f,%s\n",
   scene_names[audScene],bank_names[audBank],i,a->ch+1,a->note,
   (unsigned long)a->on,(unsigned long)a->off,a->slot,a->preset,a->bank,
   a->samples,first,last,first<0?-1.0:first-(double)a->on,
   a->samples?"CONTRIBUTED":"MISSING");
 }
 return missing;
}
