//
        // The suite probes the C23 integer model assumptions that removed legacy sign representations.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <stdio.h>
#include <string.h>


        int main(void)
        {
            signed char negative_one = -1;
            unsigned char raw = 0;

            memcpy(&raw, &negative_one, sizeof(raw));

            if (raw != UCHAR_MAX)
                return 1;

            puts("OK integer_model_probe");
            return 0;
        }

