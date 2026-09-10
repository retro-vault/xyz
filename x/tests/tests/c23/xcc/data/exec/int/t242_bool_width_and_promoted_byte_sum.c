typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
#define NOINLINE __attribute__((noinline))

static NOINLINE _Bool byte_bool(u8 value) { return (_Bool)value; }
static NOINLINE _Bool word_bool(u16 value) { return (_Bool)value; }
static NOINLINE _Bool long_bool(u32 value) { return (_Bool)value; }
static NOINLINE _Bool high_bool(u16 value) { return (_Bool)(value | 256u); }
static NOINLINE u16 byte_sum(u8 a, u8 b) { return a + b; }
static NOINLINE u16 reverse_sum(u8 a, u8 b) { return b + a; }
static NOINLINE int divided_sum(u8 a, u8 b) { return (a + b + 17) / 8; }

static const u16 values[] = {0u,1u,2u,127u,128u,255u,256u,257u,32768u,65535u};
static const u32 long_values[] = {0ul,1ul,256ul,65536ul,0x80000000ul,0xfffffffful};

int main(void)
{
    u16 i;
    u16 j;
    volatile u8 observed;
    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        observed = word_bool(values[i]);
        if (observed != (values[i] != 0u)) return 1;
        observed = byte_bool((u8)values[i]);
        if (observed != ((u8)values[i] != 0u)) return 2;
        observed = high_bool(values[i]);
        if (observed != 1u) return 3;
        for (j = 0; j < sizeof(values) / sizeof(values[0]); ++j) {
            volatile u16 a = (u8)values[i];
            volatile u16 b = (u8)values[j];
            if (byte_sum((u8)a, (u8)b) != a + b) return 4;
            if (reverse_sum((u8)a, (u8)b) != a + b) return 5;
            if (divided_sum((u8)a, (u8)b) != (a + b + 17u) / 8u) return 6;
        }
    }
    for (i = 0; i < sizeof(long_values) / sizeof(long_values[0]); ++i) {
        observed = long_bool(long_values[i]);
        if (observed != (long_values[i] != 0ul)) return 7;
    }
    return 0;
}
