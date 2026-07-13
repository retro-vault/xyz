#include "xcc_exec_test.h"

static volatile int source_value = 0x1234;

int
modern_word_result(void)
{
    return source_value;
}

int
variadic_stack_wrapper(int tag, ...)
{
    int const result = modern_word_result();

    (void)tag;
    return result;
}

int
main(void)
{
    XCC_CHECK_EQ_INT_ID(1, variadic_stack_wrapper(1, 2), 0x1234);
    source_value = -12345;
    XCC_CHECK_EQ_INT_ID(2, variadic_stack_wrapper(3, 4), -12345);
    return 0;
}
