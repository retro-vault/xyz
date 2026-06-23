//
        // call_once support is mandated and usable.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdio.h>
#include <threads.h>


        static once_flag global_once = ONCE_FLAG_INIT;
        static int calls = 0;

        static void initialize_once(void)
        {
            ++calls;
        }

        int main(void)
        {
            call_once(&global_once, initialize_once);
            call_once(&global_once, initialize_once);

            if (calls != 1)
                return 1;

            puts("OK call_once_mandated");
            return 0;
        }

