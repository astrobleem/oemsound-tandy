#include "RAWFILE.H"
extern void Check(char *,int);
static RFSong s;static RF8 data[1024],events[RF_EVENTS];
static RF16 len;
static void base(void){RF16 i;for(i=0;i<1024;i++)data[i]=0;
 data[0]='T';data[1]='N';data[2]='D';data[3]='Y';data[4]=2;data[5]=4;data[6]=1;data[8]=1;data[9]=3;data[10]=50;
 data[14]=0x14;data[15]=0x20;data[16]=0;data[17]=255;data[18]=63;
 data[19]=255;data[20]=64;data[21]=255;data[22]=64;data[23]=255;data[24]=64;len=25;}
static RF16 load(void){return RFLoad(&s,data,len,events,1);}
void CoreTests(void)
{
 RF16 i,n,r;RF32 sum;
 base();Check("TND_valid",!load());Check("initial_silence",s.volume[0]==15);
 Check("row_start",RFStep(&s)&&s.period[0]==426&&s.volume[0]==2);
 Check("tick1",RFStep(&s)&&s.row==1&&s.tick==1);
 Check("tick2",RFStep(&s)&&s.row==1&&s.tick==0);
 for(i=3;i<192;i++)RFStep(&s);Check("full_last_row_duration",s.ticks==192);Check("play_once_EOF",!RFStep(&s));
 RFReset(&s);Check("reset_restarts",RFStep(&s)&&s.row==1&&s.period[0]==426);
 base();data[4]=1;Check("reject_TND_v1",load()==2);
 base();data[5]=5;Check("reject_PC_speaker_fifth",load()==2);
 base();data[8]=17;Check("reject_pattern_bound",load()==2);
 base();data[9]=0;Check("reject_zero_speed",load()==2);
 base();data[10]=0;Check("reject_18Hz_special_profile",load()==2);
 base();data[7]=1;Check("reject_loop_outside_order",load()==1);
 base();data[13]=1;Check("reject_order_outside_patterns",load()==1);
 base();data[18]=64;Check("reject_row_run_overflow",load()==1);
 base();data[18]=0;Check("reject_zero_run",load()==1);
 base();data[14]=0x10;Check("reject_note_octave0",load()==2);
 base();data[14]=0xfa;Check("reject_note15",load()==2);
 base();data[15]=0x26;Check("reject_unknown_FX",load()==2);
 base();data[15]=0x2b;data[16]=1;Check("reject_jump_outside_song",load()==1);
 base();data[15]=0x2d;data[16]=64;Check("reject_break_row64",load()==1);
 base();data[15]=0x2f;Check("reject_F00",load()==2);
 base();data[14]=0xc9;data[16]=0xff;Check("reject_arp_overflow",load()==2);
 base();data[len++]=0;Check("reject_trailing_bytes",load()==1);
 base();for(n=0;n<len;n++)Check("reject_every_truncation",RFLoad(&s,data,n,events,1)!=0);
 base();load();Check("elapsed_55ms_two_ticks",RFAdvance(&s,55)&&s.ticks==2&&s.debt==15000UL);
 Check("elapsed_remainder_preserved",RFAdvance(&s,5)&&s.ticks==3&&s.debt==0);
 Check("long_gap_fails_muted",!RFAdvance(&s,501)&&s.error==3&&s.volume[0]==15);
 base();data[15]=0x21;data[16]=4;load();RFStep(&s);Check("porta_up_in_Hz",s.freq[0]==266&&s.period[0]==420);
 base();data[15]=0x22;data[16]=4;load();RFStep(&s);Check("porta_down_in_Hz",s.freq[0]==258&&s.period[0]==433);
 base();data[15]=0x2a;data[16]=0x84;load();RFStep(&s);Check("finetune_add_Hz",s.freq[0]==266);
 base();data[15]=0x2a;data[16]=4;load();RFStep(&s);Check("finetune_sub_Hz",s.freq[0]==258);
 base();data[15]=0x24;data[16]=5;load();RFStep(&s);Check("volume_fraction_step",s.volume[0]==3);
 base();data[15]=0x24;data[16]=0x50;load();RFStep(&s);Check("volume_up",s.volume[0]==1);
 base();data[15]=0x2f;data[16]=31;load();RFStep(&s);Check("F1F_changes_tempo",s.hz==31&&s.speed==3);
 base();data[15]=0x2f;data[16]=2;load();RFStep(&s);Check("F02_changes_speed",s.speed==2&&s.tick==1);
 base();data[15]=0x2b;data[16]=0;load();for(i=0;i<6;i++)RFStep(&s);Check("B00_jump_bounded",s.position==0&&s.row==0);
 base();data[15]=0x2d;data[16]=4;load();RFStep(&s);Check("pattern_break_next_order",s.position==1&&s.row==4);
 base();data[14]=0xe0;data[15]=0x21;data[16]=3;load();s.freq[0]=262;s.volume[0]=5;RFStep(&s);Check("effect_only_keeps_volume",s.volume[0]==5&&s.freq[0]==265);
 base();data[14]=0xd0;load();s.volume[0]=2;RFStep(&s);Check("keyoff_mutes",s.volume[0]==15);
 base();for(i=len;i>13;i--)data[i+3]=data[i-1];len+=4;
 data[11]=4;data[13]=0;data[14]=1;data[15]=255;data[16]=145;
 Check("native_description_401_padding",!load());data[16]=146;Check("reject_description_402",load()==1);
 /* A second explicit event followed by a run: distinguish volume zero
    from absence and exercise slide target memory and fractional pluck decay. */
 base();data[9]=1;data[17]=0;data[18]=0;data[19]=0;data[20]=255;data[21]=62;
 data[22]=255;data[23]=64;data[24]=255;data[25]=64;data[26]=255;data[27]=64;len=28;
 Check("explicit_zero_volume_valid",!load());RFStep(&s);RFStep(&s);Check("explicit_zero_volume_applied",s.volume[0]==0);
 RFStep(&s);Check("empty_row_preserves_volume",s.volume[0]==0);
 data[17]=0x54;data[18]=0x23;data[19]=4;load();RFStep(&s);RFStep(&s);
 Check("tone_slide_target",s.target[0]==330&&s.freq[0]==266);
 data[17]=0xe0;data[18]=0x05;data[19]=5;load();RFStep(&s);s.target[0]=330;RFStep(&s);
 Check("combined_slide_keeps_target",s.target[0]==330&&s.freq[0]==267&&s.volume[0]==3);
 base();data[15]=0x20;data[16]=0x37;load();RFStep(&s);Check("arpeggio_first_offset",s.period[0]==359);RFStep(&s);Check("arpeggio_second_offset",s.period[0]==285);RFStep(&s);Check("arpeggio_returns_base",s.period[0]==426);
 base();load();s.ticks=108000UL;Check("tick_budget_mutes",!RFStep(&s)&&s.error==3&&s.volume[0]==15);
 base();data[23]=0x95;data[24]=255;data[25]=63;len=26;load();RFStep(&s);
 Check("noise_white_tone3_mode",s.noise==7&&s.volume[3]==5);
 RFStep(&s);RFStep(&s);RFStep(&s);Check("empty_noise_row_sustains",s.noise==7&&s.volume[3]==5);
 /* Build independent AGI four-track period/attenuation fixture. */
 for(i=0;i<4;i++){n=8+i*7;data[i*2]=(RF8)n;data[i*2+1]=0;data[n]=3;data[n+1]=0;data[n+2]=0x1a;data[n+3]=0xb;data[n+4]=0x92;data[n+5]=data[n+6]=255;}
 len=36;r=RFLoad(&s,data,len,events,2);Check("AGI_valid",!r);RFStep(&s);
 Check("AGI_exact_divider",s.period[0]==427);Check("AGI_attenuation",s.volume[0]==2);Check("AGI_noise_bits",s.noise==3);
 RFStep(&s);RFStep(&s);Check("AGI_duration_three_ticks",!s.done[0]);Check("AGI_EOF_after_duration",!RFStep(&s)&&s.volume[0]==15);
 RFReset(&s);Check("AGI_restart",RFStep(&s)&&s.period[0]==427);
 data[0]=9;Check("AGI_reject_bad_first_offset",RFLoad(&s,data,len,events,2)!=0);data[0]=8;
 data[2]=8;Check("AGI_reject_overlap",RFLoad(&s,data,len,events,2)!=0);data[2]=15;
 data[8]=0;Check("AGI_reject_duration_zero",RFLoad(&s,data,len,events,2)!=0);data[8]=3;
 data[13]=0;Check("AGI_reject_no_terminator",RFLoad(&s,data,len,events,2)!=0);data[13]=255;
 for(n=0;n<len;n++)Check("AGI_reject_every_truncation",RFLoad(&s,data,n,events,2)!=0);
 /* Deterministic malformed-input mutation sweep; termination and guards checked
    by ASan/UBSan in host runner, no waveform/fidelity claim. */
 sum=1;for(i=0;i<2000;i++){base();sum=sum*1664525UL+1013904223UL;n=(RF16)(sum%len);data[n]^=(RF8)(sum>>24);r=load();if(!r){for(n=0;n<8;n++)if(!RFStep(&s))break;}}
 Check("mutation_sweep_completed",1);
}
