#include "xcc_exec_test.h"

static unsigned int mix_add(unsigned int s, unsigned char salt)
{
    s += (unsigned int)salt;
    s += (unsigned int)(0x31u + salt);
    return s;
}

static unsigned int mix_xor(unsigned int s, unsigned char salt)
{
    s ^= (unsigned int)salt;
    s ^= (unsigned int)(salt ^ 0x5au);
    return s;
}

int main(void)
{
    XCC_CHECK_EQ_UINT_ID(1, mix_add(0x1234u, (unsigned char)0x56u), 0x1311u);
    XCC_CHECK_EQ_UINT_ID(2, mix_add(0xfff0u, (unsigned char)0x20u), 0x0061u);
    XCC_CHECK_EQ_UINT_ID(3, mix_xor(0xabcdu, (unsigned char)0x34u), 0xab97u);
    XCC_CHECK_EQ_UINT_ID(4, mix_xor(0x5500u, (unsigned char)0xa5u), 0x555au);
    return 0;
}
