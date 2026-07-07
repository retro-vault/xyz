#include "xcc_exec_test.h"
#include <stdarg.h>

static int sum3(int count, ...) {
    va_list ap;
    int total = 0;

    va_start(ap, count);
    while (count > 0) {
        total += va_arg(ap, int);
        --count;
    }
    va_end(ap);
    return total;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, sum3(3, 10, 20, 3), 33);
    XCC_CHECK_EQ_INT_ID(2, sum3(2, 7, 5), 12);
    XCC_CHECK_EQ_INT_ID(3, sum3(4, 1, 2, 3, 4), 10);
    return 0;
}
