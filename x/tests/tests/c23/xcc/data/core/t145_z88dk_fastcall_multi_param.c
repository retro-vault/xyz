#include "z88dk_attrs.h"

[[z88dk::fastcall]] int z88_fast_sum2(int a, int b) {
    return a + b;
}

int main(void) {
    [[z88dk::fastcall]] int (*fp)(int, int) = z88_fast_sum2;
    return fp(3, 4);
}
