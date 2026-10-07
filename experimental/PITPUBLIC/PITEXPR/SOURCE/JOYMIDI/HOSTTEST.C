/* Independent explicit event oracle through raw capture decode + mapping. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "JMCORE.H"
static unsigned checks,events;static JM_STATE s;static JM_OUT out;static JOY_CAL c;static JOY_SAMPLE p;static unsigned long now;static unsigned char raw[512];
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#v);exit(1);}}while(0)
static void burst(unsigned x,unsigned y,unsigned b){unsigned i;for(i=0;i<512;i++){raw[i]=(unsigned char)((~b&15)<<4);if(i<x)raw[i]|=5;if(i<y)raw[i]|=10;}joy_decode(raw,512,&p);}
static void run(int x,int y,unsigned b){int nx=0,ny=0,valid;burst(50+(unsigned)x*2,50+(unsigned)y*2,b);valid=jm_ready(&c,&p,0,&nx,&ny);now=(now+55UL)&0xffffffffUL;jm_tick(&s,valid,nx,ny,b,now,&out);CHECK(out.count<=JM_MAX_MSG);events+=out.count;}
static void repeat(int x,int y,unsigned b,unsigned count){while(count--){run(x,y,b);CHECK(out.count==0);}}
static void event(unsigned i,unsigned st,unsigned a,unsigned b){CHECK(i<out.count);CHECK(out.msg[i].status==st&&out.msg[i].a==a&&out.msg[i].b==b);}
static void init(void){unsigned i;joy_reset(&c);burst(50,50,0);joy_track(&c,&p);burst(250,250,0);joy_track(&c,&p);burst(150,150,0);joy_center(&c,&p);for(i=0;i<4;i++){CHECK(c.low[i]==50&&c.high[i]==250&&c.center[i]==150);}jm_init(&s);now=0;}
static void arm(int x,int y){jm_arm(&s,x,y,now);CHECK(s.armed&&!s.sounding);}
static void press(int x,int y){run(x,y,1);CHECK(out.count==0);run(x,y,1);CHECK(out.count==1);}
static void stopOracle(int sounding){jm_stop(&s,&out);CHECK(out.stopped&&!s.armed&&!s.sounding);CHECK(out.count==(unsigned)(sounding?3:2));if(sounding)event(0,0x80,(unsigned)sounding,0);event(sounding?1:0,0xb0,123,0);event(sounding?2:1,0xb0,120,0);}
int main(void){int x,y,i;unsigned a,seed=111,notes=0,previous=0;unsigned long last=0;
 init();CHECK(jm_ready(&c,&p,0,&x,&y)&&x==50&&y==50);CHECK(jm_ready(&c,&p,1,&x,&y));CHECK(!jm_ready(&c,&p,2,&x,&y));
 /* Uncalibrated / partial / invalid / narrow spans never arm-ready. */
 {JOY_CAL bad=c;bad.centered=0;CHECK(!jm_ready(&bad,&p,0,&x,&y));bad=c;bad.high[0]=157;CHECK(!jm_ready(&bad,&p,0,&x,&y));bad=c;bad.low[0]=143;CHECK(!jm_ready(&bad,&p,0,&x,&y));bad=c;bad.high[0]=149;CHECK(!jm_ready(&bad,&p,0,&x,&y));}
 burst(0,150,0);CHECK(!jm_ready(&c,&p,0,&x,&y));burst(512,150,0);CHECK(!jm_ready(&c,&p,0,&x,&y));burst(150,150,0);CHECK(jm_ready(&c,&p,0,&x,&y));
 repeat(50,50,1,100);CHECK(!s.armed&&events==0);arm(50,50);repeat(50,50,1,12);CHECK(s.releaseGate&&!s.sounding);
 run(50,50,0);CHECK(out.count==0&&!s.releaseGate);press(50,50);event(0,0x90,66,72);
 repeat(50,0,1,8);CHECK(s.sounding==66&&s.velocity==127); /* Y alone no retrigger. */
 for(i=0;i<100;i++){run(i&1?53:47,50,1);CHECK(out.count==0&&s.sounding==66);} /* near upper/lower boundaries */
 run(100,0,1);CHECK(out.count==0);run(100,0,1);CHECK(out.count==0);run(100,0,1);CHECK(out.count==2);event(0,0x80,66,0);event(1,0x90,72,127);
 run(0,100,1);CHECK(out.count==0);run(0,100,1);CHECK(out.count==0);run(0,100,1);CHECK(out.count==2);event(0,0x80,72,0);event(1,0x90,60,16);
 run(0,100,0);CHECK(out.count==1);event(0,0x80,60,0);CHECK(!s.sounding); /* no release rate gate */
 repeat(0,100,0,3);press(0,100);event(0,0x90,60,16);stopOracle(60);jm_stop(&s,&out);CHECK(out.count==0);
 /* Raw timeout and early input close; recovered samples never auto-arm. */
 arm(50,50);run(50,50,0);press(50,50);burst(512,150,1);now+=55;jm_tick(&s,jm_ready(&c,&p,0,&x,&y),x,y,1,now,&out);CHECK(out.stopped&&out.count==3&&!s.armed);event(0,0x80,66,0);event(1,0xb0,123,0);event(2,0xb0,120,0);repeat(50,50,1,30);
 arm(50,50);run(50,50,0);press(50,50);burst(0,150,1);now+=55;jm_tick(&s,jm_ready(&c,&p,0,&x,&y),x,y,1,now,&out);CHECK(out.stopped&&out.count==3&&!s.armed);
 arm(50,50);run(50,50,0);press(50,50);run(50,50,3);CHECK(out.stopped&&out.count==3&&!s.armed); /* B2 panic */
 arm(50,50);run(50,50,0);press(50,50);now+=501;jm_tick(&s,1,50,50,1,now,&out);CHECK(out.stopped&&out.count==3&&!s.armed);
 /* Unsigned 32-bit wrap is safe, and 500 exactly remains accepted. */
 now=0xfffffff0UL;arm(50,50);run(50,50,0);press(50,50);CHECK(s.sounding==66);now=(now+500UL)&0xffffffffUL;jm_tick(&s,1,50,50,1,now,&out);CHECK(!out.stopped&&s.armed);stopOracle(66);
 /* Press bounce cannot sound, release remains immediate. */
 init();arm(50,50);run(50,50,0);for(i=0;i<100;i++){run(50,50,1);CHECK(out.count==0);run(50,50,0);CHECK(out.count==0);}press(50,50);event(0,0x90,66,72);run(50,50,0);CHECK(out.count==1);event(0,0x80,66,0);
 /* Every coordinate endpoint maps to the stated bounded scale/velocity. */
 for(i=0;i<=100;i++){jm_init(&s);arm(i,i);run(i,i,0);press(i,i);CHECK(s.note==60+i*13/101);CHECK(s.velocity>=16&&s.velocity<=127);CHECK(s.sounding>=60&&s.sounding<=72);stopOracle(s.sounding);}
 /* 50k pseudo-random input ticks: exact monophonic pairing, note-on rate,
    bounded messages, no orphan NoteOff and supported status/data only. */
 init();arm(50,50);run(50,50,0);
 for(i=0;i<50000;i++){
  seed=(seed*25173u+13849u)&65535u;x=(int)(seed%101);seed=(seed*25173u+13849u)&65535u;y=(int)(seed%101);
  run(x,y,(seed&31)?1:0);
  for(a=0;a<out.count;a++){
   CHECK(out.msg[a].status==0x80||out.msg[a].status==0x90);CHECK(out.msg[a].a>=60&&out.msg[a].a<=72);CHECK(out.msg[a].b<=127);
   if(out.msg[a].status==0x90){CHECK(!previous);CHECK(!notes||((now-last)&0xffffffffUL)>=JM_NOTE_GAP);previous=out.msg[a].a;last=now;notes++;}
   else{CHECK(previous==out.msg[a].a);previous=0;}
  }
  CHECK(previous==(unsigned)s.sounding);
 }
 printf("PASS %u checks; %u randomized NoteOns; raw-decode-to-MIDI exact event oracle, jitter, bounds, rate, release, panic, dropout, rearm, stall and wrap.\n",checks,notes);return 0;
}
