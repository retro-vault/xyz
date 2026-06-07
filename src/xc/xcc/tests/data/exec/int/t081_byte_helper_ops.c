#include "xcc_exec_test.h"

int main(void) {
    signed char sa = (signed char)-100;
    signed char sb = (signed char)7;
    signed char sn = (signed char)-9;
    unsigned char ua = (unsigned char)250;
    unsigned char ub = (unsigned char)11;

    XCC_CHECK_EQ_INT_ID(1, sa / sb, -14);
    XCC_CHECK_EQ_INT_ID(2, sa % sb, -2);
    XCC_CHECK_EQ_INT_ID(3, sa / ub, -9);
    XCC_CHECK_EQ_INT_ID(4, sa % ub, -1);
    XCC_CHECK_EQ_INT_ID(5, ua / sb, 35);
    XCC_CHECK_EQ_INT_ID(6, ua % sb, 5);
    XCC_CHECK_EQ_INT_ID(7, (signed char)12 * sn, -108);
    XCC_CHECK_EQ_INT_ID(8, (signed char)-5 * ub, -55);
    XCC_CHECK_EQ_INT_ID(9, ua * (signed char)-4, -1000);
    XCC_CHECK_EQ_INT_ID(10, ua / ub, 22);
    XCC_CHECK_EQ_INT_ID(11, ua % ub, 8);

    return 0;
}
