#include "xcc_exec_test.h"

static unsigned long rol7(unsigned long x)  { return (x << 7)  | (x >> 25); }
static unsigned long rol8(unsigned long x)  { return (x << 8)  | (x >> 24); }
static unsigned long rol12(unsigned long x) { return (x << 12) | (x >> 20); }
static unsigned long rol16(unsigned long x) { return (x << 16) | (x >> 16); }
static unsigned long rol17(unsigned long x) { return (x << 17) | (x >> 15); }
static unsigned long rol22(unsigned long x) { return (x << 22) | (x >> 10); }
static unsigned long rol24(unsigned long x) { return (x << 24) | (x >> 8); }

int
main(void)
{
    volatile unsigned long value = 0x12345678ul;

    XCC_CHECK_EQ_U32_ID(1, rol7(value),  0x3c09u, 0x1a2bu);
    XCC_CHECK_EQ_U32_ID(2, rol8(value),  0x7812u, 0x3456u);
    XCC_CHECK_EQ_U32_ID(3, rol12(value), 0x8123u, 0x4567u);
    XCC_CHECK_EQ_U32_ID(4, rol16(value), 0x1234u, 0x5678u);
    XCC_CHECK_EQ_U32_ID(5, rol17(value), 0x2468u, 0xacf0u);
    XCC_CHECK_EQ_U32_ID(6, rol22(value), 0x8d15u, 0x9e04u);
    XCC_CHECK_EQ_U32_ID(7, rol24(value), 0x3456u, 0x7812u);
    return 0;
}
