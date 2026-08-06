#include "../../core/t236_o3_msb_masked_shift_xor.c"

static volatile unsigned int reference_word;
static volatile unsigned char volatile_polynomial;

static __attribute__((noinline)) unsigned int
step_and_keep_input(unsigned char value, unsigned char polynomial)
{
    unsigned char carry = (unsigned char)(value >> 7);
    unsigned char mask = (unsigned char)(0u - carry);
    unsigned char step =
        (unsigned char)((unsigned char)(value << 1) ^
                        (unsigned char)(mask & polynomial));

    return (unsigned int)step | ((unsigned int)value << 8);
}

static __attribute__((noinline)) unsigned char
step_with_volatile_polynomial(unsigned char value)
{
    unsigned char carry = (unsigned char)(value >> 7);
    unsigned char mask = (unsigned char)(0u - carry);

    return (unsigned char)((unsigned char)(value << 1) ^
                           (unsigned char)(mask & volatile_polynomial));
}

int main(void)
{
    unsigned int input;

    for (input = 0u; input < 256u; ++input) {
        unsigned char value = (unsigned char)input;
        unsigned char polynomial = (unsigned char)(input * 53u + 29u);
        unsigned char expected;

        reference_word = (unsigned int)value << 1;
        if ((value & 0x80u) != 0u)
            reference_word ^= polynomial;
        expected = (unsigned char)reference_word;

        if (branchless_msb_shift_xor(value, polynomial) != expected)
            return 1;
        if (step_and_keep_input(value, polynomial) !=
            ((unsigned int)expected | ((unsigned int)value << 8)))
            return 2;

        volatile_polynomial = polynomial;
        if (step_with_volatile_polynomial(value) != expected)
            return 3;
    }
    return 0;
}
