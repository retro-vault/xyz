#include "xcc_exec_test.h"

static __attribute__((noinline)) void
copyb(unsigned char *dst, const unsigned char *src)
{
    *dst = *src;
}

static __attribute__((noinline)) void
copyw(unsigned int *dst, const unsigned int *src)
{
    *dst = *src;
}

int
main(void)
{
    unsigned char source_byte = 0xa5u;
    unsigned char target_byte = 0u;
    unsigned int source_word = 0x5aa5u;
    unsigned int target_word = 0u;

    copyb(&target_byte, &source_byte);
    copyw(&target_word, &source_word);
    XCC_CHECK_EQ_UINT_ID(1, target_byte, 0xa5u);
    XCC_CHECK_EQ_UINT_ID(2, target_word, 0x5aa5u);
    return 0;
}
