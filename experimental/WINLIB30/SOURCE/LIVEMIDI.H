/* Additive app-local live input adapter. Pure state/rendering in foreground.
   Existing MD generation flush/seed/rearm retains the sole SOUND/PIT owner.
   Age remains55ms Stage5 units; an input rebuild reanchors its next envelope
   tick by at most55ms. This is not an8ms key-dispatch latency promise. */
#include "APPCLK.H"
#include "MIDISYN.H"
typedef WORD(FAR PASCAL *LIBANKSEED)(WORD,DWORD,DWORD,DWORD,WORD);
typedef WORD(FAR PASCAL *LISEED)(WORD,DWORD,DWORD,DWORD);
static ACRESET liBeginFn;static MSYN liSynth;static WORD liFlags,liVelocity[4];static DWORD liNow;static LISEED liSeedFn;static LIBANKSEED liBankSeedFn;
static WORD LIOpen(void){if(ACOpen())return 1;if(acVoiceFn(acHandle,liFlags)){ACClose();return 1;}MSReset(&liSynth);MSBank(&liSynth,acBankSelected);liSynth.flags=liFlags;liNow=0;return 0;}
static WORD LIClose(void){WORD r=ACClose();if(!r){MSReset(&liSynth);MSBank(&liSynth,acBankSelected);liSynth.flags=liFlags;}return r;}
static WORD LIReset(void){WORD r=ACReset();if(!r){MSReset(&liSynth);MSBank(&liSynth,acBankSelected);liSynth.flags=liFlags;liNow=0;}return r;}
static WORD LIBank(WORD bank){if(ACBank(bank))return 1;return MSBank(&liSynth,bank);}
static WORD LIApply(DWORD value){WORD i,age[3],noise=(WORD)value&15;DWORD b=(value>>16)&127;
 if(!acHandle||ACClock())return 1;liNow=acClockValue;for(i=0;i<3;i++)age[i]=liSynth.age[i];
 if(MSMessage(&liSynth,liNow,value))return 1;for(i=0;i<3;i++)if(age[i]!=liSynth.age[i])liVelocity[i]=(WORD)b;
 if(noise==9&&((WORD)value&240)==0x90&&b)liVelocity[3]=(WORD)b;return 0;}
static WORD LIRebuild(void){WORD order[4],i,j,t,ch,note;DWORD at[4],msg,age;
 if(liBeginFn(acHandle,liNow)||acStatusFn(acHandle,&acInfo,sizeof(acInfo)))return 1;acGen=acInfo.generation;acStarted=acSealed=0;acWatermark=liNow;
 for(i=0;i<4;i++){age=liNow/55UL-liSynth.voice[i].began;at[i]=liNow-age*55UL;order[i]=i;}
 for(i=0;i<4;i++)for(j=i+1;j<4;j++)if(at[order[j]]<at[order[i]]||(at[order[j]]==at[order[i]]&&order[j]<3&&order[i]<3&&liSynth.age[order[j]]<liSynth.age[order[i]])){t=order[i];order[i]=order[j];order[j]=t;}
 for(j=0;j<4;j++){i=order[j];note=i<3?liSynth.note[i]:liSynth.noiseNote;if(!note)continue;ch=i<3?liSynth.channel[i]:9;
  if(i<3&&liSeedFn(acHandle,acGen,at[i],0xc0|ch|((DWORD)(liSynth.voice[i].preset<<4)<<8)))return 1;
  msg=0x90|ch|((DWORD)note<<8)|((DWORD)liVelocity[i]<<16);if(i<3){if(liBankSeedFn(acHandle,acGen,at[i],msg,liSynth.voice[i].bank))return 1;}else if(liSeedFn(acHandle,acGen,at[i],msg))return 1;}
 for(i=0;i<16;i++)if(liSeedFn(acHandle,acGen,liNow,0xc0|i|((DWORD)liSynth.program[i]<<8)))return 1;
 if(ACPush(liNow,0xc0|((DWORD)liSynth.program[0]<<8)))return 1;return ACPublish(liNow+1000UL,0);}
static WORD LIPump(void){if(!acHandle)return 0;if(ACClock())return 1;liNow=acClockValue;MSRender(&liSynth,liNow);if(!acStarted)return 0;return ACPublish(liNow+1000UL,0);}
static DWORD LIMessage(WORD m,DWORD value){if(m==MODM_RESET)return LIReset();if(m==MODM_CLOSE)return LIClose();if(m!=MODM_DATA)return 1;if(LIApply(value))return 1;return LIRebuild();}
static WORD FAR PASCAL LIDebug(WORD q){WORD i,r=0;if(q==1)return acHandle!=0;if(!acHandle)return 0;if(q==0){for(i=0;i<3;i++)if(liSynth.note[i])r|=1<<i;if(liSynth.noiseNote)r|=8;return r;}if(q>=4&&q<=6)return liSynth.note[q-4];if(q>=12&&q<=14)return liSynth.volume[q-12];return 0;}
static int LIBind(HINSTANCE mod,LPSTR cmd){liFlags=(!lstrcmp(cmd,"/expr")||!lstrcmp(cmd,"/exprtest"))?3:0;liSeedFn=(LISEED)GetProcAddress(mod,(LPSTR)12L);liBeginFn=(ACRESET)GetProcAddress(mod,(LPSTR)18L);liBankSeedFn=(LIBANKSEED)GetProcAddress(mod,(LPSTR)20L);return liSeedFn&&liBeginFn&&liBankSeedFn&&ACBind(mod);}
