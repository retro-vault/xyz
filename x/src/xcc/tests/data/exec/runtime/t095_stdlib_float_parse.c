#include "xcc_exec_test.h"
#include <stdlib.h>

static unsigned strtof_endoff(const char *base) {
    char *end = 0;
    (void)strtof(base, &end);
    return (unsigned)(end - base);
}

static int strtod_int(const char *base) {
    return (int)strtod(base, 0);
}

int main(void) {
    char *end;
    const char *base;
    float f;
    double d;
    long double ld;

    end = 0;
    base = "123.5";
    f = strtof(base, &end);
    if ((int)f != 123) return 1;
    if (*end != '\0') return 2;

    end = 0;
    base = "-0.75e2tail";
    f = strtof(base, &end);
    if ((int)f != -75) return 3;
    if (*end != 't') return 4;

    d = strtod("6.25e-3rest", 0);
    if (!(d > 0.0 && d < 0.01)) return 6;

    if (strtod_int("001.5tail") != 1) return 8;

    d = atof("2.75");
    if ((int)d != 2) return 9;

    ld = strtold("0", 0);
    if ((double)ld != 0.0) return 10;

    if (strtod_int("  +42.5x") != 42) return 12;

    if (strtof_endoff("xyz") != 0u) return 14;
    d = strtod("xyz", 0);
    if (d != 0.0) return 15;

    return 0;
}
