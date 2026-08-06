#include "xcc_exec_test.h"

static volatile unsigned long checkpoint;

static __attribute__((noinline)) unsigned long
step(unsigned long value, unsigned long polynomial)
{
    unsigned long mask = 0ul - (value & 1ul);
    return (value >> 1) ^ (mask & polynomial);
}

static __attribute__((noinline)) unsigned long
advance_four(unsigned long value)
{
    value = (value & 1ul) ? ((value >> 1) ^ 0xa3000005ul) : (value >> 1);
    value = (value & 1ul) ? ((value >> 1) ^ 0x10203041ul) : (value >> 1);
    value = (value & 1ul) ? ((value >> 1) ^ 0x8000800bul) : (value >> 1);
    value = (value & 1ul) ? ((value >> 1) ^ 0x01020409ul) : (value >> 1);
    return value;
}

static __attribute__((noinline)) unsigned long
advance_across_barrier(unsigned long value)
{
    value = (value & 1ul) ? ((value >> 1) ^ 0x31415927ul) : (value >> 1);
    checkpoint = value;
    value = (value & 1ul) ? ((value >> 1) ^ 0x27182819ul) : (value >> 1);
    return value;
}

int
main(void)
{
    unsigned long value = 0x6d2b79f5ul;
    unsigned int i;

    for (i = 0; i != 96u; ++i) {
        unsigned long expected = value;
        unsigned long first;

        expected = step(expected, 0xa3000005ul);
        expected = step(expected, 0x10203041ul);
        expected = step(expected, 0x8000800bul);
        expected = step(expected, 0x01020409ul);
        if (advance_four(value) != expected)
            return 1;

        first = step(value, 0x31415927ul);
        expected = step(first, 0x27182819ul);
        checkpoint = 0ul;
        if (advance_across_barrier(value) != expected)
            return 2;
        if (checkpoint != first)
            return 3;

        value = value * 1664525ul + 1013904223ul;
    }
    return 0;
}
