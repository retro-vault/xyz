//
        // The new <stdbit.h> header exposes C23 bit utilities.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <stdbit.h>
#include <stdio.h>


        int main(void)
        {
            unsigned int value = 0b1011000u;

            if (stdc_count_ones(value) != 3)
                return 1;

            if (stdc_bit_width(value) != 7u)
                return 1;

            if (stdc_has_single_bit(8u) != 1)
                return 1;

            if (stdc_bit_floor(10u) != 8u)
                return 1;

            if (stdc_bit_ceil(10u) != 16u)
                return 1;

            if (stdc_rotate_left(1u, 3) != 8u)
                return 1;

            puts("OK stdbit_header");
            return 0;
        }

