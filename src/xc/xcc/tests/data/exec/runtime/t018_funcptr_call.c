#include "xcc_exec_test.h"

static int add_one(int x) {
    return x + 1;
}

static int call_it(int (*fn)(int), int value) {
    return fn(value);
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, call_it(add_one, 41), 42);
    return 0;
}
