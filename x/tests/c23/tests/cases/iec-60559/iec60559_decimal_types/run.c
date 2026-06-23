//
// Decimal floating-point TS integration exposes _DecimalN types when claimed.
// Generated from tests/spec/c23_suite.py.
//
// This file is part of the C23 compatibility suite.
//

#include <stdio.h>


#ifdef __STDC_IEC_60559_DFP__
int main(void)
{
    _Decimal64 left = 1.20dd;
    _Decimal64 right = 2.30dd;
    _Decimal64 total = left + right;

    if (total != 3.50dd)
        return 1;

    puts("OK iec60559_decimal_types");
    return 0;
}
#else
int main(void)
{
    puts("NOT-CLAIMED iec60559_decimal_types");
    return 0;
}
#endif

