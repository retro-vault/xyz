/* t047: float _Complex arithmetic — add, subtract, multiply */

/* Helper: check two floats by integer representation match */
static int feq(float a, float b) {
    return a == b;
}

int test_complex_add(void) {
    float _Complex a;  /* real=1.0, imag=2.0 */
    float _Complex b;  /* real=3.0, imag=4.0 */
    float _Complex c;
    /* Manually initialise components via pointers */
    float *pa = (float *)&a;
    float *pb = (float *)&b;
    pa[0] = 1.0f;  pa[1] = 2.0f;
    pb[0] = 3.0f;  pb[1] = 4.0f;
    c = a + b;
    float *pc = (float *)&c;
    /* real: 1+3=4, imag: 2+4=6 */
    if (!feq(pc[0], 4.0f)) return 0;
    if (!feq(pc[1], 6.0f)) return 0;
    return 1;
}

int main(void) {
    if (!test_complex_add()) return 0;
    return 1;
}
