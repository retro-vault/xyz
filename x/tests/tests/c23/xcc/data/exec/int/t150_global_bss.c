#include "xcc_exec_test.h"

struct zero_record {
    unsigned char tag;
    unsigned int value;
    unsigned long total;
};

static unsigned char zero_block[513];
static unsigned int explicit_zero = 0;
static struct zero_record zero_aggregate = {0};
static unsigned int initialized_word = 0x1234u;

int
main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, zero_block[0], 0);
    XCC_CHECK_EQ_UINT_ID(2, zero_block[512], 0);
    XCC_CHECK_EQ_UINT_ID(3, explicit_zero, 0);
    XCC_CHECK_EQ_UINT_ID(4, zero_aggregate.tag, 0);
    XCC_CHECK_EQ_UINT_ID(5, zero_aggregate.value, 0);
    XCC_CHECK_EQ_U32_ID(6, zero_aggregate.total, 0, 0);
    XCC_CHECK_EQ_UINT_ID(7, initialized_word, 0x1234u);

    zero_block[512] = 0xa5u;
    explicit_zero = 0x4321u;
    zero_aggregate.total = 0x89abcdeful;

    XCC_CHECK_EQ_UINT_ID(8, zero_block[512], 0xa5u);
    XCC_CHECK_EQ_UINT_ID(9, explicit_zero, 0x4321u);
    XCC_CHECK_EQ_U32_ID(10, zero_aggregate.total, 0xcdefu, 0x89abu);
    return 0;
}
