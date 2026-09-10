typedef unsigned char u8;
typedef unsigned int u16;
static __attribute__((noinline)) u8 subtract_0(u8 value) { return (u8)(0u - value); }
static __attribute__((noinline)) u8 subtract_1(u8 value) { return (u8)(1u - value); }
static __attribute__((noinline)) u8 subtract_17(u8 value) { return (u8)(17u - value); }
static __attribute__((noinline)) u8 subtract_73(u8 value) { return (u8)(73u - value); }
static __attribute__((noinline)) u8 subtract_127(u8 value) { return (u8)(127u - value); }
static __attribute__((noinline)) u8 subtract_128(u8 value) { return (u8)(128u - value); }
static __attribute__((noinline)) u8 subtract_254(u8 value) { return (u8)(254u - value); }
static __attribute__((noinline)) u8 subtract_255(u8 value) { return (u8)(255u - value); }

static const u16 constants[] = {0u, 1u, 17u, 73u, 127u, 128u, 254u, 255u};
static u8 (*const functions[])(u8) = {subtract_0, subtract_1, subtract_17, subtract_73, subtract_127, subtract_128, subtract_254, subtract_255};
int main(void)
{
    u16 i;
    u16 value;
    for (i = 0; i < sizeof(constants) / sizeof(constants[0]); ++i)
        for (value = 0; value < 256u; ++value) {
            volatile u16 left = constants[i];
            volatile u16 right = value;
            if (functions[i]((u8)value) != (u8)(left - right))
                return 1;
        }
    return 0;
}
