//
        // PTRDIFF_WIDTH reflects the C23 minimum width rule.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stddef.h>
#include <stdio.h>


        static_assert(PTRDIFF_WIDTH >= 16);

        int main(void)
        {
            puts("OK ptrdiff_width_floor");
            return 0;
        }

