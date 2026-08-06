unsigned char trunc_add_byte(unsigned char left, unsigned char right)
{
    return (unsigned char)(left + right);
}

unsigned char trunc_sub_byte(unsigned char left, unsigned char right)
{
    return (unsigned char)(left - right);
}

unsigned char trunc_and_byte(unsigned char left, unsigned char right)
{
    return (unsigned char)(left & right);
}

unsigned char trunc_or_byte(unsigned char left, unsigned char right)
{
    return (unsigned char)(left | right);
}

unsigned char trunc_xor_byte(unsigned char left, unsigned char right)
{
    return (unsigned char)(left ^ right);
}

unsigned char trunc_neg_byte(unsigned char value)
{
    return (unsigned char)(0u - value);
}

unsigned char trunc_not_byte(unsigned char value)
{
    return (unsigned char)~value;
}

unsigned char trunc_shift_byte(unsigned char value)
{
    return (unsigned char)((unsigned int)value << 3);
}

unsigned char trunc_right_shift_byte(unsigned char value)
{
    return (unsigned char)(value >> 3);
}

unsigned char trunc_msb_byte(unsigned char value)
{
    return (unsigned char)((unsigned int)value >> 7);
}

unsigned char trunc_crc_step(unsigned char value)
{
#define TRUNC_CRC_ROUND()                                                   \
    do {                                                                    \
        value = (value & 0x80u) != 0u                                      \
            ? ((unsigned char)(value << 1) ^ 7u)                            \
            : (unsigned char)(value << 1);                                  \
    } while (0)

    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
    TRUNC_CRC_ROUND();
#undef TRUNC_CRC_ROUND
    return value;
}

volatile unsigned char trunc_byte_slot;

void increment_truncated_byte_slot(void)
{
    ++trunc_byte_slot;
}

void add_three_to_truncated_byte_slot(void)
{
    trunc_byte_slot += 3u;
}
