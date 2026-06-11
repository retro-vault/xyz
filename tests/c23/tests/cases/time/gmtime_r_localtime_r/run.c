//
        // The gmtime_r and localtime_r functions are available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <time.h>


        int main(void)
        {
            time_t epoch = 0;
            struct tm utc_result;
            struct tm local_result;

            if (gmtime_r(&epoch, &utc_result) == NULL)
                return 1;

            if (localtime_r(&epoch, &local_result) == NULL)
                return 1;

            if (utc_result.tm_year != 70 || utc_result.tm_mon != 0 || utc_result.tm_mday != 1)
                return 1;

            if (mktime(&local_result) == (time_t)-1)
                return 1;

            puts("OK gmtime_r_localtime_r");
            return 0;
        }

