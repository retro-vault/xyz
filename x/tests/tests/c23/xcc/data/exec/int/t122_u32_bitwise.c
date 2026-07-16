#include "xcc_exec_test.h"

static unsigned long
and32(unsigned long a, unsigned long b)
{
    return a & b;
}

static unsigned long
or32(unsigned long a, unsigned long b)
{
    return a | b;
}

static unsigned long
xor32(unsigned long a, unsigned long b)
{
    return a ^ b;
}

static unsigned long
xor_const32(unsigned long a)
{
    return a ^ 0xa55aa55aul;
}

static unsigned long
frame_add32(unsigned long a, unsigned long b)
{
    volatile unsigned long left = a;
    volatile unsigned long right = b;
    volatile unsigned long result = left + right;

    return result;
}

static unsigned long
frame_add_chain32(unsigned long a, unsigned long b,
                  unsigned long c, unsigned long d)
{
    volatile unsigned long first = a;
    volatile unsigned long second = b;
    volatile unsigned long third = c;
    volatile unsigned long fourth = d;

    return first + second + third + fourth;
}

static unsigned long
bitwise_add_chain32(unsigned long a, unsigned long b,
                    unsigned long c, unsigned long d,
                    const unsigned long *input)
{
    volatile unsigned long first = a;
    volatile unsigned long second = b;
    volatile unsigned long third = c;
    volatile unsigned long fourth = d;

    return (fourth ^ (second & (third ^ fourth))) + *input +
           0xd76aa478ul + first;
}

static unsigned long
place_byte_0(unsigned char value)
{
    return (unsigned long)value;
}

static unsigned long
place_byte_8(unsigned char value)
{
    return (unsigned long)value << 8;
}

static unsigned long
place_byte_16(unsigned char value)
{
    return (unsigned long)value << 16;
}

static unsigned long
place_byte_24(unsigned char value)
{
    return (unsigned long)value << 24;
}

int
main(void)
{
    volatile unsigned long a = 0x12345678ul;
    volatile unsigned long b = 0xf0f00f0ful;

    XCC_CHECK_EQ_U32_ID(1, and32(a, b),       0x0608u, 0x1030u);
    XCC_CHECK_EQ_U32_ID(2, or32(a, b),        0x5f7fu, 0xf2f4u);
    XCC_CHECK_EQ_U32_ID(3, xor32(a, b),       0x5977u, 0xe2c4u);
    XCC_CHECK_EQ_U32_ID(4, xor_const32(a),    0xf322u, 0xb76eu);
    XCC_CHECK_EQ_U32_ID(5, frame_add32(0xfffffffful, 2ul),
                        0x0001u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(6,
        frame_add_chain32(0xfffffffful, 1ul,
                          0x12345678ul, 0xedcba987ul),
        0xffffu, 0xffffu);
    XCC_CHECK_EQ_U32_ID(7,
        frame_add_chain32(0xfffffffful, 0xfffffffful,
                          0xfffffffful, 0xfffffffful),
        0xfffcu, 0xffffu);
    XCC_CHECK_EQ_U32_ID(8,
        bitwise_add_chain32(0x12345678ul, 0x89abcdeful,
                            0x0f1e2d3cul, 0x76543210ul, &b),
        0x493bu, 0x59edu);
    XCC_CHECK_EQ_U32_ID(9,  place_byte_0(0xa5u),  0x00a5u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(10, place_byte_8(0xa5u),  0xa500u, 0x0000u);
    XCC_CHECK_EQ_U32_ID(11, place_byte_16(0xa5u), 0x0000u, 0x00a5u);
    XCC_CHECK_EQ_U32_ID(12, place_byte_24(0xa5u), 0x0000u, 0xa500u);
    return 0;
}
