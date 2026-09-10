typedef unsigned char u8;

typedef unsigned int u16;

static volatile int divisor = 8;
static volatile unsigned mask_word = 1023;

unsigned char byte_quotient(unsigned char value) { return value / 8; }
unsigned char byte_remainder(unsigned char value) { return value % 8; }
int masked_quotient(int value) { return (value & 1023) / 8; }
int exact_quotient(int value) { return (value & -8) / 8; }
int exact_remainder(int value) { return (value & -8) % 8; }
int ordinary_quotient(int value) { return value / 8; }
int ordinary_remainder(int value) { return value % 8; }
unsigned redundant_mask(unsigned value) { return ((value >> 6) | 7) & 1023; }
int impossible_mask(unsigned value) { return ((value & 0x7555) | 0x0120) == 0x1a55; }
int impossible_range(unsigned value) { return (value & 511) > 600; }
int sum_nonnegative(u8 a, u8 b) { return (a + b + 17) / 8; }
_Bool high_bool(unsigned value) { return (_Bool)(value | 256); }

static const int inputs[] = {
    -32768, -32767, -32761, -32760, -1025, -1024, -1023,
    -257, -256, -255, -17, -16, -15, -9, -8, -7, -1,
    0, 1, 7, 8, 9, 15, 16, 17, 255, 256, 257, 1023, 1024,
    1025, 32759, 32760, 32761, 32767
};

int main(void)
{
    unsigned i;
    for (i = 0; i < 256; ++i) {
        u8 a = (u8)i;
        u8 b = (u8)(i * 73u + 11u);
        if (byte_quotient(a) != i / divisor) return 1;
        if (byte_remainder(a) != i % divisor) return 2;
        if (sum_nonnegative(a, b) != ((int)a + b + 17) / divisor)
            return 3;
        if (!high_bool(i)) return 4;
    }
    for (i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        int value = inputs[i];
        int multiple = value & -8;
        unsigned raw = (unsigned)value;
        if (masked_quotient(value) != (int)(raw & mask_word) / divisor)
            return 5;
        if (exact_quotient(value) != multiple / divisor) return 6;
        if (exact_remainder(value) != multiple % divisor) return 7;
        if (ordinary_quotient(value) != value / divisor) return 8;
        if (ordinary_remainder(value) != value % divisor) return 9;
        if (redundant_mask(raw) != (((raw >> 6) | 7u) & mask_word))
            return 10;
        if (impossible_mask(raw) || impossible_range(raw)) return 11;
        if (!high_bool(raw)) return 12;
    }
    return 0;
}
