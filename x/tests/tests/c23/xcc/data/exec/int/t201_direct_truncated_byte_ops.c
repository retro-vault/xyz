#include "../../core/t234_o3_direct_truncated_byte_ops.c"

static volatile unsigned int reference_word;
static volatile int signed_reference_word;

static __attribute__((noinline)) signed char
signed_right_shift_byte(signed char value)
{
    return (signed char)(value >> 3);
}

int main(void)
{
    unsigned int i;

    for (i = 0u; i < 256u; ++i) {
        unsigned char left = (unsigned char)i;
        unsigned char right = (unsigned char)(i * 73u + 19u);

        reference_word = (unsigned int)left + (unsigned int)right;
        if (trunc_add_byte(left, right) !=
            (unsigned char)reference_word)
            return 1;

        reference_word = (unsigned int)left - (unsigned int)right;
        if (trunc_sub_byte(left, right) !=
            (unsigned char)reference_word)
            return 2;

        reference_word = (unsigned int)left & (unsigned int)right;
        if (trunc_and_byte(left, right) !=
            (unsigned char)reference_word)
            return 3;

        reference_word = (unsigned int)left | (unsigned int)right;
        if (trunc_or_byte(left, right) !=
            (unsigned char)reference_word)
            return 4;

        reference_word = (unsigned int)left ^ (unsigned int)right;
        if (trunc_xor_byte(left, right) !=
            (unsigned char)reference_word)
            return 5;

        reference_word = 0u - (unsigned int)left;
        if (trunc_neg_byte(left) != (unsigned char)reference_word)
            return 6;

        reference_word = ~(unsigned int)left;
        if (trunc_not_byte(left) != (unsigned char)reference_word)
            return 7;

        reference_word = (unsigned int)left << 3;
        if (trunc_shift_byte(left) != (unsigned char)reference_word)
            return 8;

        reference_word = (unsigned int)left >> 3;
        if (trunc_right_shift_byte(left) != (unsigned char)reference_word)
            return 12;

        reference_word = (unsigned int)left >> 7;
        if (trunc_msb_byte(left) != (unsigned char)reference_word)
            return 13;

        signed_reference_word = (signed char)left;
        signed_reference_word >>= 3;
        if (signed_right_shift_byte((signed char)left) !=
            (signed char)signed_reference_word)
            return 14;

        reference_word = left;
        for (unsigned char round = 0u; round < 8u; ++round) {
            unsigned int next = reference_word << 1;
            if ((reference_word & 0x80u) != 0u)
                next ^= 7u;
            reference_word = (unsigned char)next;
        }
        if (trunc_crc_step(left) != (unsigned char)reference_word)
            return 9;

        trunc_byte_slot = left;
        increment_truncated_byte_slot();
        reference_word = (unsigned int)left + 1u;
        if (trunc_byte_slot != (unsigned char)reference_word)
            return 10;

        trunc_byte_slot = left;
        add_three_to_truncated_byte_slot();
        reference_word = (unsigned int)left + 3u;
        if (trunc_byte_slot != (unsigned char)reference_word)
            return 11;
    }

    return 0;
}
