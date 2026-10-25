/* Actual MIDICLK bank/reset/seed state; raw hardware is a host mock. */
#define main inherited_md_host_main
#include "mdhost/driver.c"
#undef main
static WORD pairLive(TCXPAIR FAR *p){(void)p;return 0;}
int main(void)
{
 WORD h;DWORD gen;MSYN a,b;
 ck(MDOpen(&h)==MD_OK,"bank owner open");
 ck(instrumentBank==0&&synth.bank==0&&folded.bank==0,"fresh Default both");
 /* Paired hardware selection is mocked; only actual API/render state is tested. */
 paired=1;rpairlive=pairLive;
 ck(MDInstrumentBank(h,1)==MD_OK,"select Chamber");
 ck(instrumentBank==1&&synth.bank==1&&folded.bank==1,"selected both");
 a=synth;b=folded;ck(MDInstrumentBank(h,2)==MD_BAD,"invalid bank refused");
 ck(!memcmp(&a,&synth,sizeof(a))&&!memcmp(&b,&folded,sizeof(b))&&instrumentBank==1,"invalid bank no mutation");
 task=2;ck(MDInstrumentBank(h,0)==MD_BUSY,"foreign bank refused");task=1;
 ck(instrumentBank==1,"foreign selection unchanged");
 ck(MDReset(h,220)==MD_OK,"reset selected");
 ck(synth.bank==1&&folded.bank==1&&instrumentBank==1,"reset keeps selection both");
 gen=info.generation;
 ck(MDSeedBank(h,gen,110,0x7f3c90,0)==MD_OK,"seed old Default voice");
 ck(synth.voice[0].bank==0&&folded.voice[0].bank==0,"per-voice Default both");
 ck(synth.bank==1&&folded.bank==1&&instrumentBank==1,"seed restores explicit Chamber");
 ck(MDSeedBank(h,gen,165,0x7f4091,1)==MD_OK,"seed Chamber voice");
 ck(synth.voice[0].bank==0&&synth.voice[1].bank==1&&folded.voice[1].bank==1,"mixed held banks survive");
 ck(MDInstrumentBank(h,0)==MD_OK,"select Default for future notes");
 ck(synth.voice[1].bank==1&&folded.voice[1].bank==1,"selection does not recolour held voices");
 a=synth;b=folded;ck(MDSeedBank(h,gen,220,0x7f4392,99)==MD_BAD,"invalid seed bank refused");
 ck(!memcmp(&a,&synth,sizeof(a))&&!memcmp(&b,&folded,sizeof(b))&&instrumentBank==0,"invalid seed bank atomic");
 ck(MDSeedBank(h,gen-1,220,0x7f4392,1)==MD_STALE,"stale bank seed");
 ck(MDBeginLive(h,500)==MD_OK,"live generation reset");
 ck(synth.bank==0&&folded.bank==0&&instrumentBank==0,"live reset keeps chosen Default");
 ck(MDInstrumentBank(h,1)==MD_OK&&MDBeginLive(h,660)==MD_OK,"Chamber live generation");
 ck(synth.bank==1&&folded.bank==1&&instrumentBank==1,"live reset keeps Chamber both");
 ck(MDSeed(h,info.generation,660,0x7f3c90)==MD_OK,"ordinary seed uses selection");
 ck(synth.voice[0].bank==1&&folded.voice[0].bank==1,"ordinary seed Chamber both");
 ck(MDClose(h)==MD_OK&&!lease,"bank owner cleanup");
 ck(MDOpen(&h)==MD_OK&&instrumentBank==0&&synth.bank==0&&folded.bank==0,"new owner starts Default");
 ck(MDClose(h)==MD_OK,"new owner cleanup");
 printf("PASS actual MIDICLK bank state checks=%u\n",tests);return 0;
}
