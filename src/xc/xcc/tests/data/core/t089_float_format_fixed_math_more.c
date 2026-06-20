float frexpf(float x, int *exp);
float ldexpf(float x, int exp);
float fmaf(float x, float y, float z);
float remquof(float x, float y, int *quo);
float hypotf(float x, float y);
int ilogbf(float x);

float fixed_math_more(float x, float y, float z, int *exp, int *quo) {
    float r = frexpf(x, exp);
    r = ldexpf(r, *exp);
    r = fmaf(r, y, z);
    r = remquof(r, y, quo);
    return hypotf(r, x);
}

int fixed_math_exp(float x) {
    return ilogbf(x);
}
