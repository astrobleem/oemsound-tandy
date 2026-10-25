/* No WAV writing: compare actual normal renders with the existing takes. */
static int AudCompare(const char *directory,unsigned scene,unsigned bank,RENDER *r)
{
 char path[PATH_LEN];FILE *f;unsigned char h[44];unsigned long n;int lo,hi,value;
 sprintf(path,"%s/%s_%s.wav",directory,scene_names[scene],bank_names[bank]);
 f=fopen(path,"rb");if(!f)return 1;
 if(fread(h,1,44,f)!=44||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4)||
    memcmp(h+36,"data",4)){fclose(f);return 1;}
 for(n=0;n<r->n;n++){
  lo=fgetc(f);hi=fgetc(f);if(lo<0||hi<0){fclose(f);return 1;}
  value=lo|(hi<<8);if(value>=32768)value-=65536;
  if(r->data[n]!=(short)value){fclose(f);return 1;}
 }
 value=fgetc(f);fclose(f);return value!=EOF;
}
int main(int argc,char **argv)
{
 unsigned scene,bank,negative,missing,total=0,notes=0;FILE *csv;
 static const unsigned expected[4]={8,8,8,53};
 if(argc!=3&&argc!=4)return 2;
 negative=argc==4&&!strcmp(argv[3],"--muted-note");
 csv=fopen(argv[2],"w");if(!csv)return 2;
 fprintf(csv,"scene,bank,on_id,channel,note,on_ms,keyoff_ms,slot,preset,latched_bank,contribution_samples,first_pcm_ms,last_pcm_ms,attack_delay_ms,status\n");
 /* A velocity-zero note-on is an off, not an expected sounding note. */
 AudBegin(0,0,0);MSReset(&synth);
 if(IsNoteOn(MakeMsg(0x90,60,0))||
    AudEvent(0,0,MakeMsg(0x90,60,0),MSMessage(&synth,0,MakeMsg(0x90,60,0)))||audCount){fclose(csv);return 2;}
 puts("PASS velocity-zero note-on classified as off");
 for(scene=0;scene<SCENE_COUNT;scene++){
  unsigned long n;if(negative&&scene!=SC_PHRASE)continue;
  if(BuildScene(scene)){fclose(csv);return 2;}
  n=((list.ev[list.count-1].ms+TAIL_MS)*AUD_RATE)/1000UL;
  for(bank=0;bank<BANK_COUNT;bank++){
   RENDER r;if(negative&&bank)continue;
   AudBegin(scene,bank,negative);
   if(PrepareRender(&r,n)||RenderBank(bank,&r)){fclose(csv);return 2;}
   if(audCount!=expected[scene]||r.rejected||r.steals||r.clipped){fclose(csv);return 2;}
   if(!negative&&AudCompare(argv[1],scene,bank,&r)){fprintf(stderr,"PCM differs %s/%s\n",scene_names[scene],bank_names[bank]);fclose(csv);return 2;}
   missing=AudReport(csv);total+=missing;notes+=audCount;
   printf("scene=%s bank=%s expected=%u missing=%u clipped=%lu rejected=%lu steals=%lu mixed_peak=%lu\n",
    scene_names[scene],bank_names[bank],audCount,missing,r.clipped,r.rejected,r.steals,r.peak);
   if(negative&&(audMuted<0||missing!=1||audNotes[audMuted].samples||!r.peak||r.sumSq<=0.0)){fclose(csv);return 2;}
   free(r.data);
  }
 }
 fclose(csv);
 if(negative){puts("EXPECTED_NEGATIVE_DETECTED: one muted note is missing while mixed PCM remains nonzero");return 1;}
 printf("%s per-note PCM contribution: notes=%u missing=%u; all8 existing WAVs byte-identical\n",total?"FAIL":"PASS",notes,total);
 return total?1:0;
}
