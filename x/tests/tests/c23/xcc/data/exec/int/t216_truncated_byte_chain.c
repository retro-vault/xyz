static volatile unsigned int reference_word;
static volatile int signed_reference_word;
static volatile unsigned int volatile_word;

static __attribute__((noinline)) unsigned char
shift_xor_chain(unsigned char accumulator)
{
    return (unsigned char)(((unsigned int)accumulator << 1) ^
                           ((unsigned int)accumulator + 19u));
}

static __attribute__((noinline)) unsigned char
arithmetic_chain(unsigned char value)
{
    return (unsigned char)((((unsigned int)value + 73u) * 3u) ^ 0xa5u);
}

static __attribute__((noinline)) unsigned char
wide_input_chain(unsigned int value)
{
    return (unsigned char)(((value - 0x0345u) << 3) ^ 0x5au);
}

static __attribute__((noinline)) signed char
signed_chain(signed char value)
{
    return (signed char)((((int)value + 37) * 5) ^ 0x6d);
}

static __attribute__((noinline)) signed char
guarded_signed_right_shift(signed char value)
{
    return (signed char)(((int)value >> 3) ^ 0x35);
}

static __attribute__((noinline)) unsigned char
guarded_volatile_input(void)
{
    return (unsigned char)(((volatile_word + 0x1234u) << 1) ^ 0x5au);
}

static __attribute__((noinline)) unsigned int
shift_mask_9(unsigned int value)
{
    return (value >> 9) & 0x000fu;
}

static __attribute__((noinline)) unsigned int
shift_mask_15(unsigned int value)
{
    return (value >> 15) & 0x0001u;
}

int
main(void)
{
    unsigned int input;

    for (input = 0u; input < 256u; ++input) {
        unsigned char left = (unsigned char)input;
        signed char signed_left = (signed char)left;

        reference_word = ((unsigned int)left << 1) ^
                         ((unsigned int)left + 19u);
        if (shift_xor_chain(left) != (unsigned char)reference_word)
            return 1;

        reference_word = (((unsigned int)left + 73u) * 3u) ^ 0xa5u;
        if (arithmetic_chain(left) != (unsigned char)reference_word)
            return 2;

        reference_word =
            (((input * 257u + 0x1200u) - 0x0345u) << 3) ^ 0x5au;
        if (wide_input_chain((unsigned int)(input * 257u + 0x1200u)) !=
            (unsigned char)reference_word)
            return 3;

        signed_reference_word = (((int)signed_left + 37) * 5) ^ 0x6d;
        if (signed_chain(signed_left) !=
            (signed char)signed_reference_word)
            return 4;

        signed_reference_word = ((int)signed_left >> 3) ^ 0x35;
        if (guarded_signed_right_shift(signed_left) !=
            (signed char)signed_reference_word)
            return 5;

        volatile_word = (unsigned int)(input * 211u + 0x4567u);
        reference_word = ((volatile_word + 0x1234u) << 1) ^ 0x5au;
        if (guarded_volatile_input() != (unsigned char)reference_word)
            return 6;

        reference_word = input * 257u + 0x1200u;
        if (shift_mask_9((unsigned int)reference_word) !=
            (((unsigned char)(reference_word >> 8) >> 1) & 0x0fu))
            return 7;
        if (shift_mask_15((unsigned int)reference_word) !=
            (((unsigned char)(reference_word >> 8) >> 7) & 0x01u))
            return 8;
    }

    return 0;
}
