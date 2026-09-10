typedef unsigned char u8;

static volatile int divisor = 8;
static volatile unsigned source;
static unsigned modified;

int signed_byte_division(signed char value)
{
    return value / 8;
}

int negative_multiple_division(signed char value)
{
    return ((int)value & -8) / 8;
}

int negative_rounded_division(int value)
{
    return ((value & -8) | 3) / 8;
}

unsigned wrap_add(u8 value)
{
    return (unsigned)((u8)(value + 224)) / 8;
}

unsigned branch_join(unsigned value, unsigned choose)
{
    unsigned result;
    if (choose)
        result = value & 31;
    else
        result = value | 0x8000;
    return result & 0x803f;
}

unsigned loop_redefinition(unsigned value)
{
    unsigned i;
    value &= 31;
    for (i = 0; i < 3; ++i)
        value = (value << 5) | (value >> 11);
    return value & 0x803f;
}

unsigned aliased_value(unsigned *ptr)
{
    unsigned copy = *ptr & 31;
    *ptr = 0x8000;
    return copy | (*ptr & 0x803f);
}

unsigned volatile_reads(void)
{
    return (source & 0) + (source | 0xffff);
}

long wide_nonnegative(long value)
{
    return (value & 0x7fffffffL) / 256L;
}

long wide_exact(long value)
{
    return (value & -256L) / 256L;
}

int main(void)
{
    unsigned i;
    for (i = 0; i < 256; ++i) {
        signed char value = (signed char)i;
        int mixed = ((int)value & -8) | 3;
        unsigned j;
        unsigned rotating = i & 31;
        unsigned expected_wrap = (u8)(i + 224);
        if (signed_byte_division(value) != value / divisor) return 1;
        if (negative_multiple_division(value) != ((int)value & -8) / divisor)
            return 2;
        if (negative_rounded_division(value) != mixed / divisor) return 3;
        if (wrap_add((u8)i) != expected_wrap / divisor) return 4;
        if (branch_join(i, 1) != (i & 31)) return 5;
        if (branch_join(i, 0) != ((i | 0x8000) & 0x803f)) return 6;
        for (j = 0; j < 3; ++j)
            rotating = (rotating << 5) | (rotating >> 11);
        if (loop_redefinition(i) != (rotating & 0x803f)) return 7;
        modified = i;
        if (aliased_value(&modified) != ((i & 31) | 0x8000)) return 8;
        if (modified != 0x8000) return 9;
    }
    source = 0x1234;
    if (volatile_reads() != 0xffff) return 10;
    if (wide_nonnegative(-1L) != 8388607L) return 11;
    if (wide_nonnegative(0x12345678L) != 0x123456L) return 12;
    if (wide_exact(-1L) != -1L) return 13;
    if (wide_exact(-2147483647L - 1L) != -8388608L) return 14;
    if (wide_exact(2147483647L) != 8388607L) return 15;
    return 0;
}
