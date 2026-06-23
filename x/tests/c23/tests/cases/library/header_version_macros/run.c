//
        // Changed library headers define their C23 version macros.
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        #include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbit.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>
#include <time.h>
#include <uchar.h>
#include <wchar.h>


        #define CHECK_VERSION(name) \
            do { \
                if ((name) != 202311L) \
                    return 1; \
            } while (0)

        int main(void)
        {
            CHECK_VERSION(__STDC_VERSION_MATH_H__);
            CHECK_VERSION(__STDC_VERSION_SETJMP_H__);
            CHECK_VERSION(__STDC_VERSION_STDARG_H__);
            CHECK_VERSION(__STDC_VERSION_STDATOMIC_H__);
            CHECK_VERSION(__STDC_VERSION_STDBIT_H__);
            CHECK_VERSION(__STDC_VERSION_STDCKDINT_H__);
            CHECK_VERSION(__STDC_VERSION_STDDEF_H__);
            CHECK_VERSION(__STDC_VERSION_STDINT_H__);
            CHECK_VERSION(__STDC_VERSION_STDIO_H__);
            CHECK_VERSION(__STDC_VERSION_STDLIB_H__);
            CHECK_VERSION(__STDC_VERSION_STRING_H__);
            CHECK_VERSION(__STDC_VERSION_TGMATH_H__);
            CHECK_VERSION(__STDC_VERSION_TIME_H__);
            CHECK_VERSION(__STDC_VERSION_UCHAR_H__);
            CHECK_VERSION(__STDC_VERSION_WCHAR_H__);

            puts("OK header_version_macros");
            return 0;
        }

