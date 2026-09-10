#include <stdlib.h>
#include <stdint.h>

static volatile unsigned maximum=65535u;
static volatile unsigned counts[]={0,1,2,256,257,65535u};
static unsigned short arena[512];
static heap_t private_heap;

int main(void) {
    if(malloc(0)!=0 || calloc(0,maximum)!=0 || calloc(maximum,0)!=0) return 1;
    if(malloc(maximum)!=0) return 2;
    if(calloc(counts[1],maximum)!=0 || calloc(maximum,counts[1])!=0) return 3;
    if(calloc(counts[3],counts[3])!=0 || calloc(counts[2],32768u)!=0) return 4;
    if(calloc(counts[4],255u)!=0 || calloc(maximum,maximum)!=0) return 5;
    for(unsigned n=1;n<520;n=n*2+1) {
        unsigned char *p=malloc(n);
        if(!p || ((uintptr_t)p&1)) return 6;
        for(unsigned i=0;i<n;++i)p[i]=0xa5;
        free(p);
        p=calloc(1,n);
        if(!p || ((uintptr_t)p&1)) return 7;
        for(unsigned i=0;i<n;++i)if(p[i])return 8;
        for(unsigned i=0;i<n;++i)p[i]=(unsigned char)(i*17+3);
        if(realloc(p,maximum)!=0)return 9;
        for(unsigned i=0;i<n;++i)if(p[i]!=(unsigned char)(i*17+3))return 10;
        unsigned char *q=realloc(p,n+16);
        if(!q)return 11;
        for(unsigned i=0;i<n;++i)if(q[i]!=(unsigned char)(i*17+3))return 12;
        free(q);
    }
    unsigned char *p=calloc(73,7);
    if(!p)return 13;
    for(unsigned i=0;i<511;++i)if(p[i])return 14;
    free(p);
    p=aligned_alloc(32,64);
    if(!p || ((uintptr_t)p&31))return 15;
    for(unsigned i=0;i<64;++i)p[i]=(unsigned char)i;
    if(realloc(p,maximum)!=0)return 16;
    for(unsigned i=0;i<64;++i)if(p[i]!=(unsigned char)i)return 17;
    free(p);
    heap_init_arena(&private_heap,arena,arena+512);
    if(allocate(&private_heap,maximum)!=0)return 18;
    p=allocate(&private_heap,101);
    if(p!=(unsigned char *)arena+8)return 19;
    free(p);
    p=allocate(&private_heap,101);
    if(!p)return 20;
    deallocate(&private_heap,p);
    heap_init_arena(&private_heap,(unsigned char *)arena+1,arena+512);
    p=allocate(&private_heap,101);
    if(p!=(unsigned char *)arena+9)return 21;
    free(p);
    heap_init_arena(&private_heap,(unsigned char *)arena+1,(unsigned char *)arena+8);
    if(allocate(&private_heap,1)!=0)return 22;
    return 0;
}
