#include "xcc_exec_test.h"

#include <complex.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

int main(void) {
    {
        float _Complex z = casinf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 1;
        if (f_abs(cimagf(z)) > 0.25f) return 2;
    }

    {
        float _Complex z = cacosf(CMPLXF(1.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 3;
        if (f_abs(cimagf(z)) > 0.25f) return 4;
    }

    {
        float _Complex z = catanf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 5;
        if (f_abs(cimagf(z)) > 0.25f) return 6;
    }

    {
        float _Complex z = casinhf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 7;
        if (f_abs(cimagf(z)) > 0.25f) return 8;
    }

    {
        float _Complex z = cacoshf(CMPLXF(1.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 9;
        if (f_abs(cimagf(z)) > 0.25f) return 10;
    }

    {
        float _Complex z = catanhf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z)) > 0.25f) return 11;
        if (f_abs(cimagf(z)) > 0.25f) return 12;
    }

    return 0;
}
