#include "xcc_exec_test.h"

static volatile unsigned int trace;

#define STEP1() trace += 3u;
#define STEP2() STEP1() STEP1()
#define STEP4() STEP2() STEP2()
#define STEP8() STEP4() STEP4()
#define STEP16() STEP8() STEP8()
#define STEP32() STEP16() STEP16()
#define STEP64() STEP32() STEP32()
#define STEP128() STEP64() STEP64()
#define STEP256() STEP128() STEP128()
#define STEP512() STEP256() STEP256()

static __attribute__((noinline)) unsigned int
large_dispatch(unsigned int selector)
{
    /* Keep this above the backend's expensive-structured-matcher cutoff. */
    STEP512()
    if (selector == 0u)
        return 100u;
    switch (selector) {
    case 1: return 101u;
    case 2: return 102u;
    case 3: return 103u;
    case 4: return 104u;
    case 5: return 105u;
    case 6: return 106u;
    case 7: return 107u;
    case 8: return 108u;
    case 9: return 109u;
    case 10: return 110u;
    case 11: return 111u;
    case 12: return 112u;
    case 13: return 113u;
    case 14: return 114u;
    case 15: return 115u;
    case 16: return 116u;
    case 17: return 117u;
    case 18: return 118u;
    case 19: return 119u;
    case 20: return 120u;
    case 21: return 121u;
    case 22: return 122u;
    case 23: return 123u;
    case 24: return 124u;
    case 25: return 125u;
    case 26: return 126u;
    case 27: return 127u;
    case 28: return 128u;
    case 29: return 129u;
    case 30: return 130u;
    case 31: return 131u;
    default: return 999u;
    }
}

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, large_dispatch(0u), 100u);
    XCC_CHECK_EQ_UINT_ID(2, large_dispatch(1u), 101u);
    XCC_CHECK_EQ_UINT_ID(3, large_dispatch(17u), 117u);
    XCC_CHECK_EQ_UINT_ID(4, large_dispatch(31u), 131u);
    XCC_CHECK_EQ_UINT_ID(5, large_dispatch(32u), 999u);
    XCC_CHECK_EQ_UINT_ID(6, trace, 7680u);
    return 0;
}
