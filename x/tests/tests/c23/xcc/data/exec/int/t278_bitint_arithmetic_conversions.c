typedef unsigned _BitInt(9) u9;
typedef _BitInt(9) s9;
typedef unsigned _BitInt(17) u17;
typedef _BitInt(17) s17;
volatile unsigned long input_a, input_b;
volatile unsigned shift_count;
#define UNSIGNED_CHECK(N, MASK) \
typedef unsigned _BitInt(N) u##N; \
__attribute__((noinline)) int check_u##N(u##N a, u##N b, unsigned shift) { \
    unsigned long x=(unsigned long)a, y=(unsigned long)b, mask=MASK; \
    if ((unsigned long)(a+b) != ((x+y)&mask)) return 1; \
    if ((unsigned long)(a-b) != ((x-y)&mask)) return 2; \
    if ((unsigned long)(a*b) != ((x*y)&mask)) return 3; \
    if ((unsigned long)(a<<shift) != ((x<<shift)&mask)) return 4; \
    if ((unsigned long)(a>>shift) != (x>>shift)) return 5; \
    if ((unsigned long)(~a) != ((~x)&mask)) return 6; \
    if ((unsigned long)(-a) != ((0UL-x)&mask)) return 7; \
    if ((a+b==0) != (((x+y)&mask)==0)) return 8; \
    if ((unsigned long)(a&b) != (x&y)) return 9; \
    if ((unsigned long)(a|b) != (x|y)) return 10; \
    if ((unsigned long)(a^b) != (x^y)) return 11; \
    if (b && (unsigned long)(a/b) != x/y) return 12; \
    if (b && (unsigned long)(a%b) != x%y) return 13; \
    u##N copy=a; \
    if ((unsigned long)copy++ != x || (unsigned long)copy != ((x+1)&mask)) return 14; \
    copy=a; if ((unsigned long)++copy != ((x+1)&mask)) return 15; \
    copy=a; if ((unsigned long)copy-- != x || (unsigned long)copy != ((x-1)&mask)) return 16; \
    copy=a; if ((unsigned long)--copy != ((x-1)&mask)) return 17; \
    copy=a; if ((unsigned long)(copy+=b) != ((x+y)&mask)) return 18; \
    copy=a; if ((unsigned long)(copy-=b) != ((x-y)&mask)) return 19; \
    copy=a; if ((unsigned long)(copy*=b) != ((x*y)&mask)) return 20; \
    copy=a; if ((unsigned long)(copy<<=shift) != ((x<<shift)&mask)) return 21; \
    return 0; \
}
UNSIGNED_CHECK(1, 1UL)
UNSIGNED_CHECK(3, 7UL)
UNSIGNED_CHECK(7, 127UL)
UNSIGNED_CHECK(8, 255UL)
UNSIGNED_CHECK(9, 511UL)
UNSIGNED_CHECK(13, 8191UL)
UNSIGNED_CHECK(17, 131071UL)
UNSIGNED_CHECK(23, 8388607UL)
UNSIGNED_CHECK(31, 2147483647UL)

typedef _BitInt(8) s8;
__attribute__((noinline)) int signed_byte_shift(s8 a, unsigned count) {
    return (int)(a >> count);
}
static u9 static_zero=512;
static u9 static_max=-1;
static s9 static_negative=495;
static u9 static_array[]={512,-1,513};
struct values { u9 a; s9 b; };
static struct values static_struct={512,495};
static unsigned words[520];
__attribute__((noinline)) int mixed(u9 a, u17 b, s9 c, s17 d) {
    if ((long)(a+1) != 512L) return 31;
    if ((unsigned long)(b+(unsigned)1) != 0UL) return 32;
    if ((long)(c+d) != 39983L) return 33;
    if ((long)(c+(unsigned)40000) != 39983L) return 34;
    u9 local_zero=512, local_max=-1;
    s9 local_negative=495;
    if ((unsigned)local_zero || (unsigned)local_max!=511 || (int)local_negative!=-17) return 35;
    if ((unsigned)static_zero || (unsigned)static_max!=511 || (int)static_negative!=-17) return 36;
    if ((unsigned)static_array[0] || (unsigned)static_array[1]!=511 || (unsigned)static_array[2]!=1) return 37;
    if ((unsigned)static_struct.a || (int)static_struct.b!=-17) return 38;
    if ((long)(-c)!=17L || (long)(~c)!=16L) return 39;
    if ((long)(c*(s9)2)!=-34L || (long)(c/(s9)2)!=-8L || (long)(c%(s9)2)!=-1L) return 40;
    if ((long)(c>>(unsigned long)1)!=-9L) return 41;
    unsigned *p=words;
    if (p+a != &words[511] || a+p != &words[511]) return 42;
    p+=a; if (p!=&words[511]) return 43;
    p-=a; if (p!=words) return 44;
    words[a]=1234; if (*(words+a)!=1234 || words[511]!=1234) return 45;
    if (&words[a] != &words[511]) return 46;
    words[a]++; if (words[511]!=1235) return 47;
    return 0;
}
#define RUN(N, A, B, S) do { input_a=A; input_b=B; shift_count=S; int r=check_u##N((u##N)input_a,(u##N)input_b,shift_count); if(r) return (N*7+r); } while(0)
int main(void) {
    RUN(1,1,1,0); RUN(1,0,1,0);
    RUN(3,7,1,1); RUN(3,0,3,2); RUN(3,5,2,1);
    RUN(8,255,1,0); RUN(8,0,3,7); RUN(8,131,2,3);
    RUN(7,127,1,1); RUN(7,0,3,6); RUN(7,67,2,3);
    RUN(9,511,1,1); RUN(9,0,3,8); RUN(9,257,2,4);
    RUN(13,8191,1,1); RUN(13,0,3,12); RUN(13,4097,2,6);
    RUN(17,131071UL,1,1); RUN(17,0,3,16); RUN(17,65537UL,2,8);
    RUN(23,8388607UL,1,1); RUN(23,0,3,22); RUN(23,4194305UL,2,11);
    RUN(31,2147483647UL,1,1); RUN(31,0,3,30); RUN(31,1073741825UL,2,15);
    shift_count=0;
    if (signed_byte_shift((s8)-127,shift_count)!=-127) return 50;
    shift_count=1;
    if (signed_byte_shift((s8)-127,shift_count)!=-64) return 51;
    input_a=511; input_b=131071UL;
    return mixed((u9)input_a,(u17)input_b,(s9)-17,(s17)40000L);
}
