static volatile unsigned and_mask = 0x1111;
static volatile unsigned or_mask = 0x5335;
static volatile unsigned xor_mask = 0x0ff0;
static volatile unsigned shift_seven = 7;

unsigned mask_chain(unsigned value) { return (value & 0x5555) & 0x3333; }
unsigned or_chain(unsigned value) { return (value | 0x5005) | 0x0330; }
unsigned xor_chain(unsigned value) { return (value ^ 0x55aa) ^ 0x5a5a; }
unsigned left_chain(unsigned value) { return (value << 3) << 4; }
unsigned right_chain(unsigned value) { return (value >> 3) >> 4; }
unsigned full_left_chain(unsigned value) { return (value << 7) << 10; }
unsigned full_right_chain(unsigned value) { return (value >> 7) >> 10; }
int sign_chain(int value) { return (value >> 5) >> 12; }
unsigned long wide_left_chain(unsigned long value) { return (value << 21) << 17; }
unsigned long wide_right_chain(unsigned long value) { return (value >> 21) >> 17; }
long wide_sign_chain(long value) { return (value >> 21) >> 17; }

unsigned shared_chain(unsigned value)
{
    unsigned partial = value & 0x5555;
    return (partial & 0x3333) + partial;
}

unsigned changed_source(unsigned value)
{
    unsigned partial = value & 0x5555;
    value += 17;
    return (partial & 0x3333) ^ value;
}

int main(void)
{
    unsigned i;
    for (i = 0; i < 256; ++i) {
        unsigned value = i * 271u;
        int signed_value = (int)value;
        if (mask_chain(value) != (value & and_mask)) return 1;
        if (or_chain(value) != (value | or_mask)) return 2;
        if (xor_chain(value) != (value ^ xor_mask)) return 3;
        if (left_chain(value) != (unsigned)(value << shift_seven)) return 4;
        if (right_chain(value) != (value >> shift_seven)) return 5;
        if (full_left_chain(value) || full_right_chain(value)) return 6;
        if (sign_chain(signed_value) != (signed_value < 0 ? -1 : 0)) return 7;
        if (shared_chain(value) != (unsigned)((value & and_mask) + (value & 0x5555)))
            return 8;
        if (changed_source(value) != ((value & and_mask) ^ (unsigned)(value + 17)))
            return 9;
    }
    if (wide_left_chain(0xffffffffUL) || wide_right_chain(0xffffffffUL))
        return 10;
    if (wide_sign_chain(-1L) != -1L || wide_sign_chain(2147483647L) != 0L)
        return 11;
    return 0;
}
