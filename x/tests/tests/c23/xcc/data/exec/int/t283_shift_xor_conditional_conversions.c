typedef unsigned char u8;
typedef unsigned _BitInt(7) u7;
#define STEP(x) (((x)&0x80u) ? ((u8)((x)<<1)^0x53u):(u8)((x)<<1))
__attribute__((noinline)) u8 f0(u8 x){return STEP(x);}
__attribute__((noinline)) u8 f1(u8 x){return (x&0x80u)?((u8)(x<<1)^0x53u):(unsigned)(u8)(x<<1);}
__attribute__((noinline)) u8 f2(u8 x){return (x&0x80u)?((u8)(x<<1)^0x53u):(long)(u8)(x<<1);}
__attribute__((noinline)) u8 f3(u8 x){return (x&0x80u)?((u8)(x<<1)^0x53u):(int)(signed char)(x<<1);}
__attribute__((noinline)) u8 f4(u8 x){return (x&0x80u)?((u8)(x<<1)^0x53u):(bool)(x<<1);}
__attribute__((noinline)) u8 f5(u8 x){return (x&0x80u)?((u8)(x<<1)^0x53u):(u7)(x<<1);}
__attribute__((noinline)) u8 f6(u8 x){return (x&0x80u)?((u7)(x<<1)^0x53u):(u7)(x<<1);}
__attribute__((noinline)) u7 f7(u8 x){return STEP(x);}
__attribute__((noinline)) bool f8(u8 x){return STEP(x);}
__attribute__((noinline)) u8 f9(u8 x,u8 polynomial){return (x&0x80u)?((u8)(x<<1)^polynomial):(u8)(x<<1);}
__attribute__((noinline)) u8 f10(u8 x){x=STEP(x);return STEP(x);}
__attribute__((noinline)) unsigned f11(u8 x){return (x&0x80u)?((unsigned)(x<<1)^0x53u):(unsigned)(x<<1);}

static volatile u8 input, polynomial;
static u8 reference(u8 x) {
    return (u8)(((unsigned)x*2u) ^ ((x&128u)?83u:0u));
}
int main(void) {
    static const u8 polys[]={0,1,83,128,255};
    for(unsigned x=0;x<256;++x) {
        input=(u8)x;
        u8 expected=reference((u8)x);
        if(f0(input)!=expected || f1(input)!=expected || f2(input)!=expected || f3(input)!=expected) return 1;
        if(f4(input)!=((x&128u)?expected:(x!=0)))return 2;
        if(f5(input)!=((x&128u)?expected:((x*2u)&127u)))return 3;
        if(f6(input)!=(((x*2u)&127u)^((x&128u)?83u:0u)))return 4;
        if((unsigned)f7(input)!=(expected&127u))return 5;
        if(f8(input)!=(expected!=0))return 6;
        if(f10(input)!=reference(expected))return 7;
        if(f11(input)!=((x*2u)^((x&128u)?83u:0u)))return 8;
        for(unsigned p=0;p<5;++p) {
            polynomial=polys[p];
            if(f9(input,polynomial)!=(u8)((x*2u)^((x&128u)?polys[p]:0u)))return 9;
        }
    }
    return 0;
}
