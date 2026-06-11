//
        // Extended month-name formats added through POSIX harmonization are available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <locale.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


        int main(void)
        {
            struct tm value = {0};
            char full_month[32];
            char alt_month[32];

            if (setlocale(LC_TIME, "C") == NULL)
                return 1;

            value.tm_year = 124;
            value.tm_mon = 2;
            value.tm_mday = 1;

            if (strftime(full_month, sizeof(full_month), "%B", &value) == 0)
                return 1;

            if (strftime(alt_month, sizeof(alt_month), "%OB", &value) == 0)
                return 1;

            if (strcmp(full_month, "March") != 0)
                return 1;

            if (strcmp(alt_month, "March") != 0)
                return 1;

            puts("OK strftime_extended_month_formats");
            return 0;
        }

