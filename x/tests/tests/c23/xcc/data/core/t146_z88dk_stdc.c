#include "z88dk_attrs.h"

[[z88dk::stdc]] int z88_stdc_sum2(int a, int b) {
    return a + b;
}

int main(void) {
    [[z88dk::stdc]] int (*fp)(int, int) = z88_stdc_sum2;
    return fp(5, 6);
}
