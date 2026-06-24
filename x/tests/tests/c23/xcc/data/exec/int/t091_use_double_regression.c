#include "xcc_exec_test.h"

static volatile int use_inputs[3] = { 9, 0, -4 };

static int double_it(int x) {
    return x + x;
}

static int use_double(int x) {
    return double_it(x) + 1;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, use_double(use_inputs[0]), 19);
    XCC_CHECK_EQ_INT_ID(2, use_double(use_inputs[1]), 1);
    XCC_CHECK_EQ_INT_ID(3, use_double(use_inputs[2]), -7);
    return 0;
}
