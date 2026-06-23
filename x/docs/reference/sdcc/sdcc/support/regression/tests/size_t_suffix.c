/*
 * test case for C2y literal suffixes for size_t
 */

#include <testfwk.h>
#include <stddef.h>

#ifdef __SDCC
_Pragma("std_sdcc23")
#endif

void
testSizeTSuffix(void)
{
#ifdef __SDCC
    size_t a = 42uz + 0zu + 0UZ + 0ZU;
    ptrdiff_t b = -21z + 42Z;

    ASSERT(a == 42);
    ASSERT(b == 21);
    ASSERT(sizeof(size_t) == sizeof(0uz));
    ASSERT(sizeof(ptrdiff_t) == sizeof(0z));
#endif
}
