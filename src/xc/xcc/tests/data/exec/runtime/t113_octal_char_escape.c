#include "xcc_exec_test.h"

int main(void) {
    XCC_CHECK_EQ_INT_ID(1, '\040', 32);
    XCC_CHECK_EQ_INT_ID(2, '\011', 9);
    XCC_CHECK_EQ_INT_ID(3, '\177', 127);
    XCC_CHECK_EQ_INT_ID(4, '\0', 0);
    return 0;
}
