//
        // The memset_explicit function is available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <string.h>


        int main(void)
        {
            unsigned char buffer[6] = "secret";
            size_t i = 0;

            memset_explicit(buffer, 0xA5, sizeof(buffer));

            for (i = 0; i < sizeof(buffer); ++i) {
                if (buffer[i] != 0xA5)
                    return 1;
            }

            puts("OK memset_explicit");
            return 0;
        }

