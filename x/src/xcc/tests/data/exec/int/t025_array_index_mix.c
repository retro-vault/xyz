#include "xcc_exec_test.h"

int main(void) {
    unsigned int a = 100u;
    unsigned int b = 200u;

    if (a < b)
        a = a + 50u;
    else
        a = 0u;

    b = a + 250u;

    XCC_CHECK_EQ_UINT_ID(1, a, 150u);
    XCC_CHECK_EQ_UINT_ID(2, b, 400u);
    return 0;
}
