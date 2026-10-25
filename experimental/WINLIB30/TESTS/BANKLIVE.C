/* Actual live reconstruction: mixed latched banks and explicit preference. */
#define LIVE_MAIN inherited_live_main
#include "live_host.c"
#undef main
static unsigned voiceFor(MSYN *s,WORD key)
{
 unsigned i;for(i=0;i<3;i++)if(s->note[i]&&s->key[i]==key+1)return i;return 3;
}
int main(void)
{
 unsigned first,second;
 ck(inherited_live_main()==0,"prior live behavior");
 ck(!LIOpen()&&!LIBank(1),"live Chamber choice");
 ck(!LIMessage(MODM_DATA,0x7f3c90),"Chamber first note");
 first=voiceFor(&synth,60);ck(first<3&&synth.voice[first].bank==1,"seed latches Chamber");
 advance(330);ck(!LIPump(),"age Chamber voice");
 ck(!LIBank(0)&&acBankSelected==0&&liSynth.bank==0,"new notes use Default");
 ck(!LIApply(0x7f4090)&&!LIRebuild(),"rebuild mixed held banks");
 first=voiceFor(&synth,60);second=voiceFor(&synth,64);
 ck(first<3&&second<3&&synth.voice[first].bank==1&&synth.voice[second].bank==0,"live seed retains each voice bank");
 ck(synth.bank==0&&folded.bank==0&&instrumentBank==0,"seed restores explicit selected bank");
 ck(LIBank(2)!=0&&acBankSelected==0&&liSynth.bank==0,"invalid live bank unchanged");
 task=2;ck(LIBank(1)!=0&&acBankSelected==0,"foreign live bank refused");task=1;
 ck(!LIBank(1)&&!LIReset(),"Chamber panic/reset");
 ck(liSynth.bank==1&&synth.bank==1&&folded.bank==1&&instrumentBank==1,"all renderers retain selection through reset");
 ck(!LIClose()&&!lease&&acBankSelected==1,"close retains preference and releases owner");
 ck(!LIOpen()&&synth.bank==1&&liSynth.bank==1,"reopen reapplies preference");
 ck(!LIClose()&&!lease,"final live cleanup");
 printf("PASS actual live bank/source checks=%u\n",tests);return 0;
}
