//
        // The new <stdckdint.h> header exposes checked integer operations.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <stdckdint.h>
#include <stdio.h>


        int main(void)
        {
            int result = 0;

            if (ckd_add(&result, 20, 3))
                return 1;

            if (result != 23)
                return 1;

            if (!ckd_add(&result, INT_MAX, 1))
                return 1;

            puts("OK stdckdint_header");
            return 0;
        }

