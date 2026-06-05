/*
 * complex.c
 *
 * Small complex helper subset for the xcc Z80 libc.
 *
 * The compiler already lowers complex arithmetic operators directly. This
 * file supplies the standard library helpers that layer on top of the current
 * accessors and math subset.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <complex.h>
#include <math.h>

float _Complex conjf(float _Complex value)
{
    return CMPLXF(crealf(value), -cimagf(value));
}

float cabsf(float _Complex value)
{
    float real;
    float imag;

    real = crealf(value);
    imag = cimagf(value);
    return sqrtf(real * real + imag * imag);
}

float cargf(float _Complex value)
{
    return atan2f(cimagf(value), crealf(value));
}
