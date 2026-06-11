//
        // u8 character literals and char8_t are supported.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <uchar.h>


        int main(void)
        {
            char8_t text[] = u8"AB";
            static_assert(_Generic((u8'A'), char8_t: 1, default: 0));

            if (sizeof(text) != 3)
                return 1;

            if (text[0] != (char8_t)'A' || text[1] != (char8_t)'B')
                return 1;

            puts("OK char8_t_and_u8_char");
            return 0;
        }

