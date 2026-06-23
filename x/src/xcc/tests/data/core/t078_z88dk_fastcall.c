#include "z88dk_attrs.h"

int z88_fast_inc(int x) {
    return x + 1;
}

int main(void) {
    return z88_fast_inc(7);
}
