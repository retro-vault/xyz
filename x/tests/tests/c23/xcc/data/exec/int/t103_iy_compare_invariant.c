#include "xcc_exec_test.h"

static volatile int test_keys[3] = {3, 12, -1};

int
score_key(int key)
{
    int i;
    int score = 0;

    for (i = 0; i < 10; ++i) {
        if (i < key)
            score = score + 1;
        if (i == key)
            score = score + 10;
    }
    return score;
}

int
main(void)
{
    XCC_CHECK_EQ_INT_ID(1, score_key(test_keys[0]), 13);
    XCC_CHECK_EQ_INT_ID(2, score_key(test_keys[1]), 10);
    XCC_CHECK_EQ_INT_ID(3, score_key(test_keys[2]), 0);
    return 0;
}
