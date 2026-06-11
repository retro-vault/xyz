//
        // u8, u, and U string literals use UTF-8, UTF-16, and UTF-32 code units.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdint.h>
#include <stdio.h>
#include <uchar.h>


        int main(void)
        {
            char8_t utf8_text[] = u8"\u00A2";
            char16_t utf16_text[] = u"\u00A2";
            char32_t utf32_text[] = U"\u00A2";

            if (sizeof(utf8_text) != 3)
                return 1;

            if (utf8_text[0] != 0xC2 || utf8_text[1] != 0xA2 || utf8_text[2] != 0)
                return 1;

            if (utf16_text[0] != 0x00A2 || utf16_text[1] != 0)
                return 1;

            if (utf32_text[0] != 0x000000A2 || utf32_text[1] != 0)
                return 1;

            puts("OK utf_string_encodings");
            return 0;
        }

