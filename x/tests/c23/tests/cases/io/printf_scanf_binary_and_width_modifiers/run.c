//
        // Binary formatting and the wN length modifiers are available for printf and scanf.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


        int main(void)
        {
            char buffer[64];
            unsigned int parsed_binary = 0;
            int8_t parsed_small = 0;

            if (snprintf(buffer, sizeof(buffer), "%#" PRIb8 " %w8d", (uint8_t)10, (int8_t)-5) < 0)
                return 1;

            if (strcmp(buffer, "0b1010 -5") != 0)
                return 1;

            if (sscanf("1010 -5", "%b %w8d", &parsed_binary, &parsed_small) != 2)
                return 1;

            if (parsed_binary != 10u || parsed_small != -5)
                return 1;

            puts("OK printf_scanf_binary_and_width_modifiers");
            return 0;
        }

