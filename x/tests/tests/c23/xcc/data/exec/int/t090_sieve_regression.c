#include "xcc_exec_test.h"

#define LIMIT 100

static unsigned char composite[101];

static int count_primes(void) {
    int i;
    int j;
    int count;
    int last;

    for (i = 0; i <= LIMIT; ++i) {
        composite[i] = 0;
    }

    for (i = 2; i * i <= LIMIT; ++i) {
        if (composite[i]) {
            continue;
        }

        for (j = i * i; j <= LIMIT; j += i) {
            composite[j] = 1;
        }
    }

    count = 0;
    last = 0;
    for (i = 2; i <= LIMIT; ++i) {
        if (composite[i]) {
            continue;
        }

        last = i;
        ++count;
    }

    XCC_CHECK_EQ_INT_ID(1, count, 25);
    XCC_CHECK_EQ_INT_ID(2, last, 97);
    return 0;
}

int main(void) {
    return count_primes();
}
