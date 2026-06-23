#include "xcc_exec_test.h"

static int add_to(int *p, int value) {
    *p = *p + value;
    return *p;
}

int main(void) {
    int total = 4;

    XCC_CHECK_EQ_INT_ID(1, add_to(&total, 6), 10);
    XCC_CHECK_EQ_INT_ID(2, total, 10);
    XCC_CHECK_EQ_INT_ID(3, add_to(&total, -3), 7);
    return 0;
}
