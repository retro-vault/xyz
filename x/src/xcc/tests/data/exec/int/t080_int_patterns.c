#include "xcc_exec_test.h"

static unsigned int popcount(unsigned int v) {
    unsigned int c = 0u;
    while (v != 0u) {
        c = c + (v & 1u);
        v = v >> 1;
    }
    return c;
}

static int is_power_of_two(unsigned int v) {
    if (v == 0u) return 0;
    return (v & (v - 1u)) == 0u;
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, popcount(0u), 0u);
    XCC_CHECK_EQ_UINT_ID(2, popcount(0xFFu), 8u);
    XCC_CHECK_EQ_UINT_ID(3, popcount(0xA5u), 4u);
    XCC_CHECK_EQ_INT_ID(4, is_power_of_two(0u), 0);
    XCC_CHECK_EQ_INT_ID(5, is_power_of_two(1u), 1);
    XCC_CHECK_EQ_INT_ID(6, is_power_of_two(256u), 1);
    XCC_CHECK_EQ_INT_ID(7, is_power_of_two(7u), 0);
    return 0;
}
