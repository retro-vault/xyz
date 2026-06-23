#include "xcc_exec_test.h"

static double dadd(double a, double b) {
    return a + b;
}

static double dmul(double a, double b) {
    return a * b;
}

static int dlt(double a, double b) {
    return a < b;
}

static long long d2ll(double x) {
    return (long long)x;
}

static unsigned long long d2ull(double x) {
    return (unsigned long long)x;
}

static double ll2d(long long x) {
    return (double)x;
}

static double ull2d(unsigned long long x) {
    return (double)x;
}

static float d2f(double x) {
    return (float)x;
}

static double f2d(float x) {
    return (double)x;
}

int main(void) {
    double sum = dadd(3.5, 2.25);
    unsigned long long big = ((unsigned long long)1 << 48) + 0x1234ULL;
    long long signed_big = ((long long)1 << 40) + 12345LL;
    long double ld = (long double)4.75;
    if (d2ll(dmul(sum, 4.0)) != 23LL) return 1;
    if (!dlt(-1.0, 0.0)) return 2;
    if (dlt(4.0, 4.0)) return 3;
    if (d2ll(ll2d(signed_big)) != signed_big) return 4;
    if (d2ull(ull2d(big)) != big) return 5;
    if (d2ll(f2d((float)9.5f) * 2.0) != 19LL) return 6;
    if (d2ll((double)d2f(3.25) * 4.0) != 13LL) return 7;
    if (d2ll((double)ld * 4.0) != 19LL) return 8;

    return 0;
}
