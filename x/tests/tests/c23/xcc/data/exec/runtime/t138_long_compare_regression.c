#include "xcc_exec_test.h"

#include <stdint.h>

struct mixed_ints {
    int8_t   i8;
    uint16_t u16;
    int32_t  i32;
    int64_t  i64;
};

int main(void) {
    struct mixed_ints s = { -5, 40000u, 123456789L, 987654321012345LL };
    int32_t local = s.i32;
    int32_t neg = -123456789L;

    XCC_CHECK_EQ_INT_ID(1, s.i32 > 0, 1);
    XCC_CHECK_EQ_INT_ID(2, local > 0, 1);
    XCC_CHECK_EQ_INT_ID(3, ((long)s.i32) > 0L, 1);
    XCC_CHECK_EQ_INT_ID(4, s.i32 == 123456789L, 1);
    XCC_CHECK_EQ_INT_ID(5, s.i32 == 0x0001CD15L, 0);
    XCC_CHECK_EQ_INT_ID(6, neg < 0, 1);
    XCC_CHECK_EQ_INT_ID(7, neg > 0, 0);
    XCC_CHECK_EQ_INT_ID(8, s.i64 > 0, 1);
    return 0;
}
