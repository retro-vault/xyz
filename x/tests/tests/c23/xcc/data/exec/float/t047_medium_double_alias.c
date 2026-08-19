#include "xcc_exec_test.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>

#ifndef __XCC_MODEL_M__
#error this regression must use the M-model compiler
#endif

#ifndef __XCC_DOUBLE_IS_FLOAT__
#error the M model must advertise its double/float alias
#endif

static_assert(sizeof(double) == sizeof(float),
              "M-model double must use the float representation");
static_assert(sizeof(long double) == sizeof(float),
              "M-model long double must use the float representation");
static_assert(DBL_MANT_DIG == FLT_MANT_DIG,
              "M-model double characteristics must match float");

static double subtract_unsuffixed(double value) {
    return value - 1.1;
}

static double (*static_sin_pointer)(double) = sin;

int main(void) {
    double (*sin_pointer)(double) = sin;
    double value = subtract_unsuffixed(2.1);
    const wchar_t wide_value[] = {'5', '.', '5', 0};
    const wchar_t wide_long_value[] = {'6', '.', '5', 0};

    XCC_CHECK_EQ_INT_ID(1, sizeof(double), sizeof(float));
    XCC_CHECK_EQ_INT_ID(2, sizeof(long double), sizeof(float));
    XCC_CHECK_ID(3, value > 0.9 && value < 1.1);
    XCC_CHECK_EQ_INT_ID(4, (int)sin(0.0), 0);
    XCC_CHECK_EQ_INT_ID(5, (int)sin_pointer(0.0), 0);
    XCC_CHECK_EQ_INT_ID(6, (int)strtod("2.75", 0), 2);
    XCC_CHECK_EQ_INT_ID(7, (int)strtold("3.5", 0), 3);
    XCC_CHECK_EQ_INT_ID(8, (int)atof("4.25"), 4);
    XCC_CHECK_EQ_INT_ID(9, (int)difftime((time_t)9, (time_t)4), 5);
    XCC_CHECK_EQ_INT_ID(10, (int)static_sin_pointer(0.0), 0);
    XCC_CHECK_EQ_INT_ID(11, (int)wcstod(wide_value, 0), 5);
    XCC_CHECK_EQ_INT_ID(12, (int)wcstold(wide_long_value, 0), 6);
    return 0;
}
