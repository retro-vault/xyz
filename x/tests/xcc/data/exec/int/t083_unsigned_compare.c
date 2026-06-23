#include "xcc_exec_test.h"

static int lt_u8(unsigned char a, unsigned char b) { return a < b; }
static int le_u8(unsigned char a, unsigned char b) { return a <= b; }
static int gt_u8(unsigned char a, unsigned char b) { return a > b; }
static int ge_u8(unsigned char a, unsigned char b) { return a >= b; }

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, lt_u8(200u, 250u), 1);
    XCC_CHECK_EQ_INT_ID(2, lt_u8(250u, 200u), 0);
    XCC_CHECK_EQ_INT_ID(3, le_u8(250u, 250u), 1);
    XCC_CHECK_EQ_INT_ID(4, gt_u8(250u, 200u), 1);
    XCC_CHECK_EQ_INT_ID(5, gt_u8(1u, 255u), 0);
    XCC_CHECK_EQ_INT_ID(6, ge_u8(255u, 1u), 1);
    XCC_CHECK_EQ_INT_ID(7, ge_u8(1u, 255u), 0);
    return 0;
}
