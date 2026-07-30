#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned char
advance(unsigned char value)
{
    value = (value & 0x80u)
                ? ((unsigned char)(value << 1) ^ 0x1du)
                : (unsigned char)(value << 1);
    value = (value & 0x80u)
                ? ((unsigned char)(value << 1) ^ 0xa7u)
                : (unsigned char)(value << 1);
    value = (value & 0x80u)
                ? ((unsigned char)(value << 1) ^ 0x39u)
                : (unsigned char)(value << 1);
    value = (value & 0x80u)
                ? ((unsigned char)(value << 1) ^ 0xd5u)
                : (unsigned char)(value << 1);
    return value;
}

static unsigned char
reference_step(unsigned char value, unsigned char polynomial)
{
    unsigned char carry = (unsigned char)(value >> 7);
    unsigned char mask = (unsigned char)(0u - carry);

    return (unsigned char)((unsigned char)(value << 1) ^
                           (unsigned char)(mask & polynomial));
}

int
main(void)
{
    unsigned int input;

    for (input = 0; input != 256u; ++input) {
        unsigned char expected = (unsigned char)input;
        expected = reference_step(expected, 0x1du);
        expected = reference_step(expected, 0xa7u);
        expected = reference_step(expected, 0x39u);
        expected = reference_step(expected, 0xd5u);
        if (advance((unsigned char)input) != expected)
            return (int)(input + 1u);
    }
    return 0;
}
