#include "xcc_exec_test.h"

static unsigned int add_salt(unsigned char salt) {
    unsigned int s = 0u;
    s += (unsigned int)(0x31u + salt);
    return s;
}

static int less_than_limit(unsigned char x) {
    return x < 250u;
}

int main(void) {
    XCC_CHECK_EQ_UINT_ID(1, add_salt(0u),   49u);
    XCC_CHECK_EQ_UINT_ID(2, add_salt(200u), 249u);
    XCC_CHECK_EQ_INT_ID (3, less_than_limit(200u), 1);
    XCC_CHECK_EQ_INT_ID (4, less_than_limit(250u), 0);
    XCC_CHECK_EQ_INT_ID (5, less_than_limit(255u), 0);
    return 0;
}
