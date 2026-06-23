#include "xcc_exec_test.h"

#include <complex.h>
#include <math.h>

static float f_abs(float x) {
    return x < 0.0f ? -x : x;
}

int main(void) {
    {
        float _Complex z = cexpf(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z) - 1.0f) > 0.001f) return 1;
        if (f_abs(cimagf(z)) > 0.001f) return 2;
    }

    {
        float _Complex input = CMPLXF(1.0f, 0.0f);
        float _Complex z = clogf(input);
        if (f_abs(crealf(z) - logf(cabsf(input))) > 0.001f) return 3;
        if (f_abs(cimagf(z) - cargf(input)) > 0.001f) return 4;
    }

    {
        float _Complex z = csqrtf(CMPLXF(4.0f, 0.0f));
        if (f_abs(crealf(z) - 2.0f) > 0.02f) return 5;
        if (f_abs(cimagf(z)) > 0.02f) return 6;
    }

    {
        float _Complex z = cexp(CMPLXF(0.0f, 0.0f));
        if (f_abs(crealf(z) - 1.0f) > 0.001f) return 7;
        if (f_abs(cimagf(z)) > 0.001f) return 8;
    }

    return 0;
}
