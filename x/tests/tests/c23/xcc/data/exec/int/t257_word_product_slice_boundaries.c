typedef unsigned int u16;
typedef unsigned long u32;
#define NOINLINE __attribute__((noinline))
static volatile u16 snapshot;
static volatile u32 wide_snapshot;
static u16 data[5] = {0x1234u,0xffedu,0x00ffu,0x8001u,0x4321u};

NOINLINE u16 mixed_signed(int a, u16 b) {
    return (u16)((((long)a * (long)b) >> 8) & 0xffffL);
}
NOINLINE u16 signed_inputs(int a, int b) {
    return (u16)((((long)a * (long)b) >> 8) & 0xffffL);
}
NOINLINE u16 wider_input(u32 a, u16 b) {
    return (u16)((((a * (u32)b) & 0xffffffffUL) >> 8) & 0xffffUL);
}
NOINLINE u16 zero_extend_signed_bits(int a, u16 b) {
    u16 bits = (u16)(a & 0xffffu);
    return (u16)((((u32)bits * (u32)b) >> 8) & 0xffffUL);
}
NOINLINE u16 truncate_before_multiply(u32 a, u32 b) {
    return (u16)((((u32)(u16)(a & 0xffffUL) *
                    (u32)(u16)(b & 0xffffUL)) >> 8) & 0xffffUL);
}
NOINLINE u16 observable_widen(u16 a, u16 b) {
    volatile u32 left = (u32)a;
    return (u16)(((left * (u32)b) >> 8) & 0xffffUL);
}
NOINLINE u16 change(u16 x) { data[0] = x; return x; }
NOINLINE u16 old_value_across_call(u16 b) {
    u32 a = (u32)data[0];
    u32 c = (u32)change(b);
    return (u16)(((a*c) >> 8) & 0xffffUL);
}
NOINLINE u16 returned_then_read(u16 a, u16 b) {
    u32 wide = (u32)a;
    u16 part = (u16)(((wide*(u32)b) >> 8) & 0xffffUL);
    wide_snapshot = wide ^ 0xdeadbeefUL;
    snapshot = a;
    return part;
}
NOINLINE u16 pointer_loop(u16 *p, u16 count) {
    u16 sum = 0;
    while (count--) {
        u16 a = *p++;
        u16 b = *p;
        sum = (u16)((sum + (u16)((((u32)a*(u32)b)>>8)&0xffffUL))&0xffffu);
    }
    return sum;
}
NOINLINE u16 two_live_products(u16 a,u16 b,u16 c,u16 d) {
    u16 first = (u16)((((u32)a*(u32)b)>>8)&0xffffUL);
    u16 second = (u16)((((u32)c*(u32)d)>>8)&0xffffUL);
    return first ^ second;
}
int main(void) {
    if (mixed_signed(-1,65535u) != 65280u) return 1;
    if (zero_extend_signed_bits(-1,65535u) != 65024u) return 2;
    if (mixed_signed(-32768,32768u) != 0u) return 1;
    if (zero_extend_signed_bits(-32768,32768u) != 0u) return 2;
    if (mixed_signed(-12345,521u) != 40411u) return 1;
    if (zero_extend_signed_bits(-12345,521u) != 42715u) return 2;
    if (mixed_signed(-255,257u) != 65280u) return 1;
    if (zero_extend_signed_bits(-255,257u) != 0u) return 2;
    if (mixed_signed(32767,65535u) != 65152u) return 1;
    if (zero_extend_signed_bits(32767,65535u) != 65152u) return 2;
    if (signed_inputs(-32768,-32768) != 0u) return 3;
    if (signed_inputs(-1,-1) != 0u) return 3;
    if (signed_inputs(-32768,32767) != 128u) return 3;
    if (signed_inputs(-12345,12491) != 53010u) return 3;
    if (wider_input(305419896UL,2748u) != 53296u) return 4;
    if (truncate_before_multiply(305419896UL,2309687996UL) != 41008u) return 5;
    if (wider_input(4294967295UL,65535u) != 65280u) return 4;
    if (truncate_before_multiply(4294967295UL,2309750783UL) != 65024u) return 5;
    if (wider_input(65536UL,256u) != 0u) return 4;
    if (truncate_before_multiply(65536UL,2309685504UL) != 0u) return 5;
    if (wider_input(4275835444UL,43981u) != 25423u) return 4;
    if (truncate_before_multiply(4275835444UL,2309729229UL) != 14159u) return 5;
    if (observable_widen(1u,65535u) != 255u) return 6;
    if (returned_then_read(1u,65535u) != 255u || snapshot != 1u || wide_snapshot != 3735928558UL) return 7;
    if (observable_widen(65535u,65535u) != 65024u) return 6;
    if (returned_then_read(65535u,65535u) != 65024u || snapshot != 65535u || wide_snapshot != 3735896336UL) return 7;
    if (observable_widen(32768u,257u) != 32896u) return 6;
    if (returned_then_read(32768u,257u) != 32896u || snapshot != 32768u || wide_snapshot != 3735895791UL) return 7;
    if (observable_widen(43981u,52719u) != 13197u) return 6;
    if (returned_then_read(43981u,52719u) != 13197u || snapshot != 43981u || wide_snapshot != 3735885090UL) return 7;
    data[0] = 0x1234u;
    if (old_value_across_call(0xabcd) != 14159u || data[0] != 0xabcdu) return 8;
    data[0] = 0x1234u;
    if (pointer_loop(data,4) != 16854u) return 9;
    if (two_live_products(65535u,43981u,32769u,65244u) != 41642u) return 10;
    return 0;
}
