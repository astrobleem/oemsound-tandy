#include <stdio.h>
#include <stdlib.h>
#include "AUDIO.H"
static FILE *output;
static unsigned port61=0xA4;
int inp(unsigned port) { return port==0x61?port61:0; }
int outp(unsigned port,int value)
{
    if(port==0x61)port61=(unsigned)value;
    fprintf(output,"%u,%u\n",port,(unsigned)value);
    return value;
}
void PsgByte(unsigned value) { fputc(value,output); }
int main(int argc,char **argv)
{
    const AUDIO *audio;
    FILE *input;
    unsigned char raw[14],header[20];
    STATE s;
    unsigned i,count=0;
    if(argc!=4)return 2;
    audio=argv[1][0]=='P'?&buddy_psg:
        (argv[1][0]=='N'?&buddy_silent:&buddy_speaker);
    input=fopen(argv[2],"rb");output=fopen(argv[3],"wb");
    if(!input||!output)return 3;
    if(fread(header,1,20,input)!=20)return 4;
    audio->open();
    while(fread(raw,1,14,input)==14){
        s.when=0;
        for(i=0;i<4;i++){
            s.note[i]=raw[4+i];s.velocity[i]=raw[8+i];
        }
        s.retrigger=raw[12];audio->state(&s);++count;
    }
    audio->mute();fclose(input);fclose(output);
    if(audio_mask||port61!=0xA4)return 5;
    printf("states=%u, mask=%u, port61=%u\n",count,audio_mask,port61);
    return 0;
}
