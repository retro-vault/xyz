#include "xcc_exec_test.h"

static void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int main(void) {
    int x = 10;
    int y = 20;
    swap(&x, &y);
    XCC_CHECK_EQ_INT_ID(1, x, 20);
    XCC_CHECK_EQ_INT_ID(2, y, 10);
    int z = 5;
    int *p = &z;
    *p = 42;
    XCC_CHECK_EQ_INT_ID(3, z, 42);
    return 0;
}
