#include "xcc_exec_test.h"
#include "z88dk_attrs.h"

int z88_fast_inc(int x) {
    return x + 1;
}

int z88_stack_add3(int a, int b, int c) {
    return a + b + c;
}

static void touch_callee_state(void) {
}

[[z88dk::callee]] int z88_stack_return_after_call(int a, int b) {
    touch_callee_state();
    return a + b + 14;
}

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, z88_fast_inc(41), 42);
    XCC_CHECK_EQ_INT_ID(2, z88_stack_add3(10, 20, 3), 33);
    XCC_CHECK_EQ_INT_ID(3, z88_stack_return_after_call(3, 5), 22);
    return 0;
}
