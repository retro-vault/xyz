#include "xcc_exec_test.h"

#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

static double d_abs(double x) {
    return x < 0.0 ? -x : x;
}

int main(void) {
    float sf = erff(0.0f);
    if (f_abs(sf) > 0.001f) return 1;

    sf = erff(1.0f);
    if (!(sf > 0.82f && sf < 0.87f)) return 2;

    sf = erff(-1.0f);
    if (!(sf < -0.82f && sf > -0.87f)) return 3;

    sf = erfcf(0.0f);
    if (!(sf > 0.99f && sf < 1.01f)) return 4;

    sf = erfcf(1.0f);
    if (!(sf > 0.14f && sf < 0.18f)) return 5;

    sf = lgammaf(1.0f);
    if (f_abs(sf) > 0.05f) return 6;

    sf = lgammaf(5.0f);
    if (!(sf > 3.10f && sf < 3.25f)) return 7;

    sf = tgammaf(1.0f);
    if (!(sf > 0.99f && sf < 1.01f)) return 8;

    sf = tgammaf(5.0f);
    if (!(sf > 23.0f && sf < 25.0f)) return 9;

    sf = tgammaf(0.5f);
    if (!(sf > 1.65f && sf < 1.90f)) return 10;

    {
        double dx = erf(1.0);
        if (!(dx > 0.82 && dx < 0.87)) return 11;
    }

    {
        double dx = erfc(1.0);
        if (!(dx > 0.14 && dx < 0.18)) return 12;
    }

    {
        double dx = lgamma(5.0);
        if (!(dx > 3.10 && dx < 3.25)) return 13;
    }

    {
        double dx = tgamma(5.0);
        if (!(dx > 23.0 && dx < 25.0)) return 14;
    }

    {
        long double lx = tgammal(0.5L);
        if (!(lx > 1.65L && lx < 1.90L)) return 15;
    }

    {
        long double lx = erfcl(1.0L);
        if (!(lx > 0.14L && lx < 0.18L)) return 16;
    }

    {
        double dx = lgammal(1.0L);
        if (d_abs(dx) > 0.05) return 17;
    }

    return 0;
}
