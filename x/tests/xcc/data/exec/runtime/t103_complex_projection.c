#include "xcc_exec_test.h"

#include <complex.h>
#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

typedef union complex_float_bits {
    float f;
    unsigned long u;
} complex_float_bits;

int main(void) {
    complex_float_bits pos_inf;
    complex_float_bits neg_inf;
    pos_inf.u = 0x7f800000UL;
    neg_inf.u = 0xff800000UL;

    {
        float _Complex z = CMPLXF(2.0f, -3.5f);
        float _Complex p = cprojf(z);
        if (f_abs(crealf(p) - 2.0f) > 0.001f) return 1;
        if (f_abs(cimagf(p) + 3.5f) > 0.001f) return 2;
    }

    {
        float _Complex z = CMPLXF(neg_inf.f, -2.0f);
        float _Complex p = cprojf(z);
        if (!isinf(crealf(p))) return 3;
        if (signbit(crealf(p))) return 4;
        if (cimagf(p) != 0.0f) return 5;
        if (!signbit(cimagf(p))) return 6;
    }

    {
        float _Complex z = CMPLXF(1.0f, pos_inf.f);
        float _Complex p = cprojf(z);
        if (!isinf(crealf(p))) return 7;
        if (signbit(crealf(p))) return 8;
        if (cimagf(p) != 0.0f) return 9;
        if (signbit(cimagf(p))) return 10;
    }

    {
        float _Complex z = CMPLXF(-4.0f, -0.0f);
        float _Complex p = cproj(z);
        if (f_abs(crealf(p) + 4.0f) > 0.001f) return 11;
        if (!signbit(cimagf(p))) return 12;
    }

    return 0;
}
