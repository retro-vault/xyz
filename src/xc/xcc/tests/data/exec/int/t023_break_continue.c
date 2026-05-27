#include "xcc_exec_test.h"

static int skip_one(void) {
    int i;
    int sum = 0;

    for (i = 0; i < 4; ++i) {
        if (i == 1)
            continue;
        sum = sum + i;
    }
    return sum;
}

static int stop_early(void) {
    int i;
    int sum = 0;

    for (i = 0; i < 5; ++i) {
        if (i == 3)
            break;
        sum = sum + i;
    }
    return sum;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, skip_one(), 5);
    XCC_CHECK_EQ_INT_ID(2, stop_early(), 3);
    return 0;
}
