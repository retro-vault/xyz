#include "xcc_exec_test.h"

#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

static double d_abs(double x) {
    return x < 0.0 ? -x : x;
}

int main(void) {
    float sf = sinhf(1.0f);
    if (!(sf > 1.10f && sf < 1.25f)) return 1;

    sf = sinhf(-1.0f);
    if (!(sf < -1.10f && sf > -1.25f)) return 2;

    sf = coshf(1.0f);
    if (!(sf > 1.45f && sf < 1.65f)) return 3;

    sf = coshf(-1.0f);
    if (!(sf > 1.45f && sf < 1.65f)) return 4;

    sf = tanhf(1.0f);
    if (!(sf > 0.70f && sf < 0.82f)) return 5;

    sf = tanhf(-1.0f);
    if (!(sf < -0.70f && sf > -0.82f)) return 6;

    sf = tanhf(8.0f);
    if (!(sf > 0.99f && sf <= 1.0f)) return 7;

    sf = tanhf(-8.0f);
    if (!(sf < -0.99f && sf >= -1.0f)) return 8;

    sf = sinhf(0.0f);
    if (sf != 0.0f) return 9;

    sf = coshf(0.0f);
    if (f_abs(sf - 1.0f) > 0.001f) return 10;

    sf = tanhf(0.0f);
    if (sf != 0.0f) return 11;

    {
        double dx = sinh(1.0);
        if (!(dx > 1.10 && dx < 1.25)) return 12;
    }

    {
        double dx = cosh(1.5);
        if (!(dx > 2.20 && dx < 2.45)) return 13;
    }

    {
        double dx = tanh(-2.0);
        if (!(dx < -0.95 && dx > -1.0)) return 14;
    }

    {
        double dx = tanhl(0.0);
        if (d_abs(dx) > 0.000001) return 15;
    }

    return 0;
}
