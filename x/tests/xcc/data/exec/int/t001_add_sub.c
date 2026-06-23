#include "xcc_exec_test.h"

int main(void) {
    int a = 1234;
    int b = 4321;

    XCC_CHECK_EQ_INT_ID(1, a + b, 5555);
    XCC_CHECK_EQ_INT_ID(2, 5000 - 1234, 3766);
    XCC_CHECK_EQ_INT_ID(3, 0 - 0, 0);
    return 0;
}
