#include "xcc_exec_test.h"

#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

static double d_abs(double x) {
    return x < 0.0 ? -x : x;
}

int main(void) {
    float sf = asinhf(1.0f);
    if (!(sf > 0.84f && sf < 0.92f)) return 1;

    sf = asinhf(-1.0f);
    if (!(sf < -0.84f && sf > -0.92f)) return 2;

    sf = acoshf(1.0f);
    if (sf != 0.0f) return 3;

    sf = acoshf(2.0f);
    if (!(sf > 1.28f && sf < 1.36f)) return 4;

    sf = atanhf(0.5f);
    if (!(sf > 0.52f && sf < 0.58f)) return 5;

    sf = atanhf(-0.5f);
    if (!(sf < -0.52f && sf > -0.58f)) return 6;

    sf = acoshf(0.5f);
    if (sf == sf) return 7;

    sf = atanhf(1.5f);
    if (sf == sf) return 8;

    sf = atanhf(1.0f);
    if (!(sf > 1.0e30f)) return 9;

    sf = atanhf(-1.0f);
    if (!(sf < -1.0e30f)) return 10;

    {
        double dx = asinh(2.0);
        if (!(dx > 1.40 && dx < 1.47)) return 11;
    }

    {
        double dx = acosh(3.0);
        if (!(dx > 1.74 && dx < 1.79)) return 12;
    }

    {
        double dx = atanh(0.25);
        if (!(dx > 0.24 && dx < 0.27)) return 13;
    }

    {
        long double lx = atanhl(-0.25L);
        if (!(lx < -0.24L && lx > -0.27L)) return 14;
    }

    {
        long double lx = acoshl(1.0L);
        if (d_abs((double)lx) > 0.000001) return 15;
    }

    return 0;
}
