#include "xcc_exec_test.h"

static int count_digits(unsigned int n) {
    if (n == 0u) return 1;
    int c = 0;
    while (n > 0u) {
        n = n / 10u;
        c = c + 1;
    }
    return c;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, count_digits(0u), 1);
    XCC_CHECK_EQ_INT_ID(2, count_digits(9u), 1);
    XCC_CHECK_EQ_INT_ID(3, count_digits(10u), 2);
    XCC_CHECK_EQ_INT_ID(4, count_digits(999u), 3);
    XCC_CHECK_EQ_INT_ID(5, count_digits(10000u), 5);
    return 0;
}
