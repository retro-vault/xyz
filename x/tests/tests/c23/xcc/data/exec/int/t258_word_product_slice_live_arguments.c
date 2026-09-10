typedef unsigned int u16;
typedef unsigned long u32;
#define NOINLINE __attribute__((noinline))
volatile u16 count;
volatile u16 input;
volatile u32 trace;
NOINLINE void clobber(void) {
    count = (u16)((count + 1u) & 0xffffu);
    trace = (u32)input * 0x271828UL;
}
NOINLINE u16 after_call(u16 a, u16 b) {
    clobber();
    return (u16)((((u32)a * (u32)b) >> 8) & 0xffffUL);
}
NOINLINE u16 after_branch(u16 a, u16 b, u16 flag) {
    if (flag) clobber();
    else count = 0x7654u;
    return (u16)((((u32)a * (u32)b) >> 8) & 0xffffUL);
}
NOINLINE u16 after_loop(u16 a, u16 b, u16 n) {
    while (n--) clobber();
    return (u16)((((u32)a * (u32)b) >> 8) & 0xffffUL);
}
NOINLINE u16 two_windows(u16 a, u16 b) {
    u16 x = (u16)((((u32)a * (u32)b) >> 8) & 0xffffUL);
    clobber();
    return (u16)((x ^ (u16)(((u32)a * (u32)b) >> 16)) & 0xffffu);
}
NOINLINE u16 from_memory(const u16 *a, const u16 *b) {
    clobber();
    return (u16)((((u32)*a * (u32)*b) >> 8) & 0xffffUL);
}
int main(void) {
    u16 a = 0xabcdu, b = 0xef01u;
    input = 0x4321u;
    count = 0;
    if (after_call(a,b) != 25870u || count != 1) return 1;
    if (after_branch(a,b,1) != 25870u || count != 2) return 2;
    if (after_branch(a,b,0) != 25870u || count != 0x7654u) return 3;
    count = 0;
    if (after_loop(a,b,7) != 25870u || count != 7) return 4;
    if (two_windows(a,b) != 50539u || count != 8) return 5;
    if (from_memory(&a,&b) != 25870u || count != 9) return 6;
    return 0;
}
