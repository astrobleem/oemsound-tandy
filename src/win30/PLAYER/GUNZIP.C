#include "GUNZIP.H"
typedef struct {GZ16 count[16],symbol[288];} GZTREE;
/* Linear mode: out is the whole result, capacity bytes. Ring mode (sink
   set): out is a GZ_WINDOW ring flushed GZ_CHUNK bytes at a time, limit
   bounds the total, and the CRC is accumulated while flushing. */
typedef struct {GZREAD read;void *user;GZ32 size,pos,bits;GZ16 nbits,error;
 GZ8 GZ_FAR *out;GZ16 capacity;GZ32 used,limit,crc;GZWRITE sink;void *suser;} GZSTATE;
static int get(GZSTATE *s){GZ8 b;if(s->pos>=s->size||!s->read(s->user,s->pos,&b,1)){s->error=1;return 0;}s->pos++;return b;}
static GZ16 bits(GZSTATE *s,GZ16 n){GZ16 v;while(s->nbits<n&&!s->error){s->bits|=(GZ32)get(s)<<s->nbits;s->nbits+=8;}if(s->error)return 0;v=(GZ16)(s->bits&((1UL<<n)-1));s->bits>>=n;s->nbits-=n;return v;}
static int build(GZTREE *t,GZ8 *lens,GZ16 n){GZ16 offsets[16],i,len;long left=1;
 for(i=0;i<16;i++)t->count[i]=0;for(i=0;i<n;i++){if(lens[i]>15)return 0;t->count[lens[i]]++;}
 if(t->count[0]==n)return 0;
 for(i=1;i<=15;i++){left=left*2-t->count[i];if(left<0)return 0;}
 offsets[1]=0;for(i=1;i<15;i++)offsets[i+1]=offsets[i]+t->count[i];
 for(i=0;i<n;i++)if((len=lens[i])!=0)t->symbol[offsets[len]++]=i;return 1;
}
static int decode(GZSTATE *s,GZTREE *t){GZ16 len,code=0,first=0,index=0,count;
 for(len=1;len<=15&&!s->error;len++){code|=bits(s,1);count=t->count[len];
  if(code>=first&&code-first<count)return t->symbol[index+code-first];index+=count;first=(first+count)<<1;code<<=1;}
 s->error=3;return -1;
}
static GZ32 crcupd(GZ32 crc,const GZ8 GZ_FAR *p,GZ16 n){GZ16 i,k;
 for(i=0;i<n;i++){crc^=p[i];for(k=0;k<8;k++)crc=(crc>>1)^((crc&1)?0xedb88320UL:0);}return crc;}
static void flush(GZSTATE *s,GZ16 n){GZ16 at=(GZ16)((s->used-n)&(GZ_WINDOW-1U));
 s->crc=crcupd(s->crc,s->out+at,n);if(!s->sink(s->suser,s->out+at,n))s->error=5;}
static void emit(GZSTATE *s,GZ8 v){
 if(s->sink){if(s->used>=s->limit){s->error=2;return;}s->out[(GZ16)(s->used&(GZ_WINDOW-1U))]=v;s->used++;
  if(!(s->used&(GZ_CHUNK-1U)))flush(s,GZ_CHUNK);return;}
 if(s->used>=s->capacity){s->error=2;return;}s->out[(GZ16)s->used++]=v;}
