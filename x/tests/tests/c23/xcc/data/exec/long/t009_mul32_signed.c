#include "xcc_exec_test.h"

extern long _mul32(long a, long b);

int main(void) {
    long neg_20000 = 0xffffb1e0ul;
    long neg_30000 = 0xffff8ad0ul;
    long pos_30000 = 30000l;

    XCC_CHECK_EQ_U32_ID(1, _mul32(neg_20000, 4l), 0xc780u, 0xfffeu);
    XCC_CHECK_EQ_U32_ID(2, _mul32(neg_30000, neg_20000), 0x4600u, 0x23c3u);
    XCC_CHECK_EQ_U32_ID(3, _mul32(pos_30000, neg_20000), 0xba00u, 0xdc3cu);
    return 0;
}
