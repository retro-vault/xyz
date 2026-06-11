//
        // The strdup and strndup functions are available.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <stdlib.h>
#include <string.h>


        int main(void)
        {
            char *full = strdup("hello");
            char *part = strndup("hello", 3);
            int failed = 0;

            if (full == NULL || part == NULL)
                failed = 1;

            if (!failed && strcmp(full, "hello") != 0)
                failed = 1;

            if (!failed && strcmp(part, "hel") != 0)
                failed = 1;

            free(full);
            free(part);

            if (failed)
                return 1;

            puts("OK strdup_strndup");
            return 0;
        }

