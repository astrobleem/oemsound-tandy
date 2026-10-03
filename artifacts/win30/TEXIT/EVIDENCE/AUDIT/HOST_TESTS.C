static void check(int condition,const char *label) {
 ++check_count;
 if(!condition) { fprintf(stderr,"FAIL: %s\n",label);exit(1); }
}
static void reset(void) {
 began=0;phase=0;active=0;preview=testmode=requestExit=0;
 confirm_result=IDOK;box_calls=exit_calls=exit_result=exit_clean=busy_kind=driver_loaded=0;test_log[0]=0;now_tick=1000;win_flags=0;
 hardware_signature=0x21;timer_ok=class_ok=window_ok=1;
 window_alive=0;timer_calls=kill_calls=destroy_calls=quit_calls=0;
 flags_calls=hardware_reads=register_calls=create_calls=0;
 translate_calls=dispatch_calls=getmessage_calls=0;
 port_count=0;live_timer=0;queue_length=queue_cursor=quit_pending=0;
 memset(port_bytes,0,sizeof(port_bytes));
 memset(port_ticks,0,sizeof(port_ticks));
 memset(&registered_class,0,sizeof(registered_class));
 registered_class.lpfnWndProc=ChimeProc;
}
static int muted(void) {
 return port_count>=4 && port_bytes[port_count-4]==0x9f &&
 port_bytes[port_count-3]==0xbf && port_bytes[port_count-2]==0xdf &&
 port_bytes[port_count-1]==0xff;
}
static void enqueue(DWORD tick,UINT m,WPARAM wp) {
 MSG *msg=&queue_messages[queue_length];
 msg->hwnd=WIN;msg->message=m;msg->wParam=wp;msg->lParam=0;
 queue_ticks[queue_length++]=tick;
}
static void timer_at(DWORD t,WPARAM id){now_tick=t;ChimeProc(WIN,WM_TIMER,id,0);}
static void start(void){window_alive=1;check(ChimeProc(WIN,WM_CREATE,0,0)==0,"create");check(active&&phase==4&&muted(),"silent waiting");ChimeProc(WIN,WM_TIMER,TIMER,0);check(phase==0&&port_count==7,"firstnote");}
static void schedule(void){enqueue(1000,WM_TIMER,TIMER);enqueue(1165,WM_TIMER,TIMER);enqueue(1330,WM_TIMER,TIMER);enqueue(1650,WM_TIMER,TIMER);enqueue(1825,WM_TIMER,TIMER);}
int main(void){int r,n;unsigned i,j;DWORD b;
 reset();confirm_result=2;r=WinMain(WIN,NULL,"",1);check(r==0&&!port_count&&!exit_calls&&!register_calls&&box_calls==1,"confirmation cancel has no PSG/exit side effects");
 reset();r=WinMain(WIN,NULL,"/bad",1);check(r==6&&!port_count&&!exit_calls&&!box_calls,"unknown option rejected");
 reset();win_flags=WF_PMODE;r=WinMain(WIN,NULL,"/go",1);check(r==1&&!port_count&&!exit_calls&&!hardware_reads,"protected mode rejects");
 reset();r=WinMain(WIN,WIN,"/go",1);check(r==1&&!port_count&&!exit_calls&&!flags_calls,"duplicate rejects");
 reset();hardware_signature=0;r=WinMain(WIN,NULL,"/go",1);check(r==2&&!port_count&&!exit_calls,"unknown hardware rejects");
 for(i=1;i<=5;i++){reset();if(i==5)driver_loaded=1;else busy_kind=i;r=WinMain(WIN,NULL,"/testgo",1);check(r==7&&!port_count&&!exit_calls&&!register_calls&&strstr(test_log,"BUSY"),"busy refuses without muting another writer");}
 reset();class_ok=0;r=WinMain(WIN,NULL,"/testgo",1);check(r==3&&!port_count&&!exit_calls,"register fail no exit");
 reset();window_ok=0;r=WinMain(WIN,NULL,"/testgo",1);check(r==4&&muted()&&!exit_calls,"create fail mute no exit");
 reset();timer_ok=0;r=WinMain(WIN,NULL,"/testgo",1);check(r==4&&muted()&&!active&&!live_timer&&!exit_calls,"timer fail no exit");
 reset();schedule();r=WinMain(WIN,NULL,"/testgo",1);check(r==5&&exit_calls==1&&exit_clean&&!active&&!box_calls&&strstr(test_log,"VETO OR FAILURE"),"veto after cleanup returns without resident state or repeat exit");
 reset();schedule();exit_result=1;r=WinMain(WIN,NULL,"/testgo",1);check(r==0&&exit_calls==1&&exit_clean&&strstr(test_log,"RETURN TRUE"),"exit true path");
 reset();schedule();r=WinMain(WIN,NULL,"",1);check(r==5&&box_calls==2&&exit_calls==1&&exit_clean,"normal confirmation then veto report");
 reset();schedule();r=WinMain(WIN,NULL,"/go",1);check(r==5&&box_calls==1&&exit_calls==1,"already confirmed go only reports veto");
 reset();schedule();r=WinMain(WIN,NULL,"/preview",1);check(r==0&&!exit_calls&&!requestExit&&muted()&&!box_calls,"preview cannot request exit");
 reset();schedule();r=WinMain(WIN,NULL,"/testpreview",1);check(r==0&&!exit_calls&&strstr(test_log,"NO EXIT REQUEST"),"testpreview logs no exit");
 reset();enqueue(1000,WM_TIMER,TIMER);enqueue(100000,WM_TIMER,TIMER);r=WinMain(WIN,NULL,"/testgo",1);check(!exit_calls&&!requestExit&&muted(),"grossly late incomplete phrase cannot exit");
 reset();enqueue(1000,WM_TIMER,TIMER);enqueue(1386,WM_TIMER,TIMER);r=WinMain(WIN,NULL,"/testgo",1);check(!exit_calls&&muted()&&strstr(test_log,"LATE ABORT"),"late transition aborts exit");
 reset();enqueue(1000,WM_TIMER,TIMER);enqueue(1100,WM_QUERYENDSESSION,0);enqueue(1150,WM_ENDSESSION,0);r=WinMain(WIN,NULL,"/testgo",1);check(r==0&&!exit_calls&&!active&&!live_timer&&!window_alive&&muted(),"other exit query then cancellation cancels chime without residency");
 for(i=0;i<5;i++){reset();start();for(j=1;j<=i&&j<4;j++)timer_at(j==1?1165:j==2?1330:1650,TIMER);ChimeProc(WIN,WM_CLOSE,0,0);check(!active&&!live_timer&&!requestExit&&muted(),"close each phase no exit");n=port_count;timer_at(99999,TIMER);check(n==port_count,"stale timer after close ignored");}
 reset();start();n=port_count;timer_at(1165,TIMER+1);check(port_count==n&&phase==0,"foreign timer ignored");timer_at(1164,TIMER);check(port_count==n,"first transition not early");timer_at(1165,TIMER);check(phase==1&&port_bytes[n]==0xad&&port_bytes[n+1]==0x11&&port_bytes[n+2]==0xb6,"G4 exact output");timer_at(1330,TIMER);check(phase==2&&port_bytes[port_count-3]==0xc7&&port_bytes[port_count-2]==0x16,"Eb4 exact output");timer_at(1650,TIMER);check(phase==3&&port_bytes[port_count-3]==0x9a,"fade");timer_at(1824,TIMER);check(active&&!requestExit,"completion not early");timer_at(1825,TIMER);check(!active&&requestExit&&muted(),"only completed phrase requests exit");
 reset();b=0xffffff80UL;now_tick=b;start();timer_at(b+165,TIMER);timer_at(b+330,TIMER);timer_at(b+650,TIMER);timer_at(b+825,TIMER);check(requestExit&&muted()&&!active,"DWORD wrap works");
 reset();start();ChimeProc(WIN,WM_ENDSESSION,1,0);check(!requestExit&&!active&&!live_timer&&muted(),"confirmed external session end mutes");
 check(argIs(" /go ","/go")&&!argIs("/go junk","/go")&&!argIs("/gox","/go"),"exact options");
 printf("PASS: %d assertions; host-only mocks of unchanged source.\n",check_count);return 0;
}
