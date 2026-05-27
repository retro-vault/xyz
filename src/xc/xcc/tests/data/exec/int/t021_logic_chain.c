#include "xcc_exec_test.h"

int main(void) {
    int n = 0;

    if (5 > 3)
        n = n + 1;
    if (7 == 7)
        n = n + 2;
    if (4 != 0)
        n = n + 4;

    XCC_CHECK_EQ_INT_ID(1, n, 7);
    return 0;
}
