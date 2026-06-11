//
        // timespec_getres and the additional time-base hooks are available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <time.h>


        int main(void)
        {
            struct timespec resolution = {0};
            struct timespec current = {0};

            if (timespec_getres(&resolution, TIME_UTC) != TIME_UTC)
                return 1;

            if (resolution.tv_nsec < 0 || resolution.tv_nsec >= 1000000000L)
                return 1;

            if (timespec_get(&current, TIME_UTC) != TIME_UTC)
                return 1;

            puts("OK timespec_getres");
            return 0;
        }

