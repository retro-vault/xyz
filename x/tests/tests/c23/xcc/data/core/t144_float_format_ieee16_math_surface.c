#include <math.h>

float ieee16_surface_core(float x, float y, float z) {
    int exp = 0;
    float whole = 0.0f;
    float payload = getpayloadf(&x);
    float a = sinf(x) + cosf(y) + tanf(z);
    float b = atan2f(x, y) + powf(a, z);
    float c = expf(x) + exp2f(y) + expm1f(z);
    float d = logf(x) + log2f(y) + log10f(z) + log1pf(a);
    float e = cbrtf(a) + erff(b) + erfcf(c) + tgammaf(y) + lgammaf(z);
    float f = roundevenf(b) + nearbyintf(c) + rintf(d);
    float g = frexpf(e, &exp) + ldexpf(a, exp);
    float h = modff(f, &whole) + scalbnf(g, 1) + scalblnf(g, 2L);
    return payload + h + fmaf(a, b, c) + nextafterf(x, y);
}

long ieee16_surface_ints(float x) {
    return lroundf(x) + lrintf(x) + ilogbf(x);
}

int ieee16_surface_payload(float *x, float y) {
    return setpayloadf(x, y) +
           setpayloadsigf(x, y) +
           totalorderf(*x, y) +
           totalordermagf(*x, y);
}
