#include "xcc_exec_test.h"

static int count_pairs(int n) {
    int count = 0;
    int i;
    int j;
    for (i = 1; i <= n; i = i + 1) {
        for (j = i; j <= n; j = j + 1) {
            if (i + j == n) {
                count = count + 1;
                break;
            }
        }
    }
    return count;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, count_pairs(6), 3);
    XCC_CHECK_EQ_INT_ID(2, count_pairs(10), 5);
    return 0;
}
