//
        // Variably modified types are supported independently of VLA objects.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stddef.h>
#include <stdio.h>


        static int array_sum(size_t count, int (*values)[count])
        {
            int total = 0;
            size_t i = 0;

            for (i = 0; i < count; ++i)
                total += (*values)[i];

            return total;
        }

        int main(void)
        {
            int values[4] = {1, 2, 3, 4};

            if (array_sum(4, &values) != 10)
                return 1;

            puts("OK vm_types_mandatory");
            return 0;
        }

