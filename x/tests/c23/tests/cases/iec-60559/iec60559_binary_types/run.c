//
// Binary floating-point TS integration exposes _FloatN types when claimed.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#ifdef __STDC_IEC_60559_TYPES__
int main(void)
{
    _Float32 left = (_Float32)1.5;
    _Float64 right = (_Float64)2.25;
    double total = (double)(left + (_Float32)right);

    if (total <= 3.74 || total >= 3.76)
        return 1;

    puts("OK iec60559_binary_types");
    return 0;
}
#else
int main(void)
{
    puts("NOT-CLAIMED iec60559_binary_types");
    return 0;
}
#endif

