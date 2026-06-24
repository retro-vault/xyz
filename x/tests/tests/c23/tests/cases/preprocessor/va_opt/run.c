//
        // The __VA_OPT__ preprocessor feature handles optional commas.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <string.h>


        #define APPEND(buffer, format, ...) \
            snprintf((buffer), sizeof(buffer), (format) __VA_OPT__(,) __VA_ARGS__)

        int main(void)
        {
            char first[32];
            char second[32];

            APPEND(first, "%s", "hello");
            APPEND(second, "%s %d", "value", 23);

            if (strcmp(first, "hello") != 0)
                return 1;

            if (strcmp(second, "value 23") != 0)
                return 1;

            puts("OK va_opt");
            return 0;
        }

