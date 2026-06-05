/*
 * math.c
 *
 * Small soft-float math subset for the xcc Z80 libc.
 *
 * The current target exposes classification helpers plus compact freestanding
 * implementations of fabs, copysign, sqrt, and atan2. The transcendental
 * coverage is intentionally narrow until the wider soft-float runtime grows.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <errno.h>
#include <math.h>

typedef union __libc_float_bits {
    float         value;
    unsigned long bits;
} __libc_float_bits;

static unsigned long __libc_float_raw(float value)
{
    __libc_float_bits raw;

    raw.value = value;
    return raw.bits;
}

static float __libc_float_from_bits(unsigned long bits)
{
    __libc_float_bits raw;

    raw.bits = bits;
    return raw.value;
}

static float __libc_nanf_value(void)
{
    return __libc_float_from_bits(0x7fc00000UL);
}

int __libc_fpclassifyf(float value)
{
    unsigned long bits;
    unsigned long exponent;
    unsigned long fraction;

    bits = __libc_float_raw(value);
    exponent = (bits >> 23) & 0xffUL;
    fraction = bits & 0x7fffffUL;

    if (exponent == 0xffUL) {
        return fraction == 0UL ? FP_INFINITE : FP_NAN;
    }
    if (exponent == 0UL) {
        return fraction == 0UL ? FP_ZERO : FP_SUBNORMAL;
    }
    return FP_NORMAL;
}

int __libc_signbitf(float value)
{
    return (__libc_float_raw(value) & 0x80000000UL) != 0UL;
}

int __libc_isfinitef(float value)
{
    return __libc_fpclassifyf(value) != FP_INFINITE &&
           __libc_fpclassifyf(value) != FP_NAN;
}

int __libc_isinff(float value)
{
    return __libc_fpclassifyf(value) == FP_INFINITE;
}

int __libc_isnanf(float value)
{
    return __libc_fpclassifyf(value) == FP_NAN;
}

float fabsf(float value)
{
    return __libc_float_from_bits(__libc_float_raw(value) & 0x7fffffffUL);
}

double fabs(double value)
{
    return (double)fabsf((float)value);
}

long double fabsl(long double value)
{
    return (long double)fabsf((float)value);
}

float copysignf(float mag, float sign)
{
    unsigned long mag_bits;
    unsigned long sign_bits;

    mag_bits = __libc_float_raw(mag) & 0x7fffffffUL;
    sign_bits = __libc_float_raw(sign) & 0x80000000UL;
    return __libc_float_from_bits(mag_bits | sign_bits);
}

double copysign(double mag, double sign)
{
    return (double)copysignf((float)mag, (float)sign);
}

long double copysignl(long double mag, long double sign)
{
    return (long double)copysignf((float)mag, (float)sign);
}

float sqrtf(float value)
{
    float guess;
    float next;
    int i;

    if (__libc_isnanf(value) || value == 0.0f || __libc_isinff(value)) {
        return value;
    }
    if (value < 0.0f) {
        errno = EDOM;
        return __libc_nanf_value();
    }

    guess = value > 1.0f ? value : 1.0f;
    i = 0;
    while (i < 8) {
        next = 0.5f * (guess + (value / guess));
        if (next == guess) {
            break;
        }
        guess = next;
        ++i;
    }

    return guess;
}

double sqrt(double value)
{
    return (double)sqrtf((float)value);
}

long double sqrtl(long double value)
{
    return (long double)sqrtf((float)value);
}

float atan2f(float y, float x)
{
    float z;
    float atan;
    float abs_z;

    if (__libc_isnanf(x) || __libc_isnanf(y)) {
        return __libc_nanf_value();
    }

    if (x == 0.0f) {
        if (y > 0.0f) {
            return 0.5f * (float)M_PI;
        }
        if (y < 0.0f) {
            return -0.5f * (float)M_PI;
        }
        return 0.0f;
    }

    z = y / x;
    abs_z = fabsf(z);

    if (abs_z < 1.0f) {
        atan = z / (1.0f + 0.28f * z * z);
        if (x < 0.0f) {
            if (y < 0.0f) {
                atan -= (float)M_PI;
            } else {
                atan += (float)M_PI;
            }
        }
        return atan;
    }

    atan = (0.5f * (float)M_PI) - (z / (z * z + 0.28f));
    if (y < 0.0f) {
        atan -= (float)M_PI;
    }
    return atan;
}

double atan2(double y, double x)
{
    return (double)atan2f((float)y, (float)x);
}

long double atan2l(long double y, long double x)
{
    return (long double)atan2f((float)y, (float)x);
}
