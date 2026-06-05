#include "z88dk_attrs.h"

int z88_stack_add3(int a, int b, int c) {
    return a + b + c;
}

int main(void) {
    return z88_stack_add3(1, 2, 3);
}
