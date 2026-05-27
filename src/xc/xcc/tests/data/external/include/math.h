#ifndef _MATH_H
#define _MATH_H

double sin(double);
double cos(double);
double tan(double);
double asin(double);
double acos(double);
double atan(double);
double atan2(double, double);
double sqrt(double);
double pow(double, double);
double exp(double);
double log(double);
double log10(double);
double fabs(double);
double ceil(double);
double floor(double);
double fmod(double, double);

float sinf(float);
float cosf(float);
float sqrtf(float);
float fabsf(float);
float floorf(float);
float ceilf(float);

#define M_PI   3.14159265358979323846
#define M_E    2.71828182845904523536
#define HUGE_VAL (1e10000)

#endif
