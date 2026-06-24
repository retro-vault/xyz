#include "xcc_exec_test.h"

static int char_add(signed char a, signed char b) {
    return (int)a + (int)b;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, char_add(10, -3), 7);
    XCC_CHECK_EQ_INT_ID(2, char_add(-5, -5), -10);
    XCC_CHECK_EQ_INT_ID(3, char_add(127, 0), 127);
    XCC_CHECK_EQ_INT_ID(4, char_add(-128, 1), -127);
    return 0;
}
