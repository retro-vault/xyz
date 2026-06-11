//
        // The memalignment function reports pointer alignment.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>


        int main(void)
        {
            alignas(64) unsigned char buffer[64];
            size_t alignment = memalignment(buffer);

            if (alignment < 64)
                return 1;

            puts("OK memalignment");
            return 0;
        }

