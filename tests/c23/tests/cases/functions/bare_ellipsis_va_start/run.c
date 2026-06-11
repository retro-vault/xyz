//
        // Variadic functions may use a bare ellipsis and call va_start without a named parameter.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <stdarg.h>
#include <stdio.h>


        static int sum_three(...)
        {
            va_list ap;
            int total = 0;

            va_start(ap);
            total += va_arg(ap, int);
            total += va_arg(ap, int);
            total += va_arg(ap, int);
            va_end(ap);
            return total;
        }

        int main(void)
        {
            if (sum_three(3, 7, 13) != 23)
                return 1;

            puts("OK bare_ellipsis_va_start");
            return 0;
        }

