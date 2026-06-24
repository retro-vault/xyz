//
        // The memccpy function is available through <string.h>.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <string.h>


        int main(void)
        {
            char dst[8] = {0};
            const char src[] = "abcd";
            void *stop = memccpy(dst, src, 'c', sizeof(src));

            if (stop != &dst[3])
                return 1;

            if (memcmp(dst, "abc", 3) != 0)
                return 1;

            puts("OK memccpy");
            return 0;
        }

