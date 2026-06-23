//
        // The basic source character set additions can appear in character and string literals.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <string.h>


        int main(void)
        {
            char text[] = {'@', '$', '`', '\0'};

            if (strcmp(text, "@$`") != 0)
                return 1;

            puts("OK basic_character_set_additions");
            return 0;
        }

