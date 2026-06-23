//
        // The nullptr constant and nullptr_t type behave as null pointer values.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


        int main(void)
        {
            nullptr_t np = nullptr;
            int *pointer = np;

            if ((bool)np)
                return 1;

            if (pointer != NULL)
                return 1;

            puts("OK nullptr_and_nullptr_t");
            return 0;
        }

