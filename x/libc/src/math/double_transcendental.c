/*
 * double_transcendental.c
 *
 * Small native double-precision cores for the handful of transcendental
 * functions currently exercised by the regression suite. These avoid the old
 * float-wrapper path so double callers can meet tighter tolerances.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#include <stddef.h>

typedef union ieee_double_bits {
    double d;
    unsigned char bytes[8];
} ieee_double_bits;

static double make_nan_double(void) {
    ieee_double_bits value;

    value.bytes[0] = 0x01u;
    value.bytes[1] = 0x00u;
    value.bytes[2] = 0x00u;
    value.bytes[3] = 0x00u;
    value.bytes[4] = 0x00u;
    value.bytes[5] = 0x00u;
    value.bytes[6] = 0xf0u;
    value.bytes[7] = 0x7fu;
    return value.d;
}

static double make_inf_double(int negative) {
    ieee_double_bits value;

    value.bytes[0] = 0x00u;
    value.bytes[1] = 0x00u;
    value.bytes[2] = 0x00u;
    value.bytes[3] = 0x00u;
    value.bytes[4] = 0x00u;
    value.bytes[5] = 0x00u;
    value.bytes[6] = 0xf0u;
    value.bytes[7] = negative ? 0xffu : 0x7fu;
    return value.d;
}

static double abs_double(double value) {
    return value < 0.0 ? -value : value;
}

static int is_zero_double(double value) {
    ieee_double_bits bits;

    bits.d = value;
    return (bits.bytes[0] | bits.bytes[1] | bits.bytes[2] | bits.bytes[3] |
            bits.bytes[4] | bits.bytes[5] | bits.bytes[6] |
            (unsigned char)(bits.bytes[7] & 0x7fu)) == 0u;
}

static int is_nan_double(double value) {
    ieee_double_bits bits;
    unsigned exp11;
    unsigned mantissa_nonzero;

    bits.d = value;
    exp11 = (unsigned)(bits.bytes[7] & 0x7fu) << 4;
    exp11 |= (unsigned)(bits.bytes[6] >> 4);
    mantissa_nonzero = (unsigned)(bits.bytes[6] & 0x0fu);
    mantissa_nonzero |= bits.bytes[5];
    mantissa_nonzero |= bits.bytes[4];
    mantissa_nonzero |= bits.bytes[3];
    mantissa_nonzero |= bits.bytes[2];
    mantissa_nonzero |= bits.bytes[1];
    mantissa_nonzero |= bits.bytes[0];
    return exp11 == 0x7ffu && mantissa_nonzero != 0u;
}

static double exp_series_core(double x) {
    double term = 1.0;
    double sum = 1.0;
    int i;

    for (i = 1; i <= 28; ++i) {
        term = term * x / (double)i;
        sum += term;
        if (abs_double(term) < 1e-15) {
            break;
        }
    }
    return sum;
}

double sqrtd_core(double x) {
    double guess;
    double next;
    int i;

    if (is_nan_double(x)) {
        return x;
    }
    if (x < 0.0) {
        return make_nan_double();
    }
    if (is_zero_double(x)) {
        return x;
    }

    guess = x > 1.0 ? x : 1.0;
    for (i = 0; i < 24; ++i) {
        next = 0.5 * (guess + x / guess);
        if (abs_double(next - guess) < 1e-15) {
            guess = next;
            break;
        }
        guess = next;
    }
    return guess;
}

double expd_core(double x) {
    double reduced;
    double result;
    int halvings = 0;
    int negative = 0;

    if (is_nan_double(x)) {
        return x;
    }
    if (x > 709.0) {
        return make_inf_double(0);
    }
    if (x < -709.0) {
        return 0.0;
    }

    if (x < 0.0) {
        negative = 1;
        x = -x;
    }

    reduced = x;
    while (reduced > 0.5) {
        reduced *= 0.5;
        ++halvings;
    }

    result = exp_series_core(reduced);
    while (halvings-- > 0) {
        result *= result;
    }

    if (negative) {
        return 1.0 / result;
    }
    return result;
}

double logd_core(double x) {
    static const double ln2 = 0.69314718055994530942;
    double t;
    double t2;
    double term;
    double sum;
    int exponent2 = 0;
    int denom;

    if (is_nan_double(x)) {
        return x;
    }
    if (x < 0.0) {
        return make_nan_double();
    }
    if (is_zero_double(x)) {
        return make_inf_double(1);
    }

    while (x > 1.5) {
        x *= 0.5;
        ++exponent2;
    }
    while (x < 0.75) {
        x *= 2.0;
        --exponent2;
    }

    t = (x - 1.0) / (x + 1.0);
    t2 = t * t;
    term = t;
    sum = term;

    for (denom = 3; denom <= 39; denom += 2) {
        term *= t2;
        sum += term / (double)denom;
    }

    return 2.0 * sum + (double)exponent2 * ln2;
}

double powd_core(double x, double y) {
    long integral_exp;
    double base;
    double result;
    int negative_result = 0;

    if (is_nan_double(x) || is_nan_double(y)) {
        return make_nan_double();
    }
    if (is_zero_double(y)) {
        return 1.0;
    }
    if (is_zero_double(x)) {
        return y > 0.0 ? 0.0 : make_inf_double(0);
    }

    integral_exp = (long)y;
    if ((double)integral_exp == y) {
        long exp_left = integral_exp;

        base = x;
        if (exp_left < 0) {
            exp_left = -exp_left;
            base = 1.0 / base;
        }

        if (base < 0.0) {
            negative_result = (exp_left & 1l) != 0l;
            base = -base;
        }

        result = 1.0;
        while (exp_left > 0) {
            if ((exp_left & 1l) != 0l) {
                result *= base;
            }
            base *= base;
            exp_left >>= 1;
        }
        return negative_result ? -result : result;
    }

    if (x < 0.0) {
        return make_nan_double();
    }
    return expd_core(logd_core(x) * y);
}
