#include "xcc_exec_test.h"

unsigned char src, b;

void f(void)
{
    unsigned char a = src + 1;
    if (a != b)
        b = a;
}

int main(void)
{
    src = 0u;
    b = 0u;
    f();
    XCC_CHECK_EQ_UINT_ID(1, b, 1u);

    src = 41u;
    b = 9u;
    f();
    XCC_CHECK_EQ_UINT_ID(2, b, 42u);

    src = 41u;
    b = 42u;
    f();
    XCC_CHECK_EQ_UINT_ID(3, b, 42u);

    src = 255u;
    b = 7u;
    f();
    XCC_CHECK_EQ_UINT_ID(4, b, 0u);

    return 0;
}
