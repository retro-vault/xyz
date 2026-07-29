#include "xcc_exec_test.h"

static volatile unsigned inputs[3] = {84, 7, 5};

static unsigned
regular_divide(unsigned value, unsigned divisor)
{
    unsigned acc = 0;
    unsigned i;

    for (i = 1; i <= divisor; ++i)
        acc += value / i;
    return acc;
}

static unsigned
recursive_accumulate(unsigned base, unsigned current, unsigned limit)
{
    if (current >= limit)
        return base;
    return recursive_accumulate(base + current, current + 1, limit);
}

[[sdcc::sdccall(0)]] static unsigned
explicit_stack_modulo(unsigned value, unsigned divisor)
{
    unsigned acc = value;
    unsigned i;

    for (i = 1; i <= divisor; ++i)
        acc += value % i;
    return acc;
}

int
main(void)
{
    unsigned regular = regular_divide(inputs[0], inputs[1]);
    unsigned stack = explicit_stack_modulo(inputs[0], inputs[2]);

    regular += regular_divide(inputs[0], inputs[2]);
    stack += explicit_stack_modulo(inputs[0], inputs[1]);
    XCC_CHECK_EQ_UINT_ID(1, regular, 408);
    XCC_CHECK_EQ_UINT_ID(2, stack, 176);
    XCC_CHECK_EQ_UINT_ID(3, recursive_accumulate(3, 1, 6), 18);
    return 0;
}
