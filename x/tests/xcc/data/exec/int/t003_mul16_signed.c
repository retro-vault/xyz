#include "xcc_exec_test.h"

extern int __mul16(int a, int b);

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, __mul16(321, 7), 2247);
    XCC_CHECK_EQ_INT_ID(2, __mul16(511, 63), 32193);
    XCC_CHECK_EQ_INT_ID(3, __mul16(409, 0), 0);
    return 0;
}
