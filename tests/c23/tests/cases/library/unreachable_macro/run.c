//
        // The unreachable feature is available through <stddef.h>.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stddef.h>
#include <stdio.h>


        int main(void)
        {
            if (0)
                unreachable();

            puts("OK unreachable_macro");
            return 0;
        }

