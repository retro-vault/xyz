//
        // The new IEC 60559 minimum and maximum functions are available when claimed.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <math.h>
#include <stdio.h>


        #ifdef __STDC_IEC_60559_BFP__
        int main(void)
        {
            if (fmaximum(1.0, 2.0) != 2.0)
                return 1;

            if (fminimum(1.0, 2.0) != 1.0)
                return 1;

            puts("OK iec60559_minmax");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_minmax");
            return 0;
        }
        #endif

