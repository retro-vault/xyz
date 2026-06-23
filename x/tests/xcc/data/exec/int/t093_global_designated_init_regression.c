#include "xcc_exec_test.h"

struct point {
    int x;
    int y;
};

static int table[4] = { [2] = -4, [0] = 9 };
static struct point p = { .y = 10, .x = -3 };

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, table[0], 9);
    XCC_CHECK_EQ_INT_ID(2, table[1], 0);
    XCC_CHECK_EQ_INT_ID(3, table[2], -4);
    XCC_CHECK_EQ_INT_ID(4, table[3], 0);
    XCC_CHECK_EQ_INT_ID(5, p.x, -3);
    XCC_CHECK_EQ_INT_ID(6, p.y, 10);
    return 0;
}
