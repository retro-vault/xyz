#include "xcc_exec_test.h"

static __attribute__((noinline)) unsigned long
load_le32(const unsigned char *bytes, unsigned int offset)
{
    const unsigned char *p = bytes + offset;

    return ((unsigned long)p[3] << 24) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[1] << 8) |
           (unsigned long)p[0];
}

int
main(void)
{
    static const unsigned char bytes[] = {
        0xff, 0x78, 0x56, 0x34, 0x12,
        0xef, 0xcd, 0xab, 0x89
    };

    XCC_CHECK_EQ_U32_ID(1, load_le32(bytes, 1), 0x5678u, 0x1234u);
    XCC_CHECK_EQ_U32_ID(2, load_le32(bytes, 5), 0xcdefu, 0x89abu);
    return 0;
}
