//
        // The mbrtoc8 and c8rtomb functions are available in <uchar.h>.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <limits.h>
#include <stdio.h>
#include <string.h>
#include <uchar.h>


        int main(void)
        {
            mbstate_t to_c8 = {0};
            mbstate_t to_mb = {0};
            char8_t c8 = 0;
            char buffer[MB_LEN_MAX];
            size_t converted = mbrtoc8(&c8, "A", 1, &to_c8);

            if (converted != 1 || c8 != (char8_t)'A')
                return 1;

            converted = c8rtomb(buffer, c8, &to_mb);
            if (converted != 1 || buffer[0] != 'A')
                return 1;

            puts("OK mbrtoc8_c8rtomb");
            return 0;
        }

