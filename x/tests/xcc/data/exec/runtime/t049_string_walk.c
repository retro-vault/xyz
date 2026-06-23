#include "xcc_exec_test.h"

static int tri(int n) {
    int sum = 0;

    while (n > 0) {
        sum = sum + n;
        --n;
    }
    return sum;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, tri(5), 15);
    XCC_CHECK_EQ_INT_ID(2, tri(8), 36);
    return 0;
}
