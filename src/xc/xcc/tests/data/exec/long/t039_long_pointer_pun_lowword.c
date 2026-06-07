#include "xcc_exec_test.h"

int main(void) {
    unsigned long x = 0x006ae9bcul;
    unsigned int *p = (unsigned int *)&x;
    XCC_CHECK_EQ_UINT_ID(1, p[0], 0xe9bcu);

    unsigned int y = p[0];
    XCC_CHECK_EQ_UINT_ID(2, y, 0xe9bcu);
    return 0;
}
