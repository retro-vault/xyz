#include "xcc_exec_test.h"

static volatile int square_limits[3] = { 1, 5, 10 };

static int sum_squares(int limit) {
    int i;
    int sum;

    sum = 0;
    for (i = 1; i <= limit; ++i) {
        sum += i * i;
    }
    return sum;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, sum_squares(square_limits[0]), 1);
    XCC_CHECK_EQ_INT_ID(2, sum_squares(square_limits[1]), 55);
    XCC_CHECK_EQ_INT_ID(3, sum_squares(square_limits[2]), 385);
    return 0;
}
