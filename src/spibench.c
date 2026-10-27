#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>


#define GB 0x18040000
#define SC (1u<<19)
#define MO (1u<<18)
#define DC (1u<<12)
#define CS (1u<<15)//затсывает иногда без цс

static volatile uint32_t *s,*c;
static double tm(){
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC,&t);
  return t.tv_sec+t.tv_nsec*1e-9;
}

static inline void sb(uint8_t d){
  int i;
  for(i=7;i>=0;i--){
    if((d>>i)&1)*s=MO;else *c=MO;
    *s=SC;
    *c=SC;}
}

static void cm(uint8_t d){*c=CS;*c=DC;sb(d);*s=DC;*s=CS;}
static void dt(uint8_t d){*c=CS;sb(d);*s=CS;}

static void win(){
  cm(0x2a);dt(0);dt(0);dt(0);dt(239);
  cm(0x2b);dt(0);dt(0);dt(1);dt(63);
  cm(0x2c);
}

static void fl(uint8_t h,uint8_t l){
  long i;
  win();
  *c=CS;
  for(i=0;i<76800;i++){sb(h);sb(l);}
  *s=CS;
}

int main(int n,char**v){
  int f,i,k;
  long m,j;
  double a,st;
  volatile uint32_t *g;
  uint16_t col[3]={0x07e0,0xf800,0x001f};
  if(n<2){printf("t = store speed, f = fill screen\n");return 1;}
  f=open("/dev/mem",O_RDWR|O_SYNC);
  //////////
  if(f<0){perror("open /dev/mem");return 1;}
  //////////
  g=mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,f,GB);
  if(g==MAP_FAILED){perror("mmap");return 1;}
  s=g+3;
  c=g+4;
  if(v[1][0]=='t'){
    m=20000000;
    a=tm();
    for(j=0;j<m;j++){*s=SC;*c=SC;}
    a=tm()-a;
    st=a/(2.0*m)*1e9;
    printf("store: %.1f ns\n",st);
    printf("2 stores/bit:   %.2f Mbit/s\n",1e3/(2*st));
    printf("2.5 stores/bit: %.2f Mbit/s\n",1e3/(2.5*st));
    printf("3 stores/bit:   %.2f Mbit/s\n",1e3/(3*st));
  }else{
    k=10;
    a=tm();
    for(i=0;i<k;i++)fl(col[i%3]>>8,col[i%3]&255);
    a=tm()-a;
    printf("%d frames %.2f s, %.2f fps, %.2f Mbit/s\n",k,a,k/a,k*76800.0*16/a/1e6);
  }
  return 0;
}
