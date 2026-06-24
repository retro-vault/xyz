#include "xcc_exec_test.h"

int main(void) {
    int i;
    int j;
    int k;
    int sum = 0;

    for (i = 1; i <= 10; ++i)
        sum = sum + i;
    XCC_CHECK_EQ_INT_ID(1, sum, 55);

    j = 0;
    while (j < 5) {
        sum = sum + j;
        ++j;
    }
    XCC_CHECK_EQ_INT_ID(2, sum, 65);

    k = 3;
    do {
        sum = sum + k;
        --k;
    } while (k > 0);

    XCC_CHECK_EQ_INT_ID(3, sum, 71);
    return 0;
}
