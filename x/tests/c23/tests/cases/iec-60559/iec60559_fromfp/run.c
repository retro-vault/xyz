//
        // The fromfp and ufromfp functions are available when IEC 60559 binary floating-point is claimed.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <math.h>
#include <stdio.h>


        #ifdef __STDC_IEC_60559_BFP__
        int main(void)
        {
            if (fromfp(2.25, FP_INT_DOWNWARD, INT_WIDTH) != 2)
                return 1;

            if (ufromfp(3.75, FP_INT_TOWARDZERO, UINT_WIDTH) != 3u)
                return 1;

            puts("OK iec60559_fromfp");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_fromfp");
            return 0;
        }
        #endif

