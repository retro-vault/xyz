//
        // Mixed wide and narrow literal concatenation is supported.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <wchar.h>


        int main(void)
        {
            const wchar_t *text = L"A" "B";

            if (text[0] != L'A' || text[1] != L'B' || text[2] != L'\0')
                return 1;

            puts("OK mixed_string_concatenation");
            return 0;
        }

