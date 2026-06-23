//
        // The [[noreturn]] attribute is accepted on functions.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <setjmp.h>
#include <stdio.h>


        static jmp_buf jump_buffer;

        [[noreturn]] static void leave_now(void)
        {
            longjmp(jump_buffer, 1);
        }

        int main(void)
        {
            if (setjmp(jump_buffer) == 0) {
                leave_now();
                return 1;
            }

            puts("OK attr_noreturn");
            return 0;
        }

