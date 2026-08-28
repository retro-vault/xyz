typedef unsigned long u32;

u32
mix_step(u32 a, u32 b, u32 c, u32 d, u32 input)
{
    a += (((b ^ d) & c) ^ d) + input + 3614090360ul;
    a = (a << 7) | (a >> 25);
    a += b;
    return a;
}
