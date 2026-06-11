//
        // Signed _BitInt and BITINT_MAXWIDTH are available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <stdio.h>


        #if !defined(BITINT_MAXWIDTH)
        #error BITINT_MAXWIDTH must be defined in C23.
        #endif

        int main(void)
        {
            _BitInt(9) left = 200;
            _BitInt(9) right = 23;
            _BitInt(10) total = left + right;

            if (BITINT_MAXWIDTH < 9)
                return 1;

            if (total != 223)
                return 1;

            puts("OK bit_precise_integers");
            return 0;
        }

