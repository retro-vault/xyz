/** qualifier-preserving.c: Test C23's qualifier-preserving wrapper macros for string functions.
 */

#ifdef __SDCC
_Pragma("std_c23")
#undef __STDC_VERSION__
#define __STDC_VERSION__ 202311L
#endif

#include <string.h>

#include <testfwk.h>

void testQualifierPreserving(void)
{
#if __STDC_VERSION_STRING_H__ >= 202311L
    // test char and void pointer representatives -- the wrappers all use the same mechanism
    char abc[] = "abc";
    ASSERT(memchr(abc, 'c', 3) == abc + 2);
    ASSERT(_Generic(memchr(abc, 'c', 3), void *: 1, const void *: 2, default: 3) == 1);
    ASSERT(strchr(abc, 'c') == abc + 2);
    ASSERT(_Generic(strchr(abc, 'c'), char *: 1, const char *: 2, default: 3) == 1);
    const char ABC[] = "ABC";
    ASSERT(memchr(ABC, 'C', 3) == ABC + 2);
    ASSERT(_Generic(memchr(ABC, 'C', 3), void *: 1, const void *: 2, default: 3) == 2);
    ASSERT(strchr(ABC, 'C') == ABC + 2);
    ASSERT(_Generic(strchr(ABC, 'C'), char *: 1, const char *: 2, default: 3) == 2);
#endif
}