static GZ32 word32(GZSTATE *s){GZ32 v=0;GZ16 i;for(i=0;i<4;i++)v|=(GZ32)get(s)<<(8*i);return v;}
static int run(GZSTATE *init,GZ32 size)
{
 static GZ16 lb[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
 static GZ8 le[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
 static GZ16 db[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
 static GZ8 de[30]={0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
 GZSTATE s;GZTREE lit,dist,ct;GZ8 lens[320],cl[19];GZ16 i,n,flags,last,type,nlit,ndist,ncl,len,distance,rep;
 GZ32 crc,want,plain;int sym;
 /* RFC1951 code-length order, set explicitly to avoid signed/near tables. */
 static GZ8 co[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
 s=*init;
 if(get(&s)!=31||get(&s)!=139||get(&s)!=8)return 1;flags=get(&s);if(flags&0xe0)return 1;
 for(i=0;i<6;i++)get(&s);
 if(flags&4){n=get(&s);n|=(GZ16)get(&s)<<8;while(n--&&!s.error)get(&s);}
 if(flags&8){n=0;while(get(&s)&&!s.error)if(++n>1024)return 2;}
 if(flags&16){n=0;while(get(&s)&&!s.error)if(++n>1024)return 2;}
 /* Header CRC is not silently ignored in this bounded profile. */
 if(flags&2)return 1;
 do{
  last=bits(&s,1);type=bits(&s,2);if(s.error)break;
  if(type==0){s.bits=0;s.nbits=0;n=get(&s);n|=(GZ16)get(&s)<<8;len=get(&s);len|=(GZ16)get(&s)<<8;
   if((GZ16)(n^len)!=65535U)return 1;while(n--&&!s.error)emit(&s,(GZ8)get(&s));continue;}
  if(type==3)return 1;
  if(type==1){for(i=0;i<288;i++)lens[i]=(GZ8)(i<144?8:i<256?9:i<280?7:8);if(!build(&lit,lens,288))return 3;
   for(i=0;i<32;i++)lens[i]=5;if(!build(&dist,lens,32))return 3;
  }else{
   nlit=257+bits(&s,5);ndist=1+bits(&s,5);ncl=4+bits(&s,4);if(nlit>286||ndist>32)return 3;
   for(i=0;i<19;i++)cl[i]=0;for(i=0;i<ncl;i++)cl[co[i]]=(GZ8)bits(&s,3);if(!build(&ct,cl,19))return 3;
   n=nlit+ndist;i=0;while(i<n&&!s.error){sym=decode(&s,&ct);if(sym<0)break;
    if(sym<16){lens[i++]=(GZ8)sym;continue;}
    if(sym==16){if(!i)return 3;len=lens[i-1];rep=3+bits(&s,2);}
    else if(sym==17){len=0;rep=3+bits(&s,3);}else if(sym==18){len=0;rep=11+bits(&s,7);}else return 3;
    if(rep>n-i)return 3;while(rep--)lens[i++]=(GZ8)len;
   }
   if(s.error||!lens[256]||!build(&lit,lens,nlit)||!build(&dist,lens+nlit,ndist))return 3;
  }
  while(!s.error){sym=decode(&s,&lit);if(sym<0)break;if(sym==256)break;if(sym<256){emit(&s,(GZ8)sym);continue;}
   if(sym>285)return 3;sym-=257;len=lb[sym]+bits(&s,le[sym]);sym=decode(&s,&dist);if(sym<0||sym>=30)return 3;
   distance=db[sym]+bits(&s,de[sym]);if(!distance||(GZ32)distance>s.used)return 3;
   while(len--&&!s.error)emit(&s,s.out[s.sink?(GZ16)((s.used-distance)&(GZ_WINDOW-1U)):(GZ16)(s.used-distance)]);
  }
 }while(!last&&!s.error);
 if(s.error)return s.error;s.nbits=0;s.bits=0;
 if(s.sink&&(s.used&(GZ_CHUNK-1U)))flush(&s,(GZ16)(s.used&(GZ_CHUNK-1U)));
 if(s.error)return s.error;
 want=word32(&s);plain=word32(&s);if(s.error||s.pos!=size)return 1;
 if(s.sink)crc=s.crc;else crc=crcupd(0xffffffffUL,s.out,(GZ16)s.used);
 if((crc^0xffffffffUL)!=want||plain!=s.used)return 4;init->used=s.used;return 0;
}
int gz_decode(GZREAD read,void *user,GZ32 size,GZ8 GZ_FAR *out,GZ16 cap,GZ16 *used)
{
 GZSTATE s;int r;
 if(!read||!out||!used||!cap||cap>60000U||size<18)return 1;
 s.read=read;s.user=user;s.size=size;s.pos=s.bits=0;s.nbits=s.error=0;s.out=out;s.capacity=cap;
 s.used=s.limit=s.crc=0;s.sink=0;s.suser=0;*used=0;
 r=run(&s,size);if(!r)*used=(GZ16)s.used;return r;
}
int gz_decode_stream(GZREAD read,void *user,GZ32 size,GZ8 GZ_FAR *window,GZWRITE write,void *wuser,GZ32 limit,GZ32 *total)
{
 GZSTATE s;int r;
 if(!read||!window||!write||!total||size<18)return 1;
 s.read=read;s.user=user;s.size=size;s.pos=s.bits=0;s.nbits=s.error=0;s.out=window;s.capacity=0;
 s.used=0;s.limit=limit;s.crc=0xffffffffUL;s.sink=write;s.suser=wuser;*total=0;
 r=run(&s,size);*total=s.used;return r;
}
