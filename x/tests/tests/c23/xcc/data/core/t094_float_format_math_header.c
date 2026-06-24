#include <math.h>

float header_root_abs(float x) {
    return sqrtf(fabsf(x));
}

int header_finite(float x) {
    return isfinite(x);
}
