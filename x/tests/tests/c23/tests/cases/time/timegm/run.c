//
        // The timegm function is available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <time.h>


        int main(void)
        {
            struct tm value = {0};
            time_t result;

            value.tm_year = 70;
            value.tm_mon = 0;
            value.tm_mday = 1;
            result = timegm(&value);

            if (result != 0)
                return 1;

            puts("OK timegm");
            return 0;
        }

