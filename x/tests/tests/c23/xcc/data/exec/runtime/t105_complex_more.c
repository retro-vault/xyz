#include "xcc_exec_test.h"

#include <complex.h>
#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

int main(void) {
    {
        float _Complex z = csinf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.01f) return 1;
        if (f_abs(cimagf(z)) > 0.01f) return 2;
    }

    {
        float _Complex z = ccosf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z) - 1.0f) > 0.01f) return 3;
        if (f_abs(cimagf(z)) > 0.01f) return 4;
    }

    {
        float _Complex z = ctanf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.01f) return 5;
        if (f_abs(cimagf(z)) > 0.01f) return 6;
    }

    {
        float _Complex z = csinhf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.01f) return 7;
        if (f_abs(cimagf(z)) > 0.01f) return 8;
    }

    {
        float _Complex z = ccoshf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z) - 1.0f) > 0.01f) return 9;
        if (f_abs(cimagf(z)) > 0.01f) return 10;
    }

    {
        float _Complex z = ctanhf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.01f) return 11;
        if (f_abs(cimagf(z)) > 0.01f) return 12;
    }

    {
        float _Complex z = cpowf(CMPLXF(2.0f, 0.0f), CMPLXF(3.0f, 0.0f));
        if (f_abs(crealf(z) - 8.0f) > 0.08f) return 13;
        if (f_abs(cimagf(z)) > 0.08f) return 14;
    }

    {
        float _Complex z = cpow(CMPLXF(1.0f, 0.0f), CMPLXF(0.5f, 2.0f));
        if (f_abs(crealf(z) - 1.0f) > 0.02f) return 15;
        if (f_abs(cimagf(z)) > 0.02f) return 16;
    }

    return 0;
}
